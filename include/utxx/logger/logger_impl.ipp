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

namespace utxx {

template <int N>
inline log_msg_info::log_msg_info(
    logger& a_logger, log_level a_lv, const char (&a_filename)[N], size_t a_ln)
    : m_logger(&a_logger)
    , m_level(a_lv)
    , m_src_file_len(N)
    , m_src_file(a_filename)
    , m_src_line(a_ln)
    /*
    , m_src_location_len(
            m_logger.show_location()
            ? snprintf(m_src_location, sizeof(m_src_location), " [%s:%zd]",
                path::basename(filename, filename+N), ln)
            : 0
      )
    */
{}

template <int N>
inline log_msg_info::log_msg_info(
    log_level a_lv, const std::string& a_category,
    const char (&a_filename)[N], size_t a_ln)
    : m_logger(&logger::instance())
    , m_level(a_lv)
    , m_category(a_category)
    , m_src_file_len(N)
    , m_src_file(a_filename)
    , m_src_line(0)
{}

inline log_msg_info::log_msg_info(
    log_level   a_lv,
    const std::string& a_category)
    : m_logger(NULL)
    , m_level(a_lv)
    , m_category(a_category)
    , m_src_file_len(0)
    , m_src_file("")
    , m_src_line(0)
{}

inline void log_msg_info::log(const char* fmt, ...) {
    va_list args; va_start(args, fmt);
    BOOST_SCOPE_EXIT( (&args) ) { va_end(args); } BOOST_SCOPE_EXIT_END;
    logger* l = m_logger ? m_logger : &logger::instance();
    l->log(*this, fmt, args);
}

//-----------------------------------------------------------------------------
// logger
//-----------------------------------------------------------------------------

template <int N>
inline void logger::log(logger& a_logger, log_level a_level,
    const std::string& a_category,
    const char (&a_filename)[N], size_t a_line,
    const char* a_fmt, va_list args)
{
    log_msg_info info(a_logger, a_level, a_category, a_filename, a_line);
    a_logger.log(info, a_fmt, args);
}

inline void logger::log(const log_msg_info& a_info, const char* a_fmt, va_list args) {
    if (is_enabled(a_info.level()))
        try {
            struct timeval tv;
            gettimeofday(&tv, NULL);

            m_sig_msg[level_to_signal_slot(a_info.level())](
                on_msg_delegate_t::invoker_type(a_info, &tv, a_fmt, args));
        } catch (std::runtime_error& e) {
            if (m_error)
                m_error(e.what());
            else
                throw;
        }
}

inline void logger::log(
    log_level a_level, const std::string& a_category, const char* a_fmt, va_list args)
{
    if (is_enabled(a_level))
        try {
            struct timeval tv;
            gettimeofday(&tv, NULL);

            log_msg_info info(a_level, a_category);
            m_sig_msg[level_to_signal_slot(info.level())](
                on_msg_delegate_t::invoker_type(info, &tv, a_fmt, args));
        } catch (std::runtime_error& e) {
            if (m_error)
                m_error(e.what());
            else
                throw;
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
