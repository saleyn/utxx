//----------------------------------------------------------------------------
/// \file  logger_impl.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of logger.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-10-04
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx project.

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
#pragma  once

#include <utxx/logger/logger.hpp>
#include <utxx/time_val.hpp>
#include <utxx/convert.hpp>
#include <boost/filesystem/path.hpp>
#include <algorithm>

namespace utxx {

template <typename Fun, int N>
inline bool logger::logf(
    log_level           a_level,
    const std::string&  a_category,
    const Fun&          a_fun,
    const char         (&a_src_loc)[N])
{
    if (!is_enabled(a_level))
        return false;

    return m_queue.emplace(a_level, a_category, a_fun, a_src_loc);
}

template <typename... Args>
inline bool logger::logs(
    log_level           a_level,
    const std::string&  a_category,
    Args&&...           a_args)
{
    if (!is_enabled(a_level))
        return false;

    auto fun = [=](const char* pfx, size_t psz, const char* sfx, size_t ssz) {
        detail::basic_buffered_print<1024> buf;
        buf.print(pfx, psz);
        buf.print(a_args...);
        buf.print(sfx, ssz);
        return buf.to_string();
    };
    return m_queue.emplace(a_level, a_category, fun, "");
}

template <int N, typename... Args>
inline bool logger::log(
    log_level           a_level,
    const std::string&  a_category,
    const char         (&a_src_loc)[N],
    const char*         a_fmt,
    Args&&...           a_args)
{
    if (!is_enabled(a_level))
        return false;

    auto fun = [=](char* a_buf, size_t a_size) {
        return snprintf(a_buf, a_size, a_fmt, a_args...);
    };
    return m_queue.emplace(a_level, a_category, fun, a_src_loc);
}

inline bool logger::log(
    log_level a_level, const std::string& a_category, const std::string& a_msg)
{
    if (!is_enabled(a_level))
        return false;

    auto fun = [=](const char* pfx, size_t psz, const char* sfx, size_t ssz)
    {
        detail::basic_buffered_print<512> buf;
        buf.print(pfx, psz);
        buf.print(a_msg);
        buf.print(sfx, ssz);
        return buf.to_string();
    };
    return m_queue.emplace(a_level, a_category, fun, "");
}

} // namespace utxx
