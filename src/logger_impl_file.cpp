//----------------------------------------------------------------------------
/// \file  logger_impl_file.cpp
//----------------------------------------------------------------------------
/// \brief Back-end plugin implementating synchronous file writer for the
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
#include <fcntl.h>
#include <utxx/logger/logger_impl_file.hpp>
#include <boost/thread.hpp>

namespace utxx {

static logger_impl_mgr::impl_callback_t f = &logger_impl_file::create;
static logger_impl_mgr::registrar reg("file", f);

std::ostream& logger_impl_file::dump(std::ostream& out,
    const std::string& a_prefix) const
{
    out << a_prefix << "logger." << name() << '\n'
        << a_prefix << "    filename       = " << m_filename << '\n'
        << a_prefix << "    append         = " << (m_append ? "true" : "false") << '\n'
        << a_prefix << "    mode           = " << m_mode << '\n'
        << a_prefix << "    levels         = " << logger::log_levels_to_str(m_levels) << '\n'
        << a_prefix << "    show_location  = " << (m_show_location ? "true" : "false") << '\n'
        << a_prefix << "    show_indent    = " << (m_show_ident    ? "true" : "false") << '\n';
    return out;
}

bool logger_impl_file::init(const variant_tree& a_config)
    throw(badarg_error) 
{
    BOOST_ASSERT(this->m_log_mgr);
    finalize();

    try {
        m_filename = a_config.get<std::string>("logger.file.filename");
    } catch (boost::property_tree::ptree_bad_data&) {
        throw badarg_error("logger.file.filename not specified");
    }

    m_append        = a_config.get<bool>("logger.file.append", true);
    // See comments in the beginning of the logger_impl_file.hpp on
    // thread safety.  Mutex is enabled by default in the overwrite mode (i.e. "append=false").
    // Use the "use_mutex=false" option to inhibit this behavior if your
    // platform has thread-safe write(2) call.
    m_use_mutex     = m_append ? false : a_config.get<bool>("logger.file.use_mutex", true);
    m_mode          = a_config.get<int> ("logger.file.mode", 0644);
    m_levels        = logger::parse_log_levels(
        a_config.get<std::string>("logger.file.levels", logger::default_log_levels));
    m_show_location = a_config.get<bool>("logger.file.show_location", this->m_log_mgr->show_location());
    m_show_ident    = a_config.get<bool>("logger.file.show_ident",    this->m_log_mgr->show_ident());

    if (m_levels != NOLOGGING) {
        m_fd = open(m_filename.c_str(),
                    O_CREAT|O_WRONLY|O_TRUNC|O_LARGEFILE | (m_append ? O_APPEND : 0),
                    m_mode);
        if (m_fd < 0)
            throw io_error(errno, "Error opening file", m_filename);

        // Install log_msg callbacks from appropriate levels
        for(int lvl = 0; lvl < logger_impl::NLEVELS; ++lvl) {
            log_level level = logger::signal_slot_to_level(lvl);
            if ((m_levels & static_cast<int>(level)) != 0)
                this->add_msg_logger(level,
                    on_msg_delegate_t::from_method<logger_impl_file, &logger_impl_file::log_msg>(this));
        }
        // Install log_bin callback
        this->add_bin_logger(
            on_bin_delegate_t::from_method<logger_impl_file, &logger_impl_file::log_bin>(this));
    }
    return true;
}

class guard {
    boost::mutex& m;
    bool use_mutex;
public:
    guard(boost::mutex& a_m, bool a_use_mutex) : m(a_m), use_mutex(a_use_mutex) {
        if (use_mutex) m.lock();
    }
    ~guard() { 
        if (use_mutex) m.unlock();
    }
};

void logger_impl_file::log_msg(
    const log_msg_info& info, const timestamp& a_tv, const char* fmt, va_list args)
    throw(io_error) 
{
    // See begining-of-file comment on thread-safety of the concurrent write(2) call.
    // Note that since the use of mutex is conditional, we can't use the
    // boost::lock_guard<boost::mutex> guard and roll out our own.
    guard g(m_mutex, m_use_mutex);

    char buf[logger::MAX_MESSAGE_SIZE];
    int len = logger_impl::format_message(buf, sizeof(buf), true, 
                m_show_ident, m_show_location, a_tv, info, fmt, args);
    if (write(m_fd, buf, len) < 0)
        io_error("Error writing to file:", m_filename, ' ',
            (info.has_src_location() ? info.src_location() : ""));
}

void logger_impl_file::log_bin(const char* msg, size_t size) throw(io_error) 
{
    guard g(m_mutex, m_use_mutex);

    if (write(m_fd, msg, size) < 0)
        throw io_error(errno, "Error writing to file:", m_filename);
}

} // namespace utxx
