#!/usr/bin/python

import argparse
import lxml.etree as et
import os
import pwd
import tempfile
import time
from copy import deepcopy
import sys


def flatten(*n):
    """
    Flatten deep list or tuple. Returns a generator
    """
    return (e for a in n
            for e in (flatten(*a) if isinstance(a, (tuple, list)) else (a,)))

def tree_to_string(root, comments=False, ids=None):
    s = ""
    for n in root.iter():
        if type(n) == et._Comment and not comments:
            continue
        s += node_to_string(n, ids = ids) + ": "
        s += ','.join([i+'='+n.attrib[i] \
                       for i in filter(lambda k: k in ['path','name','val'], n.keys())])
        s += '\n'
    return s

def node_path_to_string(e, ids=None):
    k = list(e.iterancestors())
    k.insert(0, e)
    s = "";
    for i in reversed(k):
        s += "/%s[%s]" % \
            ('<comment>' if type(i) == et._Comment else i.tag,
             (i.attrib[ids] if ids else (id(i)/1000,id(i)%1000)))
    return s;

def node_to_string(e, with_offset=True, ids=None):
    s = ""
    if with_offset:
        k = e.iterancestors()
        s = ' ' * 2 * len([i for i in e.iterancestors()])
    return s + "%s[%s]" % (e.tag, (e.attrib[ids] if ids else (id(e)/1000,id(e)%1000)))

def print_path(e, stream = sys.stdout, ids=None):
    stream.write(node_path_to_string(e, ids=ids) + '\n')

def print_tree(root, stream = sys.stdout, ids=None, comments=False):
    print >> stream, tree_to_string(root, comments=comments, ids=ids)

def print_element(e, stream = sys.stdout, with_offset=True, ids=None):
    print >> stream, node_to_string(e, with_offset=with_offset, ids=ids)


class RenamedTemporaryFile(object):
    """
    A temporary file which will be renamed to the specified
    path on exit.
    """

    def __init__(self, target_filename, **kwargs):
        tmpdir = kwargs.pop('dir', None)

        # Put temporary file in the same directory as the location for the
        # final file so that an atomic move into place can occur.

        if tmpdir is None:
            tmpdir = os.path.dirname(target_filename)

        self.tmpfile = tempfile.NamedTemporaryFile(dir=tmpdir, **kwargs)
        self.final_path = target_filename

    def __getattr__(self, attr):
        """
        Delegate attribute access to the underlying temporary file object.
        """
        return getattr(self.tmpfile, attr)

    def __enter__(self):
        self.tmpfile.__enter__()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if exc_type is None:
            self.tmpfile.delete = False
            result = self.tmpfile.__exit__(exc_type, exc_val, exc_tb)
            os.rename(self.tmpfile.name, self.final_path)
        else:
            result = self.tmpfile.__exit__(exc_type, exc_val, exc_tb)

        return result


class ConfigGenerator(object):
    """
    C++ header file generator for a given XML component
    configuration specification
    """

    def __init__(self, args):
        self.filename   = args.filename
        self.outdir     = args.outdir
        self.verbosity  = args.verbosity
        self.overwrite  = args.overwrite
        self.user       = args.user
        self.email      = args.email
        self.last_id    = 0

    def next_id(self):
        self.last_id += 1
        return self.last_id


    def debug(self, fun, verbosity=1):
        if self.verbosity >= verbosity:
            print >> sys.stderr, fun()

    def parse_xml(self, filename):
        try:
            return et.parse(filename).getroot()
        except:
            print >> sys.stderr, sys.exc_info()[1]
            exit(1)

    def copy_and_check_loops(self, root, skip, visited):
        copies = root.xpath(".//copy")
        self.debug(lambda:
            "Node %s has <copy> children:%s" %
            (node_path_to_string(root,ids='_ID_'),
             "\n" + "".join([node_to_string(i,ids='_ID_')+'\n' for i in copies]) \
                if len(copies) else '  None' ),
            verbosity=2)

        while len(copies):
            tn = copies.pop(0)
            tid = tn.attrib['_ID_']

            if tid in skip:
                continue

            if tid in visited:
                print >> sys.stderr, "Copy loop detected at %s: %s" % \
                                     (node_path_to_string(tn,ids='_ID_'), ','.join(visited))
                exit(1)
 
            path  = tn.attrib['path']
            nodes = tn.xpath(path)

            if not len(nodes):
                print >> sys.stderr, \
                    "Node '%s', has path %s that resolves to no nodes!\n  Skip: [%s]" % \
                    (node_path_to_string(tn,ids='_ID_'), path, ",".join(skip))
                exit(1)

            self.debug(lambda:
                "Scanning node %s [path=%s]\n%s" %
                    (node_path_to_string(tn,ids='_ID_'), path,
                     "".join([node_to_string(i,ids='_ID_')+'\n' for i in nodes])),
                verbosity=2)

            s = list(visited)
            s.insert(0,tid)

            for n in nodes:
                self.debug(lambda:
                    "Jumping %s[%s -> %s]: %s" %
                    (root.base,
                     node_to_string(tn,with_offset=False,ids='_ID_'),
                     node_to_string(n,with_offset=False,ids='_ID_'),
                     ','.join(visited)),
                    verbosity=2)
                self.copy_and_check_loops(n, skip, s)

            self.debug(lambda:
                "Inserting treenodes after %s: [%s]" % (
                    node_to_string(tn,with_offset=False,ids='_ID_'),
                     ",".join([node_to_string(i,with_offset=False,ids='_ID_') \
                         for i in nodes])),
                verbosity=2)

            # Perform the actual copy of nodes
            for n in reversed(nodes):
                l = deepcopy(n)
                # Renumber newly deepcopied nodes
                for i in l.iter(): i.attrib['_ID_'] = str(self.next_id())
                tn.addnext(l)

            # Remove the <copy> element
            if tn.getparent() is not None:
                tn.getparent().remove(tn) 
                self.debug(lambda:
                    "Removed node %s" % (node_to_string(tn,with_offset=False,ids='_ID_')),
                    verbosity=2)

            for i in s:
                skip.add(i)

    def check_include_loops(self, root, filedict, acc):
        for n in sorted(set(root.xpath(".//include"))):
            fn = unicode(n.attrib['file'])
            self.debug(lambda:
                "Checking %s/%s: %s" %(root.base, fn, [i.encode('utf8') for i in acc]))
            if fn not in filedict:
                raise Exception("File: '%s' include node '%s' not found!" %
                    n.base, n.attrib['file'])
            if fn not in acc:
                c = filedict[fn]
                s = set.copy(acc)
                s.add(fn)
                self.check_include_loops(c, filedict, s)
            else:
                print >> sys.stderr, \
                    "File: '{0}' found include loop '{1}'!".format(root.base, fn)
                exit(1)

    def parse_all_files(self, filename, d):
        root = self.parse_xml(filename)
        d[filename] = root
        for file in root.xpath(".//include"):
            n = unicode(file.attrib['file'])
            if n not in d:
                self.parse_all_files(n, d)
        return root

    def expand_includes(self, root, treedict):
        for i in root.xpath(".//include"):
            child = self.parse_xml(i.attrib['file'])
            repl_list = self.expand_includes(child, treedict)
            for j in reversed(repl_list):
                i.addnext(deepcopy(j))
            if i.getparent() is not None: i.getparent().remove(i) 

        return [root] if len(root.keys()) > 0 else root.getchildren()

    def expand_all_includes(self, filename):
        """
        Build a unique dict of include files and parse them all
        """
        d = dict()
        root = self.parse_all_files(filename, d)
        self.debug(lambda:
            "Loaded {0} XML files: {1}".format(
                len(d), [i.encode('utf8') for i in d]))
        self.check_include_loops(root, d, set([filename]))
        root = self.expand_includes(root, d).pop(0)
        # Since includes might have made nodes non-unique, we need to add
        # a surrogate unique identifier in order to check for loops in <copy> tags
        for i in root.iter():
            i.attrib['_ID_'] = str(self.next_id())
        self.copy_and_check_loops(root, set(), [])
        self.debug(lambda: print_tree(root, ids='_ID_'), verbosity=1)
        return root

    def run(self):
        outname = os.path.basename(self.filename.rstrip(".xml").rstrip(".XML") + ".hpp")
        outfile = os.path.join(self.outdir, outname)

        if os.path.isfile(outfile) and not self.overwrite:
            print >> sys.stderr, \
                "File '%s' exists and no --overwrite option provided!\n" % outfile
            exit(2)

        root = self.expand_all_includes(
            unicode(self.filename) if type(self.filename) != unicode else self.filename)[0]
        names = sorted(set(root.xpath("*//@name")))
        values = sorted(set(root.xpath("*//name/@val | *//value/@val")))

        if not len(names):
            print >> sys.stderr, "No config option names are found in XML!\n"
            exit(1)

        with RenamedTemporaryFile(outname, dir=self.outdir) as f:
            f.write("//%s\n" % ("-" * 78))
            f.write("// %s\n" % outname)
            f.write("// This file is auto-generated by utxx/bin/%s\n" \
                    % os.path.basename(sys.argv[0]))
            f.write("//\n// *** DON'T MODIFY BY HAND!!! ***\n//\n")
            f.write("// Copyright (c) 2012 Serge Aleynikov <saleyn at gmail dot com>\n")
            f.write("// Generated by: %s%s\n" % \
                    (self.user, " <" + self.email + ">" if self.email != None else ""))
            f.write("// Created.....: %s\n" % time.strftime('%Y-%m-%d %H:%M:%S'))
            f.write("//%s\n\n" % ("-" * 78))

            ifdeftag = "_UTXX_AUTOGEN_%s_" % \
                       outname.strip(" \\").upper().replace('.', '_')
            f.write("#ifndef %s\n" % ifdeftag)
            f.write("#define %s\n\n" % ifdeftag)
            f.write("#include <utxx/config_validator.hpp>\n\n")
            f.write("namespace %s {\n" % root.attrib['namespace'])
            f.write("    using namespace utxx;\n\n")

            max_name_size = max([len(n) for n in names])
            max_val_size = max([len(n) for n in values]) if len(values) > 0 else 0
            max_width = max(max_name_size, max_val_size)

            f.write("    //---------- Configuration Options ------------\n")
            for n in names:
                u = n.upper()
                f.write('    static const char CFG_%s[]%s = "%s";\n' % (u, ' ' * (max_width - len(u) + 4), u))
            f.write("\n")
            f.write("    //---------- Configuration Values -------------\n")
            for n in values:
                v = n.upper()
                f.write('    static const char CFG_VAL_%s[]%s = "%s";\n' % (v, ' ' * (max_width - len(v)), v))
            f.write("\n" +
                    "    namespace {\n" +
                    "        typedef config::option_map    ovec;\n" +
                    "        typedef config::string_set    sset;\n" +
                    "        typedef config::variant_set   vset;\n" +
                    "    }\n\n");
            name = root.attrib['name']
            f.write("    class %s : public config::validator<%s> {\n" % (name, name))
            f.write("        %s() {}\n" % name)
            f.write("        friend class config::validator<%s>;\n\n" % name)
            f.write("    public:\n")
            f.write("        const %s& init() {\n" % name)
            f.write('            m_root = "%s";\n' % root.attrib['namespace'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Apply configuration option transform")
    parser.add_argument('-f', '--filename',
                        required=True, help="XML options file name")
    parser.add_argument('-o', '--outdir',
                        required=True, help="Destination directory")
    parser.add_argument("--overwrite", action='store_true',
                        default=False, help="Overwrite output file")
    parser.add_argument('--user',
                        default=pwd.getpwuid(os.getuid()).pw_gecos,
                        help="Username of the XML author")
    parser.add_argument('--email',
                        help="Email of the XML author")
    parser.add_argument('-v', '--verbosity', action="count", help='Verbosity level')

    ConfigGenerator(parser.parse_args()).run()
