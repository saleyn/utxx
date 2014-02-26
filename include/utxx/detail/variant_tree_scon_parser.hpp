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
//    key1 = "value1"           % This is a comment
//    key2    value2            % Equal sign is optional as well as double quotes
//                              %   (in the absence of double quotes the data type
//                              %    is inferred)
//    key3 = 3,  key4 = 2.0     % Key/value pairs can be separated by ','
//    key4 = "non-unique"       % Keys don't have to be unique
//    type address {            % Child nodes can have values (e.g. "address")
//        encoded = true        % Supported value types: str | int | double | bool
//    }
//    #include "file.conf"      % Supports inclusion of other files on any level
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
        return *text == Ch('%');
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
        int                     lineno,
        int                     include_depth,
        const Translator&       translator,
        const boost::function<bool (std::string& a_filename)>
                                inc_filename_resolver = NULL
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
        // Define line here to minimize reallocations
        str_t line;

        // Initialize ptree stack (used to handle nesting)
        std::stack<Ptree *> stack;
        stack.push(&pt);                // Push root ptree on stack initially

        try {
            // While there are characters in the stream
            while (stream.good()) {
                // Read one line from stream
                ++lineno;
                std::getline(stream, line);
                if (!stream.good() && !stream.eof())
                    BOOST_PROPERTY_TREE_THROW(
                        boost::property_tree::file_parser_error(
                            "read error", filename, lineno));
                const Ch *text = line.c_str();

                // If directive found
                skip_whitespace(text);
                if (state == s_key && *text == Ch('#')) {
                    namespace bpi = boost::property_tree::info_parser;
                    static const std::basic_string<Ch> s_include =
                        bpi::convert_chtype<Ch, char>("include");
                    // Determine directive type
                    ++text;     // skip #
                    std::basic_string<Ch> directive = read_word(text, filename, lineno);
                    if (directive == s_include) {
                        // #include
                        if (include_depth > 100) {
                            BOOST_PROPERTY_TREE_THROW(
                                boost::property_tree::file_parser_error(
                                    "include depth too large, "
                                    "probably recursive include",
                                    filename, lineno));
                        }
                        str_t s = read_string(text, NULL, filename, lineno);
                        std::string inc_name = bpi::convert_chtype<char, Ch>(s.c_str());

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
                        int inc_lineno = 0;
                        read_scon_internal(inc_stream, *stack.top(),
                                           inc_name, inc_lineno,
                                           include_depth + 1, translator,
                                           inc_filename_resolver);
                    } else {   // Unknown directive
                        BOOST_PROPERTY_TREE_THROW(
                            boost::property_tree::file_parser_error(
                                "unknown '#' directive", filename, lineno));
                    }

                    // Directive must be followed by end of line
                    skip_whitespace(text);
                    if (*text != Ch('\0') && !iscomment(text)) {
                        BOOST_PROPERTY_TREE_THROW(
                            boost::property_tree::file_parser_error(
                                "expected end of line", filename, lineno));
                    }

                    // Go to next line
                    continue;
                }

                // While there are characters left in line
                while (1) {
                    // Stop parsing on end of line or comment
                    skip_whitespace(text);
                    if (*text == Ch('\0') || iscomment(text))
                        break;
                        /*
                        if (state == s_key_delim) // no data
                            state = s_key;
                        else if (state == s_kv_delim) // no key-value ',' delimiter
                            state = s_key;
                    */

                    // Process according to current parser state
                    switch (state)
                    {

                        // Parser expects key
                        case s_key:
                        {

                            if (*text == Ch('{'))   // Brace opening found
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
                            }
                            else if (*text == Ch(','))
                            {
                                if (!last)
                                    // This is the case of "{ key=value, }"
                                    // but not             "{ ,key=value }"
                                    BOOST_PROPERTY_TREE_THROW(
                                        boost::property_tree::file_parser_error(
                                            std::string("unexpected key-value ',' "
                                                        "delimiter: ") + text,
                                            filename, lineno));

                                ++text;
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
                                ++text;
                                state = s_key;
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
                                state = s_key;
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
