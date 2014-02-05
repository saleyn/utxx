//----------------------------------------------------------------------------
/// \file  logger_impl_async_file.cpp
//----------------------------------------------------------------------------
/// \brief Back-end plugin implementating asynchronous file writer for the
/// <tt>logger</tt> class.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-25
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2011 Serge Aleynikov <saleyn@gmail.com>

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
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <limits.h>
#include <fcntl.h>
#include <utxx/logger/logger_impl_async_file.hpp>

namespace utxx {

static logger_impl_mgr::impl_callback_t f = &logger_impl_async_file::create;
static logger_impl_mgr::registrar reg("async_file", f);

void logger_impl_async_file::finalize()
{
    if (!m_engine.running())
        m_engine.stop();
}

std::ostream& logger_impl_async_file::dump(std::ostream& out,
        const std::string& a_prefix) const
{
    out << a_prefix << "logger." << name() << '\n'
        << a_prefix << "    filename       = " << m_filename << '\n'
        << a_prefix << "    append         = " << (m_append ? "true" : "false") << '\n'
        << a_prefix << "    mode           = " << m_mode << '\n'
        << a_prefix << "    levels         = " << logger::log_levels_to_str(m_levels) << '\n'
        << a_prefix << "    show_location  = " << (m_show_location ? "true" : "false") << '\n'
        << a_prefix << "    show_indent    = " << (m_show_ident    ? "true" : "false") << '\n' 
        << a_prefix << "    timeout        = " << m_timeout.tv_sec << '.'
                                               << (1.0 / 1000000000.0 * m_timeout.tv_nsec) << '\n';
    return out;
}

bool logger_impl_async_file::init(const variant_tree& a_config)
    throw(badarg_error,io_error) 
{
    BOOST_ASSERT(this->m_log_mgr);
    finalize();

    try {
        m_filename = a_config.get<std::string>("logger.async_file.filename");
    } catch (boost::property_tree::ptree_bad_data&) {
        throw badarg_error("logger.async_file.filename not specified");
    }

    m_append = a_config.get<bool>("logger.async_file.append", true);
    m_mode   = a_config.get<int> ("logger.async_file.mode", 0644);
    m_levels = logger::parse_log_levels(
        a_config.get<std::string>("logger.async_file.levels", logger::default_log_levels));

    m_fd = m_engine.open_file(m_filename.c_str(), m_append, m_mode);

    unsigned long timeout = m_timeout.tv_sec * 1000 + m_timeout.tv_nsec / 1000000ul;

    m_show_location   = a_config.get<bool>("logger.async_file.show_location",
                                           this->m_log_mgr->show_location());
    m_show_ident      = a_config.get<bool>("logger.async_file.show_ident",
                                           this->m_log_mgr->show_ident());
    timeout           = a_config.get<int> ("logger.async_file.timeout", timeout);
    m_timeout.tv_sec  = timeout / 1000;
    m_timeout.tv_nsec = timeout % 1000 * 1000000ul;

    if (m_fd < 0)
        throw io_error(errno, "Error opening file: ", m_filename);

    // Install log_msg callbacks from appropriate levels
    for(int lvl = 0; lvl < logger_impl::NLEVELS; ++lvl) {
        log_level level = logger::signal_slot_to_level(lvl);
        if ((m_levels & static_cast<int>(level)) != 0)
            this->add_msg_logger(level,
                on_msg_delegate_t::from_method<
                    logger_impl_async_file, &logger_impl_async_file::log_msg>(this));
    }
    // Install log_bin callback
    this->add_bin_logger(
        on_bin_delegate_t::from_method<
            logger_impl_async_file, &logger_impl_async_file::log_bin>(this));

    m_engine.start();
    return true;
}

void logger_impl_async_file::log_msg(
    const log_msg_info& info, const timeval* a_tv, const char* fmt, va_list args)
    throw(std::runtime_error)
{
    char buf[logger::MAX_MESSAGE_SIZE];
    int len = logger_impl::format_message(buf, sizeof(buf), true, 
                m_show_ident, m_show_location, a_tv, info, fmt, args);
    send_data(info.level(), buf, len);
}

void logger_impl_async_file::log_bin(const char* msg, size_t size) 
    throw(std::runtime_error) 
{
    send_data(LEVEL_LOG, msg, size);
}

void logger_impl_async_file::send_data(log_level level, const char* msg, size_t size)
    throw(io_error)
{
    if (!m_engine.running())
        throw io_error(m_error.empty() ? "Logger terminated!" : m_error.c_str());

    char* p = m_engine.allocate(size);

    if (!p) {
        std::stringstream s("Out of memory allocating ");
        s << size << " bytes!";
        throw io_error(m_error);
    }

    memcpy(p, msg, size);

    m_engine.write(m_fd, p, size);
}

} // namespace utxx
