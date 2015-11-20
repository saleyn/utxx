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
#include <boost/lexical_cast.hpp>

namespace utxx {

inline long env(const char* a_var, long a_default) {
    const char* p = getenv(a_var);
    return p ? atoi(p) : a_default;
}

namespace {
    template <typename T>
    typename std::enable_if<!std::is_same<T, bool>::value, T>::type
    convert(const char* a) { return boost::lexical_cast<T>(a); }

    template <typename T>
    typename std::enable_if<std::is_same<T, bool>::value, T>::type
    convert(const char* a) {
        return !(strcasecmp(a, "false") == 0 ||
                 strcasecmp(a, "no")    == 0 ||
                 strcasecmp(a, "off")   == 0 ||
                 strcmp    (a, "0")     == 0);
    }

    template <typename Char = const char>
    bool match_opt(int argc, Char** argv,
                   const std::function<void(const char*)>& a_fun,
                   const std::string& a,  int& i) {
        if (a.empty() || argv[i][0] != '-') return false;
        if (a == argv[i]) {
            if (!a_fun && i < argc-1 && argv[i+1][0] != '-')
                return false;
            else
                a_fun(((i+1 >= argc || argv[i+1][0] == '-') ? "" : argv[++i]));
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
        : m_argc(a_argc), m_argv(const_cast<const char**>(a_argv)), m_idx(0)
    {}

    /// Checks if \a a_opt is present at i-th position in the m_argv list
    /// @param a_opt name of the option to match
    bool match(const char* a_opt) const {
        return strcmp(m_argv[m_idx], a_opt) == 0;
    }

    bool match(const std::string& a_short, const std::string& a_long) {
        return match_opt(m_argc, m_argv, nullptr, a_short, m_idx)
            || match_opt(m_argc, m_argv, nullptr, a_long,  m_idx);
    }

    /// Match current option against \a a_short name or \a a_long name
    ///
    /// This function is to be used in the loop:
    /// \code
    /// opts_parser opts(argc, argv);
    /// while (opts.next()) {
    ///     if (opts.match("-a", "", &a)) continue;
    ///     ...
    /// }
    /// \endcode
    template <typename Setter>
    bool match(const std::string& a_short, const std::string& a_long, const Setter& a_fun) {
        return match_opt(m_argc, m_argv, a_fun, a_short, m_idx)
            || match_opt(m_argc, m_argv, a_fun, a_long,  m_idx);
    }

    template <typename T>
    bool match(const std::string& a_short, const std::string& a_long, T* a_val) {
        auto setter = [=](const char* a) { if (a_val) *a_val = convert<T>(a); };
        return match(a_short, a_long, setter);
    }

    /// Find an option identified either by \a a_short name or \a a_long name
    ///
    /// Current fundtion doesn't change internal state variables of this class
    template <typename T>
    bool find(const std::string& a_short, const std::string& a_long, T* a_val) const {
        auto setter = [=](const char* a) { if (a_val) *a_val = convert<T>(a); };
        for (int i=1; i < m_argc; ++i)
            if (match_opt(m_argc, m_argv, setter, a_short, i)
            ||  match_opt(m_argc, m_argv, setter, a_long,  i))
                return true;
        return false;
    }

    bool            is_help() { return match("-h", "--help", (bool*)nullptr); }

    void            reset()        { m_idx = 0;               }
    bool            next()         { return ++m_idx < m_argc; }
    bool            end()    const { return m_idx >= m_argc;  }

    int             argc()   const { return m_argc; }
    const char**    argv()   const { return m_argv; }

    /// Return current option
    const char* operator()() const { return m_idx < m_argc ? m_argv[m_idx]:""; }
};


} // namespace utxx
