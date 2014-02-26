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
#include <cctype>

// SCON (Simple CONfiguration) grammar example:
// ============================================
//
// test {
//   key1 = "value1"            # This is a comment
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
//   $include{file=name.conf,
//            root=/logger  }   # Include directive may specify that not the
//                              # whole file needs to be included, but only the
//                              # part matching the path
//
//   # Valid include syntax:
//
//   $include "filename"
//   $include="filename"
//   $include { "filename" }
//   $include "filename" { root = "path.to.root.node" }
// }

namespace utxx {
namespace detail {
    // Expand known escape sequences
    template<class It>
    std::basic_string<typename std::iterator_traits<It>::value_type>
        expand_escapes(It b, It e, const std::string& file, int lineno)
    {
        typedef typename std::iterator_traits<It>::value_type Ch;
        std::basic_string<Ch> result;
        while (b != e)
        {
            if (*b == Ch('\\'))
            {
                ++b;
                if (b == e)
                    BOOST_PROPERTY_TREE_THROW(
                        boost::property_tree::file_parser_error(
                            "character expected after backslash", file, lineno));
                else if (*b == Ch('0')) result += Ch('\0');
                else if (*b == Ch('a')) result += Ch('\a');
                else if (*b == Ch('b')) result += Ch('\b');
                else if (*b == Ch('f')) result += Ch('\f');
                else if (*b == Ch('n')) result += Ch('\n');
                else if (*b == Ch('r')) result += Ch('\r');
                else if (*b == Ch('t')) result += Ch('\t');
                else if (*b == Ch('v')) result += Ch('\v');
                else if (*b == Ch('"')) result += Ch('"');
                else if (*b == Ch('\'')) result += Ch('\'');
                else if (*b == Ch('\\')) result += Ch('\\');
                else
                    BOOST_PROPERTY_TREE_THROW(boost::property_tree::file_parser_error(
                        std::string("unknown escape sequence: ") + b,
                        file, lineno));
            }
            else
                result += *b;
            ++b;
        }
        return result;
    }

    template <class Ch>
    bool iscomment(const Ch *text)
    {
        return *text == Ch('#');
    }

    // Advance pointer past whitespace
    template<class Ch>
    void skip_whitespace(const Ch *&text)
    {
        while (std::isspace(*text))
            ++text;
    }

    // Extract word (whitespace delimited) and advance pointer accordingly
    template<class Ch>
    std::basic_string<Ch> read_word(const Ch *&text, const std::string& file, int lineno)
    {
        using namespace std;
        skip_whitespace(text);
        const Ch *start = text;
        while (*text != Ch('\0') && *text != Ch('=')
            && *text != Ch(',')  && *text != Ch('{') && *text != Ch('}')
            && !isspace(*text) && !iscomment(text))
            ++text;
        return expand_escapes(start, text, file, lineno);
    }

    // Extract string (inside ""), and advance pointer accordingly
    // Set need_more_lines to true if \ continuator found
    template<class Ch>
    std::basic_string<Ch> read_string(
        const Ch *&text, bool *need_more_lines, const std::string& file, int lineno)
    {
        skip_whitespace(text);
        if (*text == Ch('\"'))
        {
            // Skip "
            ++text;

            // Find end of string, but skip escaped "
            bool escaped = false;
            const Ch *start = text;
            while ((escaped || *text != Ch('\"')) && *text != Ch('\0'))
            {
                escaped = (!escaped && *text == Ch('\\'));
                ++text;
            }

            // If end of string found
            if (*text == Ch('\"'))
            {
                std::basic_string<Ch> result = expand_escapes(start, text++, file, lineno);
                skip_whitespace(text);
                if (*text == Ch('\\'))
                {
                    if (!need_more_lines)
                        BOOST_PROPERTY_TREE_THROW(boost::property_tree::file_parser_error(
                            "unexpected \\", "", 0));
                    ++text;
                    skip_whitespace(text);
                    if (*text == Ch('\0') || iscomment(text))
                        *need_more_lines = true;
                    else
                        BOOST_PROPERTY_TREE_THROW(boost::property_tree::file_parser_error(
                            "expected end of line after \\", file, lineno));
                }
                else if (need_more_lines)
                    *need_more_lines = false;
                return result;
            }
            else
                BOOST_PROPERTY_TREE_THROW(
                    boost::property_tree::file_parser_error(
                        "unexpected end of line", file, lineno));
        }
        else
            BOOST_PROPERTY_TREE_THROW(
                boost::property_tree::file_parser_error(
                    "expected \"", file, lineno));
    }

    // Extract key
    template<class Ch>
    std::basic_string<Ch> read_key(const Ch *&text, const std::string& file, int lineno)
    {
        skip_whitespace(text);
        if (*text == Ch('\"'))
            return read_string(text, NULL, file, lineno);
        else
            return read_word(text, file, lineno);
    }

    // Extract data
    template<class Ch>
    std::basic_string<Ch> read_data(
        const Ch *&text, bool *need_more_lines, bool *is_str,
        const std::string& file, int lineno
    )
    {
        skip_whitespace(text);
        *is_str = *text == Ch('\"');
        if (*is_str)
            return read_string(text, need_more_lines, file, lineno);
        else
        {
            *need_more_lines = false;
            return read_word(text, file, lineno);
        }
    }

    //-------------------------------------------------------------------------
    // SCON Stream reader
    //-------------------------------------------------------------------------

    template<class Ptree, class Translator, class Ch>
    void read_scon_internal
    (
        std::basic_istream<Ch>& stream,
        Ptree&                  pt,
        const std::string&      filename,
        int&                    lineno,
        std::basic_string<Ch>&  last_line,
        int                     recursive_depth,
        const Translator&       translator,
        const Ch*&              text = NULL,
        const boost::function<bool (std::string& a_filename)>
                                inc_filename_resolver = NULL,
        int                     max_node_count = -1
    )
    {
        typedef std::basic_string<Ch> str_t;
        // Possible parser states
        enum state_t {
            s_key,              // Parser expects key
            s_data_delim,       // Parser expects key delimiter
            s_data,             // Parser expects data
            s_data_cont,        // Parser expects data continuation
            s_kv_delim          // Parser expects key-value delimiter ','
        };

        state_t state = s_key;          // Parser state
        Ptree *last = NULL;             // Pointer to last created ptree

        // Initialize ptree stack (used to handle nesting)
        std::stack<Ptree *> stack;
        stack.push(&pt);                // Push root ptree on stack initially

        try {
            // While there are characters in the stream
            while (stream.good() && max_node_count) {
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
                while (max_node_count) {
                    // Stop parsing on end of line or comment
                    skip_whitespace(text);
                    if (*text == Ch('\0') || iscomment(text)) {
                        text = NULL;
                        break;
                    }

                    // Process according to current parser state
                    switch (state)
                    {
                        // Parser expects key
                        case s_key:
                        {
                            if (*text == Ch('$')) // directive is found (e.g. $include)
                            {
                                namespace bpi = boost::property_tree::info_parser;
                                static const str_t s_include =
                                    bpi::convert_chtype<Ch, char>("include");

                                ++text;     // skip '$'

                                int curr_lineno = lineno;

                                // Determine the directive type

                                if (recursive_depth > 100) {
                                    BOOST_PROPERTY_TREE_THROW(
                                        boost::property_tree::file_parser_error(
                                            "recursive depth too large, "
                                            "probably recursive include",
                                            filename, lineno));
                                } else {
                                    Ptree temp;
                                    const Ch* oldtext = text;

                                    read_scon_internal(stream, temp, filename, curr_lineno,
                                                    last_line,
                                                    recursive_depth + 1, translator, text,
                                                    inc_filename_resolver, 1);

                                    typename Ptree::iterator it = temp.begin();

                                    if (it == temp.end())
                                        BOOST_PROPERTY_TREE_THROW(
                                            boost::property_tree::file_parser_error(
                                                "missing required '$' directive", filename, lineno));

                                    if (it->first != s_include)
                                        BOOST_PROPERTY_TREE_THROW(
                                            boost::property_tree::file_parser_error(
                                                str_t("invalid '$' directive: ") + it->first,
                                                filename, lineno));

                                    // $include "filename"
                                    std::string inc_name = it->second.data().is_null()
                                                        ? std::string()
                                                        : bpi::convert_chtype<char, Ch>(
                                                            it->second.data().to_string().c_str());

                                    // $include { "filename" }
                                    if (inc_name.empty() && !it->second.empty()) {
                                        if (!it->second.data().is_null())
                                            BOOST_PROPERTY_TREE_THROW(
                                                boost::property_tree::file_parser_error(
                                                    std::string(
                                                        "$include filename node cannot contain data") +
                                                        oldtext,
                                                    filename, lineno));
                                        inc_name = bpi::convert_chtype<char, Ch>(
                                                    it->second.begin()->first.c_str());
                                    }

                                    if (inc_name.empty())
                                        BOOST_PROPERTY_TREE_THROW(
                                            boost::property_tree::file_parser_error(
                                                std::string(
                                                    "$include directive missing file name: ") +
                                                    oldtext,
                                                filename, lineno));

                                    // $include { "filename", root = "path/to/include" }
                                    // $include "filename" { root = "path/to/include" }
                                    typename Ptree::path_type inc_root =
                                        it->second.get(str_t("root"), str_t());

                                    // Locate the include file
                                    bool found = boost::filesystem::exists(inc_name);
                                    if (!found && inc_filename_resolver)
                                        found = inc_filename_resolver(inc_name);

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

                                    if (inc_root.empty())
                                        read_scon_internal(inc_stream, *stack.top(),
                                                            inc_name, inc_lineno, line,
                                                            recursive_depth + 1, translator, inc_text,
                                                            inc_filename_resolver);
                                    else {
                                        Ptree temp;
                                        read_scon_internal(inc_stream, temp,
                                                            inc_name, inc_lineno, line,
                                                            recursive_depth + 1, translator, inc_text,
                                                            inc_filename_resolver);
                                        boost::optional<Ptree&> tree = temp.get_child_optional(inc_root);

                                        if (!tree)
                                            BOOST_PROPERTY_TREE_THROW(
                                                boost::property_tree::file_parser_error(
                                                    std::string("required include root path not found: ") +
                                                        bpi::convert_chtype<char, Ch>(
                                                            inc_root.dump().c_str()),
                                                    inc_name, inc_lineno));
                                        for (typename Ptree::iterator tit = temp.begin(), e = temp.end();
                                                tit != e; ++tit)
                                            last = static_cast<Ptree*>(&stack.top()->push_back(
                                                *tit)->second);
                                    }

                                    lineno = curr_lineno;
                                    state  = s_kv_delim;
                                }
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
                            }
                            else    // Key text found
                            {
                                std::basic_string<Ch> key = read_key(text, filename, lineno);
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
                                    read_data(text, &need_more_lines, &is_str,
                                              filename, lineno);
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
                                    read_string(text, &need_more_lines, filename, lineno);
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

                        case s_kv_delim:
                        {
                            // KeyValue ',' delimiter is optional
                            if (*text == Ch(','))
                                ++text;
                            skip_whitespace(text);
                            if (*text == Ch(','))
                                BOOST_PROPERTY_TREE_THROW(
                                    boost::property_tree::file_parser_error(
                                        "key expected but ',' delimiter found",
                                        filename, lineno));

                            if (max_node_count > 0)
                                max_node_count--;

                            state = s_key;

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

}}  // namespace utxx::detail

#endif
