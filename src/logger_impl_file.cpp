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
#include <utxx/logger/logger_impl.hpp>
#include <utxx/path.hpp>
#include <boost/thread.hpp>

namespace utxx {

static logger_impl_mgr::impl_callback_t f = &logger_impl_file::create;
static logger_impl_mgr::registrar reg("file", f);

std::ostream& logger_impl_file::dump(std::ostream& out,
    const std::string& a_prefix) const
{
    out << a_prefix << "logger." << name() << '\n'
        << a_prefix << "    filename       = " << m_filename << '\n'
        << a_prefix << "    append         = " << (m_append ? "true" : "false")       << '\n'
        << a_prefix << "    mode           = " << m_mode << '\n';
    if (!m_symlink.empty()) out <<
           a_prefix << "    symlink        = " << m_symlink << '\n';
    out << a_prefix << "    levels         = " << logger::log_levels_to_str(m_levels) << '\n'
        << a_prefix << "    use-mutex      = " << (m_use_mutex ? "true" : "false")    << '\n'
        << a_prefix << "    no-header      = " << (m_no_header ? "true" : "false")    << '\n';
    return out;
}

bool logger_impl_file::init(const variant_tree& a_config)
    throw(badarg_error, io_error) 
{
    BOOST_ASSERT(this->m_log_mgr);
    finalize();

    try {
        m_filename = a_config.get<std::string>("logger.file.filename");
        m_filename = m_log_mgr->replace_macros(m_filename);
    } catch (boost::property_tree::ptree_bad_data&) {
        throw badarg_error("logger.file.filename not specified");
    }

    m_append        = a_config.get<bool>("logger.file.append", true);
    // See comments in the beginning of the logger_impl_file.hpp on
    // thread safety.  Mutex is enabled by default in the overwrite mode (i.e. "append=false").
    // Use the "use-mutex=false" option to inhibit this behavior if your
    // platform has thread-safe write(2) call.
    m_use_mutex     = !m_append || a_config.get("logger.file.use-mutex", true);
    m_no_header     = a_config.get("logger.file.no-header", false);
    m_mode          = a_config.get("logger.file.mode",       0644);
    m_symlink       = a_config.get("logger.file.symlink",      "");
    auto levels     = a_config.get("logger.file.levels",       "");

    m_levels = levels.empty()
             ? m_log_mgr->level_filter()
             : logger::parse_log_levels(levels);

    auto lll = __builtin_ffs(m_levels)-1;
    auto lev = __builtin_ffs(m_log_mgr->level_filter())-1;
    if  (lll < lev)
        UTXX_THROW_RUNTIME_ERROR("File logger's levels filter '", levels,
                                 "' is less granular than logger's default '",
                                 logger::log_levels_to_str(m_log_mgr->min_level_filter()),
                                 "'");

    if (m_levels != NOLOGGING) {
        bool exists = path::file_exists(m_filename);
        m_fd = open(m_filename.c_str(),
                    O_CREAT|O_WRONLY|O_LARGEFILE | (m_append ? O_APPEND : 0),
                    m_mode);
        if (m_fd < 0)
            UTXX_THROW_IO_ERROR(errno, "Error opening file ", m_filename);

        if (!m_symlink.empty()) {
            m_symlink = m_log_mgr->replace_macros(m_symlink);
            if (!utxx::path::file_symlink(m_filename, m_symlink, true))
                UTXX_THROW_IO_ERROR(errno, "Error creating symlink ", m_symlink,
                                    " -> ", m_filename, ": ");
        }

        // Write field information
        if (!m_no_header) {
            char buf[256];
            char* p = buf, *end = buf + sizeof(buf);

            tzset();

            auto ll = m_log_mgr->log_level_to_string
                        (as_log_level(__builtin_ffs(m_levels)), false);
            int  tz = -timezone;
            int  hh = abs(tz / 3600);
            int  mm = abs(tz % 60);
            p += snprintf(p, p - end, "# Logging started at: %s %c%02d:%02d (MinLevel: %s)\n#",
                          timestamp::to_string(DATE_TIME).c_str(),
                          tz > 0 ? '+' : '-', hh, mm, ll.c_str());
            if (!exists) {
                if (!this->m_log_mgr ||
                    this->m_log_mgr->timestamp_type() != stamp_type::NO_TIMESTAMP)
                    p += snprintf(p, p - end, "Timestamp|");
                p += snprintf(p, p - end, "Level|");
                if (this->m_log_mgr) {
                    if (this->m_log_mgr->show_ident())
                        p += snprintf(p, p - end, "Ident|");
                    if (this->m_log_mgr->show_thread())
                        p += snprintf(p, p - end, "Thread|");
                    if (this->m_log_mgr->show_category())
                    p += snprintf(p, p - end, "Category|");
                }
                p += snprintf(p, p - end, "Message");
                if (this->m_log_mgr && this->m_log_mgr->show_location())
                    p += snprintf(p, p - end, " [File:Line%s]",
                                this->m_log_mgr->show_fun_namespaces() ? " Function" : "");
            }
            *p++ = '\n';

            if (write(m_fd, buf, p - buf) < 0)
                throw io_error(errno, "Error writing log header to file: ", m_filename);
        }

        // Install log_msg callbacks from appropriate levels
        for(int lvl = 0; lvl < logger::NLEVELS; ++lvl) {
            log_level level = logger::signal_slot_to_level(lvl);
            if ((m_levels & static_cast<int>(level)) != 0)
                this->add(level,
                    logger::on_msg_delegate_t::from_method
                        <logger_impl_file, &logger_impl_file::log_msg>(this));
        }
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

void logger_impl_file::log_msg(const logger::msg& a_msg, const char* a_buf, size_t a_size)
    throw(io_error)
{
    // See begining-of-file comment on thread-safety of the concurrent write(2) call.
    // Note that since the use of mutex is conditional, we can't use the
    // boost::lock_guard<boost::mutex> guard and roll out our own.
    guard g(m_mutex, m_use_mutex);

    if (write(m_fd, a_buf, a_size) < 0)
        throw io_error(errno, "Error writing to file: ", m_filename, ' ', a_msg.src_location());
}

} // namespace utxx
