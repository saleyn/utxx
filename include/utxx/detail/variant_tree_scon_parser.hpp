// ----------------------------------------------------------------------------
// Copyright (C) 2014 Serge Aleynikov
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef _UTXX_VARIANT_TREE_SCON_PARSER_HPP_
#define _UTXX_VARIANT_TREE_SCON_PARSER_HPP_

//#include <utxx/variant_tree.hpp>
#include <utxx/variant.hpp>
#include <utxx/string.hpp>
#include <boost/property_tree/detail/file_parser_error.hpp>
#include <boost/property_tree/detail/info_parser_utils.hpp>
#include <boost/property_tree/detail/info_parser_writer_settings.hpp>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <iterator>
#include <string>
#include <stack>
#include <fstream>
#include <sstream>
#include <cctype>

// SCON (Simple CONfiguration) grammar example:
// ============================================
//
// test {
//   key1 = "value1"            # Comments are preceded by a '#' sign
//   key2 = true                # Supported value types: str | int | double | bool
//
//   key3   value3              # Equal sign is optional as well as double quotes
//                              #   (in the absence of double quotes the data type
//                              #    is inferred. Strings with spaces and special
//                              #    characters have to be double quoted)
//
//   key4 = 4, key5 = 5.0       # Key/value pairs can be separated by ','
//
//   key6 = test1,              # Keys don't have to be unique
//   key6 = test2,              # and before end of line keys can be comma-delimited
//
//   key7 {                     # Keys can have unlimited hierarchy of child nodes
//     key71 = true
//     key72 = value72 {
//       key721 = 100,
//       key722 = 1.0
//     }
//   }
//
//   key8{k1=1,k2=2}            # Some
//   key9=value9{k1=1,k2=2}     #      more
//   key9 value9{k1=1,k2=2}     #           examples
//
//   key10 value10 {            # Child nodes can have values (e.g. "address")
//     key101 = true            # Supported value types: str | int | double | bool
//     $include "name.conf"     # Supports inclusion of other files at any level
//   }
//
//   $include{"name.conf",
//            root=/logger  }   # Include directive may specify that not the
//                              # whole file needs to be included, but only the
//                              # part matching the path
//
//   # Valid include syntax:
//
//   $include "filename"
//   $include { "filename" }
//   $include "filename" { root = "path.to.root.node" }
//   $include{"filename",  root = "path.to.root.node" }
// }

namespace utxx {
namespace detail {
    //-------------------------------------------------------------------------
    // SCON Stream reader
    //-------------------------------------------------------------------------

    template<class Ptree, class Translator, class Ch>
    class scon_reader
    {
        typedef boost::function<bool (std::string& a_filename)> file_resolver;
        typedef std::basic_string<Ch> str_t;

        static str_t convert_chtype(const char* a_str) {
            return boost::property_tree::info_parser::convert_chtype<Ch, char>(a_str);
        }

        std::basic_istream<Ch>& stream;
        Ptree&                  tree;
        const std::string&      filename;
        int&                    lineno;
        std::basic_string<Ch>&  last_line;
        int                     recursive_depth;
        const Translator&       translator;
        const Ch*&              text;
        bool                    one_node_only;
        const file_resolver&    resolver;
        Ptree*                  last;   // Pointer to last created ptree
        std::stack<Ptree*>      stack;  // ptree stack (used to handle nesting)
        const Ch*               orig_text;

        // Possible parser states
        enum state_t {
            s_key,              // Parser expects key
            s_data_delim,       // Parser expects key delimiter
            s_data,             // Parser expects data
            s_data_cont,        // Parser expects data continuation
            s_kv_delim          // Parser expects key-value delimiter ','
        };

    public:
        scon_reader(
            std::basic_istream<Ch>& a_stream,
            Ptree&                  a_tree,
            const std::string&      a_filename,
            int&                    a_lineno,
            std::basic_string<Ch>&  a_last_line,
            int                     a_recursive_depth,
            const Translator&       a_translator,
            const Ch*&              a_text = NULL,
            const file_resolver&    a_inc_filename_resolver = NULL,
            bool                    a_one_node_only = false
        ) : stream(a_stream)
          , tree(a_tree)
          , filename(a_filename)
          , lineno(a_lineno)
          , last_line(a_last_line)
          , recursive_depth(a_recursive_depth)
          , translator(a_translator)
          , text(a_text)
          , one_node_only(a_one_node_only)
          , resolver(a_inc_filename_resolver)
          , last(NULL)
          , orig_text(a_text)
        {
            parse();
        }

    private:
        void parse()
        {
            state_t state = s_key;  // Parser state

            stack.push(&tree);      // Push root ptree on stack initially

            bool done = false;

            try {
                // While there are characters in the stream
                while (stream.good() && !done) {
                    // Read one line from stream
                    if (!text || text[0] == Ch('\0')) {
                        ++lineno;
                        std::getline(stream, last_line);
                        if (!stream.good() && !stream.eof())
                            BOOST_PROPERTY_TREE_THROW(
                                boost::property_tree::file_parser_error(
                                    "read error", filename, lineno));
                        text = last_line.c_str();
                    }

                    BOOST_ASSERT(text);

                    // While there are characters left in line
                    while (!done && text) {
                        // Stop parsing on end of line or comment
                        skip_whitespace();
                        if (iseol()) {
                            switch (state) {
                                case s_data_delim:      // Key     # With no data before commend
                                    state = s_kv_delim;
                                    // Intentionally fall through
                                case s_kv_delim:
                                    goto KV_DELIM;
                                case s_data:            // Key =   # No data before comnnent not allowed
                                    BOOST_PROPERTY_TREE_THROW(
                                        boost::property_tree::file_parser_error(
                                            "key is missing value", filename, lineno));
                                    break;
                                default:
                                    text = NULL;
                                    break;
                            }
                            break;
                        }

                        // Process according to current parser state
                        switch (state)
                        {
                            case s_kv_delim:
                            KV_DELIM:
                            {
                                // KeyValue ',' delimiter is optional
                                if (*text == Ch(','))
                                    ++text;
                                skip_whitespace();
                                if (*text == Ch(','))
                                    BOOST_PROPERTY_TREE_THROW(
                                        boost::property_tree::file_parser_error(
                                            "key expected but ',' delimiter found",
                                            filename, lineno));
                                if (iseol())
                                    text = NULL;

                                if (one_node_only)
                                    done = stack.size() == 1;

                                state = s_key;

                            }; break;

                            // Parser expects key
                            case s_key:
                            {
                                if (*text == Ch('$')) // directive is found (e.g. $include)
                                {
                                    ++text;     // skip '$'
                                    int curr_lineno = lineno;

                                    // Determine the directive type

                                    if (recursive_depth > 100)
                                        BOOST_PROPERTY_TREE_THROW(
                                            boost::property_tree::file_parser_error(
                                                "recursive depth too large, "
                                                "probably recursive include",
                                                filename, lineno));

                                    Ptree temp;
                                    orig_text = text;

                                    scon_reader rd(stream, temp, filename, curr_lineno,
                                                   last_line, recursive_depth + 1,
                                                   translator, text,
                                                   resolver, true);

                                    typename Ptree::iterator it = temp.begin();

                                    if (it == temp.end())
                                        BOOST_PROPERTY_TREE_THROW(
                                            boost::property_tree::file_parser_error(
                                                "missing required '$' directive", filename, lineno));

                                    static const str_t CS_INCLUDE = convert_chtype("include");

                                    if (it->first != CS_INCLUDE)
                                        BOOST_PROPERTY_TREE_THROW(
                                            boost::property_tree::file_parser_error(
                                                str_t("invalid '$' directive: ") + it->first,
                                                filename, lineno));

                                    process_include_file((Ptree&)it->second);

                                    lineno = curr_lineno;
                                    state  = s_kv_delim;
                                }
                                else if (*text == Ch('{'))   // Brace opening found
                                {
                                    if (!last)
                                        BOOST_PROPERTY_TREE_THROW(
                                            boost::property_tree::file_parser_error(
                                                "unexpected {", filename, lineno));
                                    stack.push(last);
                                    last = NULL;
                                    ++text;
                                }
                                else if (*text == Ch('}'))  // Brace closing found
                                {
                                    if (stack.size() <= 1)
                                        BOOST_PROPERTY_TREE_THROW(
                                            boost::property_tree::file_parser_error(
                                                "unmatched }", filename, lineno));
                                    stack.pop();
                                    last = NULL;
                                    ++text;
                                    state = s_kv_delim;
                                }
                                else if (*text == Ch(','))
                                {
                                    if (!last)
                                        // This is the case of "{ key, }"
                                        // but not             "{ ,key=value }"
                                        BOOST_PROPERTY_TREE_THROW(
                                            boost::property_tree::file_parser_error(
                                                std::string("unexpected key-value ',' "
                                                            "delimiter: ") + text,
                                                filename, lineno));
                                    state = s_kv_delim;
                                    goto KV_DELIM;
                                }
                                else    // Key text found
                                {
                                    std::basic_string<Ch> key = read_key();
                                    last = static_cast<Ptree*>(&stack.top()->push_back(
                                        std::make_pair(key, Ptree()))->second);
                                    state = s_data_delim;
                                }

                            }; break;

                            // Parser expects key delimiter
                            case s_data_delim:
                            {
                                if (*text == Ch('=')) { // Delimiter found
                                    ++text;
                                    state = s_data;
                                } else if (*text == Ch(',')) {
                                    state = s_kv_delim;
                                    goto KV_DELIM;
                                } else
                                    state = s_data;     // Delimiter is optional
                            }; break;

                            // Parser expects data
                            case s_data:
                            {
                                // Last ptree must be defined because we are going to add data to it
                                BOOST_ASSERT(last);

                                if (*text == Ch('{'))   // Brace opening found
                                {
                                    stack.push(last);
                                    last = NULL;
                                    ++text;
                                    state = s_key;
                                }
                                else if (*text == Ch('}'))  // Brace closing found
                                {
                                    if (stack.size() <= 1)
                                        BOOST_PROPERTY_TREE_THROW(
                                            boost::property_tree::file_parser_error(
                                                "unmatched }", filename, lineno));
                                    stack.pop();
                                    last = NULL;
                                    ++text;
                                    state = s_kv_delim;
                                }
                                else    // Data text found
                                {
                                    bool need_more_lines, is_str;
                                    std::basic_string<Ch> data =
                                        read_data(&need_more_lines, &is_str);
                                    last->data() = *translator.put_value(data, is_str);
                                    state = need_more_lines ? s_data_cont : s_kv_delim;
                                }
                            }; break;

                            // Parser expects continuation of data after \ on previous line
                            case s_data_cont:
                            {
                                // Last ptree must be defined because we are going to update its data
                                BOOST_ASSERT(last);

                                if (*text == Ch('\"'))  // Continuation must start with "
                                {
                                    bool need_more_lines;
                                    std::basic_string<Ch> data =
                                        last->template get_value<std::basic_string<Ch> >() +
                                        read_string(&need_more_lines);
                                    if (need_more_lines) {
                                        // Use node's data as the temporary accumulator
                                        last->put_value(data);
                                        state = s_data_cont;
                                    } else {
                                        // Convert data to the appropriate type
                                        last->put_value(*translator.put_value(data));
                                        state = s_kv_delim;
                                    }
                                }
                                else
                                    BOOST_PROPERTY_TREE_THROW(
                                        boost::property_tree::file_parser_error(
                                            "expected \" after \\ in previous line",
                                            filename, lineno));

                            }; break;

                            // Should never happen
                            default:
                                BOOST_ASSERT(0);
                        }
                    }
                }

                // Check if stack has initial size, otherwise some {'s have not been closed
                if (stack.size() != 1)
                    BOOST_PROPERTY_TREE_THROW(
                        boost::property_tree::file_parser_error(
                            "unmatched {", filename, lineno));

            }
            catch (boost::property_tree::file_parser_error &e)
            {
                // If line undefined rethrow error with correct filename and line
                BOOST_PROPERTY_TREE_THROW(
                    e.line()
                        ? e
                        : boost::property_tree::file_parser_error(e.message(), filename, lineno));
            }
        }

        void process_include_file(Ptree& node)
        {
            // $include "filename"
            std::string inc_name = node.data().is_null()
                                ? std::string()
                                : convert_chtype(node.data().to_string().c_str());

            // $include { "filename" }
            if (inc_name.empty() && !node.empty()) {
                if (!node.data().is_null())
                    BOOST_PROPERTY_TREE_THROW(
                        boost::property_tree::file_parser_error(
                            std::string(
                                "$include filename node cannot contain data") +
                                orig_text,
                            filename, lineno));
                inc_name = convert_chtype(node.begin()->first.c_str());
            }

            if (inc_name.empty())
                BOOST_PROPERTY_TREE_THROW(
                    boost::property_tree::file_parser_error(
                        std::string(
                            "$include directive missing file name: ") +
                            orig_text,
                        filename, lineno));

            // $include { "filename", root = "path/to/include" }
            // $include "filename" { root = "path/to/include" }
            typename Ptree::path_type inc_root =
                node.get(str_t("root"), str_t());

            // Locate the include file
            bool found = boost::filesystem::exists(inc_name);
            if (!found && resolver)
                found = resolver(inc_name);

            std::basic_ifstream<Ch> inc_stream(inc_name.c_str());
            if (!inc_stream.good())
                BOOST_PROPERTY_TREE_THROW(
                    boost::property_tree::file_parser_error(
                        std::string(found ? "cannot open include file"
                                        : "include file not found")
                        + ": '" + inc_name + "'",
                        filename, lineno));

            // Parse the include file and add the content to
            // current tree (optionally skiping content to root node)
            int   inc_lineno    = 0;
            const Ch* inc_text  = NULL;
            str_t line;
            Ptree temp;

            scon_reader rd(inc_stream, inc_root.empty() ? *stack.top() : temp,
                           inc_name, inc_lineno, line,
                           recursive_depth + 1, translator, inc_text,
                           resolver, false);

            if (!inc_root.empty()) {
                boost::optional<Ptree&> tt = temp.get_child_optional(inc_root);

                if (!tt)
                    BOOST_PROPERTY_TREE_THROW(
                        boost::property_tree::file_parser_error(
                            std::string("required include root path not found: ") +
                                convert_chtype(inc_root.dump().c_str()),
                            inc_name, inc_lineno));
                for (typename Ptree::iterator
                        tit = tt->begin(), e = tt->end(); tit != e; ++tit)
                    last = static_cast<Ptree*>(&stack.top()->push_back(*tit)->second);
            }
        }

        // Expand known escape sequences
        str_t expand_escapes(const Ch *b) {
            std::basic_stringstream<Ch> result;
            while (b != text) {
                if (*b == Ch('\\')) {
                    if (++b == text)
                        BOOST_PROPERTY_TREE_THROW(
                            boost::property_tree::file_parser_error(
                                "character expected after backslash", filename, lineno));
                    else if (*b == Ch('0')) result << Ch('\0');
                    else if (*b == Ch('a')) result << Ch('\a');
                    else if (*b == Ch('b')) result << Ch('\b');
                    else if (*b == Ch('f')) result << Ch('\f');
                    else if (*b == Ch('n')) result << Ch('\n');
                    else if (*b == Ch('r')) result << Ch('\r');
                    else if (*b == Ch('t')) result << Ch('\t');
                    else if (*b == Ch('v')) result << Ch('\v');
                    else if (*b == Ch('"')) result << Ch('"');
                    else if (*b == Ch('\'')) result << Ch('\'');
                    else if (*b == Ch('\\')) result << Ch('\\');
                    else
                        BOOST_PROPERTY_TREE_THROW(boost::property_tree::file_parser_error(
                            std::string("unknown escape sequence: ") + b,
                            filename, lineno));
                }
                else
                    result << *b;
                ++b;
            }
            return result.str();
        }

        bool iscomment() const { return *text == Ch('#'); }
        bool iseol()     const { return *text == Ch('\0') || iscomment(); }

        // Advance pointer past whitespace
        void skip_whitespace() {
            while (std::isspace(*text))
                ++text;
        }

        // Extract word (whitespace delimited) and advance pointer accordingly
        str_t read_word() {
            skip_whitespace();
            const Ch *start = text;
            while (*text != Ch('\0') && *text != Ch('=')
                && *text != Ch(',')  && *text != Ch('{') && *text != Ch('}')
                && !std::isspace(*text) && !iscomment())
                ++text;
            return expand_escapes(start);
        }

        // Extract string (inside ""), and advance pointer accordingly
        // Set need_more_lines to true if \ continuator found
        str_t read_string(bool *need_more_lines) {
            skip_whitespace();
            if (*text == Ch('\"'))
            {
                // Skip \"
                ++text;

                // Find end of string, but skip escaped "
                bool escaped = false;
                const Ch *start = text;

                for(; (escaped || *text != Ch('\"')) && *text != Ch('\0'); ++text)
                    escaped = !escaped && *text == Ch('\\');

                // If end of string found
                if (*text == Ch('\"'))
                {
                    str_t result = expand_escapes(start);
                    ++text; // Skip \"
                    skip_whitespace();
                    if (*text == Ch('\\'))
                    {
                        if (!need_more_lines)
                            BOOST_PROPERTY_TREE_THROW(boost::property_tree::file_parser_error(
                                "unexpected \\", "", 0));
                        ++text;
                        skip_whitespace();
                        if (iseol())
                            *need_more_lines = true;
                        else
                            BOOST_PROPERTY_TREE_THROW(boost::property_tree::file_parser_error(
                                "expected end of line after \\", filename, lineno));
                    }
                    else if (need_more_lines)
                        *need_more_lines = false;
                    return result;
                }
                else
                    BOOST_PROPERTY_TREE_THROW(
                        boost::property_tree::file_parser_error(
                            "unexpected end of line", filename, lineno));
            }
            else
                BOOST_PROPERTY_TREE_THROW(
                    boost::property_tree::file_parser_error(
                        "expected \"", filename, lineno));
        }

        // Extract key
        str_t read_key()
        {
            skip_whitespace();
            if (*text == Ch('\"'))
                return read_string(NULL);
            else
                return read_word();
        }

        // Extract data
        str_t read_data(bool *need_more_lines, bool *is_str)
        {
            skip_whitespace();
            *is_str = *text == Ch('\"');
            if (*is_str)
                return read_string(need_more_lines);
            else
            {
                *need_more_lines = false;
                return read_word();
            }
        }


    };

}}  // namespace utxx::detail

#endif
