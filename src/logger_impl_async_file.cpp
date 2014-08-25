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
static logger_impl_mgr::registrar reg("async-file", f);

logger_impl_async_file::logger_impl_async_file(const char* a_name)
    : m_name(a_name), m_append(true), m_levels(LEVEL_NO_DEBUG)
    , m_mode(0644), m_show_location(true), m_show_ident(false)
    , m_engine_ptr(new async_logger_engine())
    , m_engine(m_engine_ptr.get())
{}

void logger_impl_async_file::finalize()
{
    if (m_engine_ptr.get()) {
        if (m_engine == m_engine_ptr.get() && m_engine->running())
            m_engine->stop();
        m_engine_ptr.reset();
    }
    m_engine = nullptr;
}

void logger_impl_async_file::set_engine(multi_file_async_logger& a_engine)
{
    m_engine = &a_engine;

    if (m_engine_ptr.get()) {
        m_engine_ptr->stop();
        m_engine_ptr.reset();
    }
}

std::ostream& logger_impl_async_file::dump(
    std::ostream& out, const std::string& a_pfx) const
{
    out << a_pfx << "logger." << name() << '\n'
        << a_pfx << "    filename       = " << m_filename << '\n'
        << a_pfx << "    append         = " << (m_append ? "true" : "false") << '\n'
        << a_pfx << "    mode           = " << m_mode << '\n'
        << a_pfx << "    levels         = " << logger::log_levels_to_str(m_levels) << '\n'
        << a_pfx << "    show-location  = " << (m_show_location ? "true" : "false") << '\n'
        << a_pfx << "    show-indent    = " << (m_show_ident    ? "true" : "false")
        << std::endl;
    return out;
}

bool logger_impl_async_file::init(const variant_tree& a_config)
    throw(badarg_error,io_error) 
{
    BOOST_ASSERT(this->m_log_mgr);

    if (m_engine->running() && m_engine == m_engine_ptr.get())
        finalize();

    if (!m_engine) {
        if (!m_engine_ptr.get())
            m_engine_ptr.reset(new async_logger_engine());
        m_engine = m_engine_ptr.get();
    }

    try {
        m_filename = a_config.get<std::string>("logger.async-file.filename");
    } catch (boost::property_tree::ptree_bad_data&) {
        throw badarg_error("logger.async_file.filename not specified");
    }

    m_append        = a_config.get<bool>("logger.async-file.append", true);
    m_mode          = a_config.get<int> ("logger.async-file.mode", 0644);
    m_levels        = logger::parse_log_levels(
                        a_config.get<std::string>("logger.async-file.levels",
                                                  logger::default_log_levels));
    m_fd            = m_engine->open_file(m_filename.c_str(), m_append, m_mode);
    m_show_location = a_config.get<bool>("logger.async-file.show-location",
                                         this->m_log_mgr->show_location());
    m_show_ident    = a_config.get<bool>("logger.async-file.show-ident",
                                         this->m_log_mgr->show_ident());

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

    if (!m_engine->running())
        m_engine->start();

    return true;
}

void logger_impl_async_file::log_msg(
    const log_msg_info& info, const timeval* a_tv, const char* fmt, va_list args)
    throw(std::runtime_error)
{
    char buf[logger::MAX_MESSAGE_SIZE];
    int len = logger_impl::format_message(buf, sizeof(buf), true,
                m_show_ident, m_show_location, a_tv, info, fmt, args);
    send_data(info.level(), info.category(), buf, len);
}

void logger_impl_async_file::log_bin(
    const std::string& a_category, const char* msg, size_t size)
    throw(std::runtime_error)
{
    send_data(LEVEL_LOG, a_category, msg, size);
}

void logger_impl_async_file::send_data(
    log_level level, const std::string& a_category, const char* a_msg, size_t a_size)
    throw(io_error)
{
    if (!m_engine->running())
        throw runtime_error("Logger terminated!");

    char* p = m_engine->allocate(a_size);

    if (!p) {
        std::stringstream s("Out of memory allocating ");
        s << a_size << " bytes!";
        throw runtime_error(s.str());
    }

    memcpy(p, a_msg, a_size);

    m_engine->write(m_fd, a_category, p, a_size);
}

} // namespace utxx
