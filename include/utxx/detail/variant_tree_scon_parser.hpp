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
#include <utxx/path.hpp>
#include <boost/property_tree/detail/file_parser_error.hpp>
#include <boost/property_tree/detail/info_parser_writer_settings.hpp>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <iterator>
#include <string>
#include <stack>
#include <fstream>
#include <sstream>
#include <cctype>
#include <ctime>

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
    struct scon_reader
    {
        enum mode_t {
            PARSE_STREAM,
            PARSE_DIRECTIVE,
            PARSE_DATA
        };

    private:
        typedef boost::function<bool (std::string& a_filename)> file_resolver;
        typedef std::basic_string<Ch> str_t;

        template<class ChDest, class ChSrc>
        static std::basic_string<ChDest> convert_chtype(const std::basic_string<ChSrc>& s) {
            std::basic_stringstream<ChDest> result;
            for(const ChSrc* text = s; *text; ++text)
                result << ChDest(*text);
            return result.str();
        }

        static str_t       convert_chtype(const str_t& a_str) { return a_str; }
        static std::string convert_chtype(const char*  a_str) { return std::string(a_str); }

        std::basic_istream<Ch>& stream;
        Ptree&                  tree;
        const std::string&      filename;
        int&                    lineno;
        std::basic_string<Ch>&  last_line;
        int                     recursive_depth;
        const Translator&       translator;
        const Ch*&              text;
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

        state_t                 state;  // Parser state
        mode_t                  mode;   // Parsing mode
        bool                    done;

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
            mode_t                  a_parse_mode = PARSE_STREAM
        ) : stream(a_stream)
          , tree(a_tree)
          , filename(a_filename)
          , lineno(a_lineno)
          , last_line(a_last_line)
          , recursive_depth(a_recursive_depth)
          , translator(a_translator)
          , text(a_text)
          , resolver(a_inc_filename_resolver)
          , last(NULL)
          , orig_text(a_text)
          , state(s_key)
          , mode(a_parse_mode)
          , done(false)
        {
            parse_stream();
        }

        /// @return end of parsed text (or NULL if parsed till the end of line)
        static const Ch* parse_expr
        (
            const Ch*           a_text,
            Ptree&              a_tree,     //< Holds the resulting parsed tree
            const Translator&   a_translator,
            const std::string&  a_filename = std::string(),
            int                 a_lineno = 0
        ) {
            std::basic_stringstream<Ch> empty;
            str_t empty_line;

            const Ch* begin = a_text;
            scon_reader(empty, a_tree, a_filename, a_lineno, empty_line, a_translator,
                        begin, NULL, true);
        }

    private:
        void check_stack_size()
        {
            // Check if stack has initial size, otherwise some {'s have not been closed
            if (stack.size() != 1)
                BOOST_PROPERTY_TREE_THROW(
                    boost::property_tree::file_parser_error(
                        "unmatched {", filename, lineno));
        }

        void parse_stream()
        {
            stack.push(&tree);      // Push root ptree on stack initially

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

                internal_parse_line();
            }

            check_stack_size();
        }

        void internal_parse_line()
        {
            static const str_t CS_INCLUDE   = convert_chtype("include");
            static const str_t CS_ENV       = convert_chtype("env");
            static const str_t CS_DATE      = convert_chtype("date");
            static const str_t CS_PATH      = convert_chtype("path");

            // While there are characters left in line
            while (!done && text) {
                // Stop parsing on end of line or comment
                skip_whitespace();
                if (iseol()) {
                    if (state == s_data) // Key =   # No data before comnnent not allowed
                        BOOST_PROPERTY_TREE_THROW(
                            boost::property_tree::file_parser_error(
                                "key is missing value", filename, lineno));
                    if (state == s_data_delim)  // Key     # With no data before comment
                        state = s_kv_delim;

                    if (state != s_kv_delim) {
                        text = NULL;
                        break;
                    }
                }

                // Process according to current parser state
                switch (state)
                {
                    case s_kv_delim:
                    KV_DELIM:
                    {
                        if (mode != PARSE_STREAM) {
                            done = stack.size() == 1;
                            if (done)
                                break;
                        }

                        skip_whitespace();
                        // KeyValue ',' delimiter is optional
                        if (*text == Ch(','))
                            ++text;
                        skip_whitespace();

                        if (iseol())
                            text = NULL;

                        state = s_key;

                    }; break;

                    // Parser expects key
                    case s_key:
                    {
                        // directive is found (e.g. $include, etc.) and it's not
                        // a macro like "$(...)"
                        if (*text == Ch('$') && *(text+1) != '(' &&
                            (mode == PARSE_STREAM || mode == PARSE_DATA))
                        {
                            // Entering directive parsing mode

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
                                           resolver,
                                           mode == PARSE_DATA ? mode : PARSE_DIRECTIVE);

                            typename Ptree::iterator it = temp.begin();

                            if (it == temp.end())
                                BOOST_PROPERTY_TREE_THROW(
                                    boost::property_tree::file_parser_error(
                                        "missing required '$' directive", filename, lineno));

                            bool err = false;

                            if (it->first == CS_INCLUDE && mode == PARSE_STREAM)
                                process_include_file((Ptree&)it->second);
                            else if (mode == PARSE_DATA) {
                                // Expecting the following macro format: $NAME{...}
                                if (!it->second.data().is_null() || it->second.empty())
                                    BOOST_PROPERTY_TREE_THROW(
                                        boost::property_tree::file_parser_error(
                                            std::string("Invalid format of macro '")
                                            + it->first + "': " + orig_text,
                                            filename, lineno));
                                else if (it->first == CS_ENV)
                                    process_env_var((Ptree&)it->second);
                                else if (it->first == CS_DATE && mode == PARSE_DATA)
                                    process_date((Ptree&)it->second);
                                else if (it->first == CS_PATH && mode == PARSE_DATA)
                                    process_path((Ptree&)it->second);
                                else
                                    err = true;
                            } else
                                err = true;

                            if (err)
                                BOOST_PROPERTY_TREE_THROW(
                                    boost::property_tree::file_parser_error(
                                        str_t("invalid '$' directive: ") + it->first,
                                        filename, lineno));

                            lineno = curr_lineno;
                            state  = s_kv_delim;
                            goto KV_DELIM;
                        }
                        else if (*text == Ch('{'))   // Brace opening found
                        {
                            // When reading a directive we map ${} to $env{}
                            if (!last) {
                                if (mode == PARSE_DATA && stack.size() == 1)
                                    last = static_cast<Ptree*>(&stack.top()->push_back(
                                        std::make_pair(CS_ENV, Ptree()))->second);
                                else
                                    BOOST_PROPERTY_TREE_THROW(
                                        boost::property_tree::file_parser_error(
                                            "unexpected {", filename, lineno));
                            }
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
                            goto KV_DELIM;
                        }
                        else if (*text == Ch(','))
                        {
                            if (!last)
                                // This is the case of "{ key, }" or "{k1 d1, , k2 ...}"
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
                            goto KV_DELIM;
                        }
                        else    // Data text found
                        {
                            bool need_more_lines, is_str;
                            std::basic_string<Ch> data =
                                read_data(&need_more_lines, &is_str);
                            state = need_more_lines ? s_data_cont : s_kv_delim;
                            last->data() = *translator.put_value(data, is_str);
                        }
                    }; break;

                    // Parser expects continuation of data after \ on previous line
                    case s_data_cont:
                    {
                        // Last ptree must be defined because we are going to update its data
                        BOOST_ASSERT(last);

                        // Continuation must be wrapped in quotes
                        if (isquote())
                        {
                            bool need_more_lines;
                            std::basic_string<Ch> data =
                                last->template get_value<std::basic_string<Ch> >() +
                                read_string(&need_more_lines, true);
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

        void process_include_file(Ptree& node)
        {
            // $include "filename"
            std::string inc_name = node.data().is_null()
                                ? std::string()
                                : convert_chtype(node.data().to_string());

            // $include { "filename" }
            if (inc_name.empty() && !node.empty()) {
                if (!node.data().is_null())
                    BOOST_PROPERTY_TREE_THROW(
                        boost::property_tree::file_parser_error(
                            std::string(
                                "$include filename node cannot contain data") +
                                orig_text,
                            filename, lineno));
                inc_name = convert_chtype(node.begin()->first);
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
            bool found = path::file_exists(inc_name);
            if (!found) {
                auto dir   = path::dirname(filename);
                auto fname = path::join(dir, inc_name);
                found = path::file_exists(fname);
                if (found)
                    inc_name = fname;
                else if (resolver)
                    found = resolver(inc_name);
            }

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
                           resolver);

            if (!inc_root.empty()) {
                boost::optional<Ptree&> tt = temp.get_child_optional(inc_root);

                if (!tt)
                    BOOST_PROPERTY_TREE_THROW(
                        boost::property_tree::file_parser_error(
                            std::string("required include root path not found: ") +
                                convert_chtype(inc_root.dump()),
                            inc_name, inc_lineno));
                for (typename Ptree::const_iterator
                        tit = tt->begin(), e = tt->end(); tit != e; ++tit)
                    last = static_cast<Ptree*>(&stack.top()->push_back(*tit)->second);
            }
        }

        void process_env_var(Ptree& node)
        {
            // $env{ "var" }
            std::string var = convert_chtype(node.begin()->first);
            str_t res;
            if (var == "EXEPATH")
                res = convert_chtype(path::program::abs_path());
            else {
                const char* env = getenv(var.c_str());
                res = convert_chtype(env ? env : "");
            }
            stack.top()->data() = res;
        }

        void process_date(Ptree& node)
        {
            // $date{Format [, now="Time", utc="true | false"]}
            std::string fmt = convert_chtype(node.begin()->first);
            str_t now = node.get("now", str_t());
            bool  utc = node.get("utc", false);

            struct tm tm;
            now_time(&tm, now, utc);

            char buf[256];
            stack.top()->data() = ::strftime(buf, sizeof(buf), fmt.c_str(), &tm) ? str_t(buf) : "";
        }

        void process_path(Ptree& node)
        {
            // $path{ "PATH" [, now="Time", utc="true | false"] }
            std::string path = convert_chtype(node.begin()->first);
            str_t now = node.get("now", str_t());
            bool  utc = node.get("utc", false);

            struct tm tm;
            now_time(&tm, now, utc);

            stack.top()->data() = utxx::path::replace_env_vars(path, &tm);
        }

        // Expand known escape sequences
        str_t expand_escapes(const Ch *b, bool is_data) {
            std::basic_stringstream<Ch> result;
            while (b < text && b) {
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
                    else if (*b == Ch('$')) result << Ch('$');
                    else if (*b == Ch('\'')) result << Ch('\'');
                    else if (*b == Ch('\\')) result << Ch('\\');
                    else
                        BOOST_PROPERTY_TREE_THROW(boost::property_tree::file_parser_error(
                            std::string("unknown escape sequence: ") + b,
                            filename, lineno));
                }
                else if (*b == Ch('$') && is_data)
                {
                    int curr_lineno = lineno;
                    orig_text = b;
                    std::basic_stringstream<Ch> dummy;
                    Ptree temp;

                    scon_reader rd(dummy, temp, filename, curr_lineno,
                                   last_line, 1,
                                   translator, b,
                                   resolver, PARSE_DATA);

                    if (temp.data().is_null())
                        BOOST_PROPERTY_TREE_THROW(
                            boost::property_tree::file_parser_error(
                                std::string("invalid macro '$' directive: ") + orig_text,
                                filename, lineno));
                    result << temp.data().to_string();
                    continue;
                }
                else
                    result << *b;
                ++b;
            }
            return result.str();
        }

        bool iscomment() const { return *text == Ch('#'); }
        bool iseol()     const { return *text == Ch('\0') || iscomment(); }
        bool isquote()   const { return *text == Ch('\"') || *text == Ch('\''); }


        // Advance pointer past whitespace
        void skip_whitespace() {
            while (std::isspace(*text))
                ++text;
        }

        // Extract word (whitespace delimited) and advance pointer accordingly
        // (skip {{...}} that can be used for defining macros
        str_t read_word(bool is_data) {
            skip_whitespace();
            const Ch *start = text;
            while (true) {
                if (*text == Ch('\0')   || *text == Ch('=') || *text == Ch(',')
                ||  std::isspace(*text) || iscomment())
                    break;
                if (*text == Ch('{')) {
                    if (*(text+1) == Ch('{')) ++text;
                    else                      break;
                }
                if (*text == Ch('}')) {
                    if (*(text+1) == Ch('}')) ++text;
                    else                      break;
                }
                ++text;
            }
            return expand_escapes(start, is_data);
        }

        // Extract string (inside ""), and advance pointer accordingly
        // Set need_more_lines to true if \ continuator found
        str_t read_string(bool *need_more_lines, bool is_data) {
            Ch qchar = *text;

            BOOST_ASSERT(qchar == Ch('\"') || qchar == Ch('\''));

            // Skip \"
            ++text;

            // Find end of string, but skip escaped "
            bool escaped = false;
            const Ch *start = text;

            for(; (escaped || *text != qchar) && !iseol(); ++text)
                escaped = !escaped && *text == Ch('\\');

            // If end of string found
            if (*text != qchar)
                BOOST_PROPERTY_TREE_THROW(
                    boost::property_tree::file_parser_error(
                        "unexpected end of line", filename, lineno));

            str_t result = expand_escapes(start, is_data);

            ++text; // Skip \"
            skip_whitespace();
            if (*text == Ch('\\'))
            {
                if (!need_more_lines)
                    BOOST_PROPERTY_TREE_THROW(boost::property_tree::file_parser_error(
                        "unexpected \\", filename, lineno));
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

        // Extract key
        str_t read_key()
        {
            skip_whitespace();
            if (isquote())
                return read_string(NULL, false);
            else
                return read_word(false);
        }

        // Extract data
        str_t read_data(bool *need_more_lines, bool *is_str)
        {
            skip_whitespace();
            *is_str = isquote();
            if (*is_str)
                return read_string(need_more_lines, true);
            else
            {
                *need_more_lines = false;
                return read_word(true);
            }
        }

        void now_time(struct tm* tm, const str_t& now, bool utc)
        {
            if (now.empty()) {
                time_t time = ::time(NULL);
                utc ? ::gmtime_r(&time, tm) : ::localtime_r(&time, tm);
                return;
            } else if (!::strptime(now.c_str(), "%Y-%m-%d %H:%M:%S", tm)) {
                BOOST_PROPERTY_TREE_THROW(
                    boost::property_tree::file_parser_error(
                        std::string("Invalid format of now time '") +
                            now + "' in the $date{} function: " + orig_text,
                        filename, lineno));
            }
        }

    };

}}  // namespace utxx::detail

#endif
