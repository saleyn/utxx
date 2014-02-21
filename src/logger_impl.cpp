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
#include <utxx/logger/logger.hpp>
#include <utxx/time_val.hpp>
#include <utxx/convert.hpp>
#include <boost/filesystem/path.hpp>
#include <algorithm>

namespace utxx {

logger_impl::logger_impl()
    : m_log_mgr(NULL), m_bin_sink_id(-1)
{
    for (int i=0; i < NLEVELS; ++i)
        m_msg_sink_id[i] = -1;
}

logger_impl::~logger_impl()
{
    if (m_log_mgr) {
        for (int i=0; i < NLEVELS; ++i)
            if (m_msg_sink_id[i] != -1) {
                log_level level = logger::signal_slot_to_level(i);
                m_log_mgr->remove_msg_logger(level, m_msg_sink_id[i]);
                m_msg_sink_id[i] = -1;
            }
        if (m_bin_sink_id != -1) {
            m_log_mgr->remove_bin_logger(m_bin_sink_id);
            m_bin_sink_id = -1;
        }
    }
}

void logger_impl::add_msg_logger(log_level level, on_msg_delegate_t subscriber)
{
    m_msg_sink_id[logger::level_to_signal_slot(level)] =
        m_log_mgr->add_msg_logger(level, subscriber);
}

void logger_impl::add_bin_logger(on_bin_delegate_t subscriber)
{
    m_bin_sink_id = m_log_mgr->add_bin_logger(subscriber);
}

int logger_impl::format_message(
    char* buf, size_t size, bool add_new_line,
    bool a_show_ident, bool a_show_location,
    const timeval* a_tv,
    const log_msg_info& info, const char* fmt, va_list args
) throw (badarg_error)
{
    // Format the message in the form:
    // Timestamp|Ident|Level|Category|Message|File:Line

    int len = timestamp::format(m_log_mgr->timestamp_type(), a_tv, buf, size);
    const char* end = buf + size - 2;
    const char* q   = logger::log_level_to_str(info.level());
    char* p = buf + len;
    *p++ = '|';
    const char* e = p + 7;
    while (*q)    *p++ = *q++;
    while (p < e) *p++ = ' ';
    *p++ = '|';
    if (a_show_ident) {
        q = info.get_logger()->ident().c_str();
        while (*q) *p++ = *q++;
    }
    *p++ = '|';
    if (!info.category().empty()) {
        int n = std::min((long)info.category().size(), end-p);
        strncpy(p, info.category().c_str(), n);
        p += n;
    }
    *p++ = '|';

    int n = vsnprintf(p, end - p, fmt, args);

    if (n < 0)
        throw badarg_error("Error formatting string:", fmt, ' ',
            (info.has_src_location() ? info.src_file() : ""));
    p += n;

    if (info.has_src_location() && a_show_location) {
        static const char s_sep = boost::filesystem::path("/").native().c_str()[0];
        *p++ = '|';
        q = strrchr(info.src_file(), s_sep);
        q = q ? q+1 : info.src_file();
        int len = info.src_file() + info.src_file_len() - q;

        if (q+len+12 < end) {
            strncpy(p, q, len);
            p += len;
            *p++ = ':';
            itoa(info.src_line(), p);
        }
    }

    if (add_new_line)
        *p++ = '\n';
    *p = '\0';
    len = p - buf;
    BOOST_ASSERT(len <= (int)size);
    return len;
}


} // namespace utxx
