//----------------------------------------------------------------------------
/// \file   logger_impl.ipp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Supplementary classes for the <logger> class.
//----------------------------------------------------------------------------
// Copyright (C) 2003-2009 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-25
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>

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
#ifndef _UTXX_LOGGER_IMPL_IPP_
#define _UTXX_LOGGER_IMPL_IPP_

#include <utxx/scope_exit.hpp>
#include <boost/filesystem/path.hpp>

namespace utxx {

template <class Alloc>
inline const logger* log_msg_info<Alloc>::get_logger() const {
    return m_logger ? m_logger : &logger::instance();
}

template <class Alloc>
inline logger* log_msg_info<Alloc>::get_logger() {
    return m_logger ? m_logger : &logger::instance();
}

template <class Alloc>
template <int N>
inline log_msg_info<Alloc>::log_msg_info(
    logger& a_logger, log_level a_lv, const char (&a_filename)[N], size_t a_ln,
    const Alloc& a_alloc
)
    : m_logger(&a_logger)
    , m_timestamp(now_utc())
    , m_level(a_lv)
    , m_src_file_len(N-1)
    , m_src_file(a_filename)
    , m_src_line(a_ln)
    , m_data(a_alloc)
    /*
    , m_src_location_len(
            m_logger.show_location()
            ? snprintf(m_src_location, sizeof(m_src_location), " [%s:%zd]",
                path::basename(filename, filename+N), ln)
            : 0
      )
    */
{
    format_header();
}

template <class Alloc>
template <int N>
inline log_msg_info<Alloc>::log_msg_info(
    log_level a_lv, const std::string& a_category,
    const char (&a_filename)[N], size_t a_ln,
    const Alloc& a_alloc
)
    : m_logger(&logger::instance())
    , m_timestamp(now_utc())
    , m_level(a_lv)
    , m_category(a_category)
    , m_src_file_len(N-1)
    , m_src_file(a_filename)
    , m_src_line(a_ln)
    , m_data(a_alloc)
{
    format_header();
}

template <class Alloc>
inline log_msg_info<Alloc>::log_msg_info(
    log_level a_lv, const std::string& a_category, const Alloc& a_alloc
)
    : m_logger(NULL)
    , m_timestamp(now_utc())
    , m_level(a_lv)
    , m_category(a_category)
    , m_src_file_len(0)
    , m_src_file("")
    , m_src_line(0)
    , m_data(a_alloc)
{
    format_header();
}

template <class Alloc>
inline void log_msg_info<Alloc>::log(const char* a_fmt, ...) {
    va_list args; va_start(args, a_fmt);
    UTXX_SCOPE_EXIT([&args]() { va_end(args); });
    format(a_fmt, args);
    get_logger()->log(*this);
}

template <class Alloc>
inline void log_msg_info<Alloc>::format(const char* a_fmt, va_list a_args)
{
    m_data.vprintf(a_fmt, a_args);
    format_footer();
}

template <class Alloc>
inline void log_msg_info<Alloc>::format(const char* a_fmt, ...)
{
    va_list args; va_start(args, a_fmt);
    UTXX_SCOPE_EXIT([&args]() { va_end(args); });
    format(a_fmt, args);
}

template <class Alloc>
inline void log_msg_info<Alloc>::log() {
    format_footer();
    get_logger()->log(*this);
}

template <class Alloc>
void log_msg_info<Alloc>::format_header() {
    // Message mormat: Timestamp|Level|Ident|Category|Message|File:Line
    // Write everything up to Message to the m_data:
    logger* lg = get_logger();

    // Write Timestamp
    size_t len = timestamp::format_size(lg->timestamp_type());
    m_data.reserve(len+1+7+1);
    timestamp::format(lg->timestamp_type(), msg_time(),
                      m_data.str(), m_data.capacity());
    m_data.advance(len);
    m_data.print('|');
    // Write Level
    m_data.print(logger::log_level_to_str(m_level)[0]);
    /*
    static const char s_space[] = "    |";
    len = 7 - logger::log_level_size(m_level);
    assert(len <= sizeof(s_space)-1);
    m_data.sprint(s_space + sizeof(s_space)-2-len, len+1);
    */
    m_data.print('|');
    if (lg->show_ident())
        m_data.print(lg->ident());
    m_data.print('|');
    if (!m_category.empty())
        m_data.print(m_category);
    m_data.print('|');
}

template <class Alloc>
void log_msg_info<Alloc>::format_footer()
{
    // Format the message in the form:
    // Timestamp|Level|Ident|Category|Message|File:Line\n
    logger* lg = get_logger();
    if (has_src_location() && lg->show_location()) {
        static const char s_sep =
            boost::filesystem::path("/").native().c_str()[0];
        if (m_data.last() == '\n')
            m_data.last() = '|';
        else
            m_data.print('|');
        const char* q = strrchr(m_src_file, s_sep);
        q = q ? q+1 : m_src_file;
        auto len = m_src_file + m_src_file_len - q;
        m_data.reserve(len+12);
        m_data.sprint(q, len);
        m_data.print(':');
        m_data.print(m_src_line);
    }
    // We reached the end of the streaming sequence:
    // log_msg_info lmi; lmi << a << b << c;
    char& c = m_data.last();
    if (c != '\n') m_data.print('\n');

    m_data.c_str(); // Writes terminating '\0'
}

//-----------------------------------------------------------------------------
// logger
//-----------------------------------------------------------------------------

inline void logger::do_log(const log_msg_info<>& a_info) {
    try {
        m_sig_msg[level_to_signal_slot(a_info.level())](
            on_msg_delegate_t::invoker_type(a_info));
    } catch (std::runtime_error& e) {
        if (m_error)
            m_error(e.what());
        else
            throw;
    }
}

template <int N>
inline void logger::log(logger& a_logger, log_level a_level,
    const std::string& a_category,
    const char (&a_filename)[N], size_t a_line,
    const char* a_fmt, va_list args)
{
    log_msg_info<> info(a_logger, a_level, a_category, a_filename, a_line);
    info.format(a_fmt, args);
    a_logger.log(info);
}

inline void logger::log(const log_msg_info<>& a_info) {
    if (is_enabled(a_info.level()))
        do_log(a_info);
}

inline void logger::log(
    log_level a_level, const std::string& a_category, const char* a_fmt, va_list args)
{
    if (is_enabled(a_level)) {
        log_msg_info<> info(a_level, a_category);
        info.format(a_fmt, args);
        do_log(info);
    }
}

inline void logger::log(const std::string& a_category, const std::string& a_msg)
{
    if (likely(is_enabled(LEVEL_LOG)))
        try {
            m_sig_bin(on_bin_delegate_t::invoker_type(
                a_category, a_msg.c_str(), a_msg.size()));
        } catch (std::runtime_error& e) {
            if (m_error)
                m_error(e.what());
            else
                throw;
        }
}

inline void logger::log(const std::string& a_category, const char* a_buf, size_t a_size) {
    if (likely(is_enabled(LEVEL_LOG)))
        try {
            m_sig_bin(on_bin_delegate_t::invoker_type(a_category, a_buf, a_size));
        } catch (std::runtime_error& e) {
            if (m_error)
                m_error(e.what());
            else
                throw;
        }
}


} // namespace utxx

#endif  // _UTXX_LOGGER_IMPL_IPP_
