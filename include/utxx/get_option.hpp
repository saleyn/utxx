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
        return  strcmp(a, "true") == 0 ||
                strcmp(a, "TRUE") == 0 ||
                strcmp(a, "YES")  == 0 ||
                strcmp(a, "yes")  == 0 ||
                strcmp(a, "ON")   == 0 ||
                strcmp(a, "on")   == 0 ||
                strcmp(a, "1")    == 0;
    }
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
    if (a_opt.empty() && a_long_opt.empty()) return false;

    auto check = [=, &a_value](const std::string& a, int& i) {
        if (a.empty()) return false;
        if (a == argv[i]) {
            if (a_value)
                *a_value = convert<T>
                    ((++i >= argc || argv[i][0] == '-') ? "" : argv[i]);
            return true;
        }
        size_t n = strlen(argv[i]);
        if (a.size()+1 <= n && strncmp(a.c_str(), argv[i], a.size()) == 0 &&
                               argv[i][a.size()] == '=') {
            if (a_value)
                *a_value = convert<T>(argv[i] + a.size()+1);
            return true;
        }
        return false;
    };

    for (int i=1; i < argc; i++) {
        if (check(a_opt,      i)) return true;
        if (check(a_long_opt, i)) return true;
    }

    return false;
}

} // namespace utxx
