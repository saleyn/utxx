//----------------------------------------------------------------------------
/// \file  path.ipp
//----------------------------------------------------------------------------
/// \brief Collection of general purpose functions for path manipulation.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-05-06
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
#ifndef _UTXX_PATH_IPP_
#define _UTXX_PATH_IPP_

#include <string>
#include <list>
#include <fstream>
#include <utxx/error.hpp>
#include <utxx/time_val.hpp>
#include <unistd.h>
#include <boost/xpressive/xpressive.hpp>
#include <regex>

namespace utxx {
namespace path {

inline std::string
replace_env_vars(const std::string& a_path, time_val a_now, bool a_utc,
                 const std::map<std::string, std::string>*  a_bindings)
{
    if (!a_now.empty()) {
        auto tm = a_now.to_tm(a_utc);
        return replace_env_vars(a_path, &tm, a_bindings);
    }
    return replace_env_vars(a_path, nullptr, a_bindings);
}

inline std::string
replace_env_vars(const std::string& a_path, const struct tm* a_now,
                 const std::map<std::string, std::string>*   a_bindings)
{
    using namespace boost::posix_time;
    using namespace boost::xpressive;

    auto regex_format_fun = [a_bindings](const smatch& what) {
        if (what[1].str() == "EXEPATH") // special case
            return program::abs_path();

        if (a_bindings) {
            auto it = a_bindings->find(what[1].str());
            if (it != a_bindings->end())
                return it->second;
        }

        const char* env = getenv(what[1].str().c_str());
        return std::string(env ? env : "");
    };

    auto regex_home_var = [](const smatch& what) { return home(); };

    std::string x(a_path);
    {
        #if defined(_MSC_VER) || defined(_WIN32) || defined(__CYGWIN32__)
        sregex re = "%"  >> (s1 = +_w) >> '%';
        #else
        sregex re = "${" >> (s1 = +_w) >> '}';
        #endif

        x = regex_replace(x, re, regex_format_fun);
    }
    #if !defined(_MSC_VER) && !defined(_WIN32) && !defined(__CYGWIN32__)
    {
        sregex re = '$' >> (s1 = +_w);
        x = regex_replace(x, re, regex_format_fun);
    }
    {
        sregex re = bos >> '~';
        x = regex_replace(x, re, regex_home_var);
    }
    #endif

    if (a_now != NULL && x.find('%') != std::string::npos) {
        char buf[384];
        if (strftime(buf, sizeof(buf), x.c_str(), a_now) == 0)
            throw badarg_error("Invalid time specification!");
        return buf;
    }
    return x;
}

} // namespace path
} // namespace utxx

#endif // _UTXX_PATH_IPP_