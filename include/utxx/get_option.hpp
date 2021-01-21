//----------------------------------------------------------------------------
/// \file  get_option.hpp
//----------------------------------------------------------------------------
/// \brief Get command-line option values.
//----------------------------------------------------------------------------
// Author:  Serge Aleynikov
// Created: 2010-01-06
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

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
#pragma once

#include <stdlib.h>
#include <string.h>
#include <type_traits>
#include <utxx/typeinfo.hpp>
#include <utxx/error.hpp>
#include <boost/lexical_cast.hpp>

namespace utxx {

inline long env(const char* a_var, long a_default) {
    const char* p = getenv(a_var);
    return p ? atoi(p) : a_default;
}

namespace {
    template <typename T>
    std::enable_if_t<!std::is_same_v<T, bool> && !std::is_same_v<T, std::nullptr_t>, T>
    convert(const char* a) {
        try {
            return boost::lexical_cast<T>(a);
        } catch (...) {
            UTXX_THROW_RUNTIME_ERROR("Cannot convert value '", a, "' to type: ",
                                     type_to_string<T>());
        }
    }

    template <typename T>
    std::enable_if_t<std::is_same_v<T, bool>, T>
    convert(const char* a) {
        return !(strcasecmp(a, "false") == 0 ||
                 strcasecmp(a,   "off") == 0 ||
                 strcasecmp(a,    "no") == 0 ||
                 strcmp    (a,     "0") == 0);
    }

    template <typename T>
    std::enable_if_t<std::is_same_v<T, std::nullptr_t>, T>
    convert(const char* a) {
        return nullptr;
    }

    template <typename Char = const char>
    bool match_opt(int argc, Char** argv,
                   const std::function<void(const char*)>& a_fun,
                   const std::string& a,  int& i) {
        if (a.empty() || argv[i][0] != '-') return false;
        if (a == argv[i]) {
            int  nextlen = i < argc-1 ? strlen(argv[i+1]) : 0;
            bool has_arg = nextlen && strcmp(argv[i+1], "--") != 0 &&
                                      ((argv[i+1][0] == '-' && nextlen == 1) ||
                                       (argv[i+1][0] != '-'));
            if (a_fun)
                a_fun(has_arg ? argv[++i] : "");
            else if (has_arg)
                // Fail if this option has argument, while it's not supposed to.
                return false;

            return true;
        }
        size_t n = strlen(argv[i]);
        if (a.size()+1 <= n && strncmp(a.c_str(), argv[i], a.size()) == 0 &&
                               argv[i][a.size()] == '=') {
            if (a_fun)
                a_fun(argv[i] + a.size()+1);
            return true;
        }
        return false;
    };
}

/// Get command line option given its short name \a a_opt and optional long name
///
/// @param argc         argument passed to main()
/// @param argv         argument passed to main()
/// @param a_fun        value handling function "void(const char* a_value)"
/// @param a_opt        option short name (e.g. "-o")
/// @param a_long_opt   option long name (e.g. "--output")
/// @return true if given option is found in the \a argv list, in which case the
///         \a a_value is set to the value of the option (e.g. "-o filename",
///         "--output=filename").
template <typename Setter, typename Char = const char>
bool get_opt(int argc, Char** argv, const Setter& a_fun,
             const std::string& a_opt, const std::string& a_long_opt = "")
{
    if (a_opt.empty() && a_long_opt.empty()) return false;

    for (int i=1; i < argc; i++) {
        if (match_opt<Char>(argc, argv, a_fun, a_opt,      i)) return true;
        if (match_opt<Char>(argc, argv, a_fun, a_long_opt, i)) return true;
    }

    return false;
}

/// Get command line option given its short name \a a_opt and optional long name
///
/// @param argc         argument passed to main()
/// @param argv         argument passed to main()
/// @param a_value      option value to be set (can be NULL)
/// @param a_opt        option short name (e.g. "-o")
/// @param a_long_opt   option long name (e.g. "--output")
/// @return true if given option is found in the \a argv list, in which case the
///         \a a_value is set to the value of the option (e.g. "-o filename",
///         "--output=filename").
template <typename T, typename Char = const char>
bool get_opt(int argc, Char** argv, T* a_value,
             const std::string& a_opt, const std::string& a_long_opt = "")
{
    auto setter = [=](const char* a) { if (a_value) *a_value = convert<T>(a); };
    return get_opt(argc, argv, setter, a_opt, a_long_opt);
}

/// Parser command-line options
class opts_parser {
    int           m_argc;
    const char**  m_argv;
    int           m_idx;

public:
    template <typename Char = const char>
    opts_parser(int a_argc, Char** a_argv)
        : m_argc(a_argc), m_argv(const_cast<const char**>(a_argv))
        , m_idx(0)
    {}

    /*
    /// Checks if \a a_opt is present at i-th position in the m_argv list
    /// @param a_opt name of the option to match
    bool test(const char* a_opt) const {
        return strcmp(m_argv[m_idx], a_opt) == 0;
    }
    */

    // NOTE: The functions below are intentionally not implementing
    // assignment of default NULL argument! Otherwise get ambiguous
    // method calls!
    bool match(const std::string& a_opt) {
        return match({ a_opt }, nullptr);
    }

    bool match(const std::string& a_opt, const std::string& a_long) {
        return match({ a_opt, a_long }, nullptr);
    }

    bool match(std::initializer_list<std::string> a_opt) {
        return match(a_opt, nullptr);
    }

    /// Match current option against \a a_short name or \a a_long name
    ///
    /// This function is to be used in the loop:
    /// \code
    /// opts_parser opts(argc, argv);
    /// while (opts.next()) {
    ///     if (opts.match("-a", "",        &a)) continue;
    ///     if (opts.match({"-b"},          &b)) continue;
    ///     if (opts.match({"-c", "--cat"}, &c)) continue;
    ///     ...
    /// }
    /// \endcode
    template <typename Setter>
    bool match(const std::string& a_short, const std::string& a_long, const Setter& a_fun) {
        return match({ a_short, a_long }, a_fun);
    }

    template <typename T>
    bool match(const std::string& a_short, const std::string& a_long, T* a_val) {
        return match({ a_short, a_long }, a_val);
    }

    template <typename Setter, typename T>
    bool match(const std::string& a_short, const std::string& a_long,
               const Setter& a_convert, T* a_val) {
        return match({ a_short, a_long }, a_convert, a_val);
    }

    template <typename Setter>
    bool match(std::initializer_list<std::string> a_opt, const Setter& a_fun) {
        for (auto& opt : a_opt)
            if (match_opt(m_argc, m_argv, a_fun, opt, m_idx))
                return true;
        return false;
    }

    template <typename T>
    bool match(std::initializer_list<std::string> a_opt, T* a_val) {
        auto setter = [=](const char* a) { if (a_val) *a_val = convert<T>(a); };
        return match(a_opt, setter);
    }

    template <typename Setter, typename T>
    bool match(std::initializer_list<std::string> a_opt, const Setter& a_convert, T* a_val) {
        assert(a_val);
        auto setter = [&](const char* a) { if (a_val) *a_val = a_convert(a); };
        for (auto& opt : a_opt)
            if (match_opt(m_argc, m_argv, setter, opt, m_idx))
                return true;
        return false;
    }

    /// Find an option identified either by \a a_short name or \a a_long name
    ///
    /// Current fundtion doesn't change internal state variables of this class
    template <typename T = std::nullptr_t>
    bool find(const std::string& a_short, T* a_val = nullptr) const {
        return find({a_short}, a_val);
    }

    template <typename T = std::nullptr_t>
    bool find(const std::string& a_short, const std::string& a_long, T* a_val = nullptr) const {
        return find({a_short, a_long}, a_val);
    }

    template <typename T = std::nullptr_t>
    bool find(std::initializer_list<std::string> a_opt, T* a_val = nullptr) const {
        auto setter = [=](const char* a) { if (a_val) *a_val = convert<T>(a); };
        for (int i=1; i < m_argc; ++i)
            for (auto& s : a_opt)
                if (match_opt(m_argc, m_argv, setter, s, i))
                    return true;
        return false;
    }

    bool            is_help() { return match("-h", "--help", (bool*)nullptr); }

    void            reset()        { m_idx = 0;               }
    bool            next()         { return ++m_idx < m_argc; }
    bool            end()    const { return m_idx >= m_argc;  }
    int             index()  const { return m_idx;            }

    int             argc()   const { return m_argc; }
    const char**    argv()   const { return m_argv; }

    /// Return current option
    const char* operator()() const { return m_idx < m_argc ? m_argv[m_idx]:""; }
};


} // namespace utxx
