// vim:ts=2 et sw=2
//----------------------------------------------------------------------------
/// \file tailagg.cpp
//----------------------------------------------------------------------------
/// \brief Tail a file by merging lines beginning with given regex's.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-08-28
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <regex>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <utxx/path.hpp>
#include <utxx/time_val.hpp>
#include <utxx/buffer.hpp>
#include <boost/concept_check.hpp>

using namespace std;

void usage(std::string const& a_err = "")
{
  if (!a_err.empty())
    std::cerr << "Error: " << a_err << endl << endl;

  std::cerr << utxx::path::program::name()
    << " [-k KeyRegEx] [-e RegEx] [-s S] [-n N] Filename\n"
    << "Extended tail that allows to batch changes on lines matching\n"
    << "regular expressions and print them per interval\n\n"
    << "    -e RegEx                 - process line containing regular expression\n"
    << "    -k[I] KeyRegEx           - use KeyRegEx to determine a key ID of a line\n"
    << "                               (the line will be printed if its content changes\n"
    << "                                for this key. If -k3 is given, this means to use\n"
    << "                                3rd group in the regex pattern)\n"
    << "    -n N                     - start tail from last N lines\n"
    << "    -s, --sleep-interval=S   - sleep for approximately S seconds (default 1s)\n"
    << "    -i, --no-case            - ignore case in regex\n"
    << "    --awk                    - use regex awk grammar\n"
    << "    --grep                   - use regex grep grammar\n"
    << "    --egrep                  - use regex egrep grammar\n"
    << "    -h, --help               - help\n"
    << endl;

  exit(1);
}

void print(const vector<string>& a_lines, bool changed[])
{
  for (int i=0, e = a_lines.size(); i < e; ++i)
    if (changed[i]) {
      changed[i] = false;
      std::cout << a_lines[i] << endl;
    }
  flush(std::cout);
}

void find_last_line(long a_count, istream* a_file)
{
  if (a_count <= 0)
    return;

  a_file->seekg(-1, ios_base::end);

  long count = 0, pos = a_file->tellg();

  while(count < a_count && pos > 0) {
    long sz = std::min<long>(1024, pos);
    a_file->seekg(-sz, ios_base::cur);
    long new_pos = a_file->tellg();

    char  buf[1024];
    int   n = a_file->readsome(buf, sz);

    if (n < 0)
        exit(1);

    char* p = buf + n;

    for (; p != buf; --p)
        if (*p == '\n' && ++count == a_count) {
            a_file->seekg(new_pos + p - buf);
            return;
        }

    pos = new_pos;
  }

  a_file->clear();
}

enum class rex {
  KEY,      // Find a match by regex key
  SEARCH    // Apply regex to the line without key checking of result
};

struct rex_info {
  rex      type;
  int      group;
  unique_ptr<regex> exp;
  string   last_key;
  string   str_exp;

  rex_info(rex tp, int grp, string val)
    : type(tp)
    , group(grp)
    , str_exp(val)
  {}
};

int main(int argc, char* argv[])
{
  int    interval = 1;
  string filename;
  long   last = 0;
  regex_constants::syntax_option_type regex_opts =
    regex_constants::syntax_option_type(0);
  vector<rex_info> regex_vals;
  vector<string>   lines;

  ifstream input;
  istream* file = &std::cin;

  auto matchopt = [&](int i, const char* sv, const char* lv)
                  { return !strcmp(argv[i], sv) || !lv || !strcmp(argv[i], lv); };
  auto matchopt_n = [&](int i, int len, const char* sv)
                  { return !strncmp(argv[i], sv, len); };
  auto hasarg   = [&](int i)
                  { return i < argc-1 && argv[i+1][0] != '-'; };

  for (int i=1; i < argc; ++i) {
    if (matchopt(i, "-s", "--sleep-interval") && hasarg(i))
      interval = atoi(argv[++i]);
    else if (matchopt(i, "-e", nullptr) && hasarg(i))
      regex_vals.push_back(rex_info(rex::SEARCH, 0, argv[++i]));
    else if (matchopt_n(i, 2, "-k") && hasarg(i)) {
      regex_vals.push_back(rex_info(
          rex::KEY,
          argv[i][2] != ' ' ? argv[i][2] - '0' : 1,
          argv[i]));
      i++;
    }
    else if (matchopt(i, "-h", "--help"))
      usage();
    else if (matchopt(i, "-n", nullptr) && hasarg(i))
      last = atoi(argv[++i]);
    else if (matchopt(i, "-i", "--no-case"))
      regex_opts |= regex_constants::icase;
    else if (matchopt(i, "--awk", nullptr))
      regex_opts |= regex_constants::awk;
    else if (matchopt(i, "--grep", nullptr))
      regex_opts |= regex_constants::grep;
    else if (matchopt(i, "--egrep", nullptr))
      regex_opts |= regex_constants::egrep;
    else if (argv[i][0] != '-')
    {
      filename = argv[i];
      input.open(filename.c_str(), ifstream::in);
      if (input.fail())
      {
        cerr << "Failed to open file: " << filename << endl;
        exit(1);
      }
      file = &input;
    }
    else
      usage(string("Invalid option: ") + argv[i]);
  }

  for (auto& e : regex_vals)
    e.exp.reset(new regex(e.str_exp, regex_opts));

  size_t sz = std::max<size_t>(1, regex_vals.size());

  bool changed[sz];
  memset(changed, 0, sizeof(bool)*sz);

  lines.resize(sz);

  utxx::time_val deadline = utxx::now_utc() + 1.0, last_time;
  int change_count = 0;

  find_last_line(last, file);

  auto now = utxx::now_utc();

  while(true) {
    if (now < deadline)
      usleep(long(deadline.diff(now) * 1000000));

    string s;
    while (getline(*file, s)) {
      if (s.empty())
        continue;

      for (size_t i = 0, e = sz; i < e; ++i) {
        auto& re = regex_vals[i];
        std::smatch rmatch;
        if (regex_vals.empty() || regex_search(s, rmatch, *re.exp))
        {
          bool&   line_changed = changed[i];
          string& last_key     = re.last_key;

          if (!line_changed) {
            line_changed |= ((re.type == rex::KEY)
                          ? last_key != rmatch[re.group]
                          : lines[i] != s);

            if (line_changed)
              change_count++;
          }

          if (re.type == rex::KEY)
            last_key = rmatch[re.group];
          lines[i] = s;
        }
      }

      now = utxx::now_utc();

      if (now >= deadline && change_count) {
        print(lines, changed);
        change_count = 0;
        deadline = now + utxx::time_val(interval, 0);
      }
    }

    if (!file->eof())
       exit(1);

    file->clear();

    deadline = now + utxx::time_val(interval, 0);
  }

  return 0;
}
