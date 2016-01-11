//----------------------------------------------------------------------------
/// \file  logger_impl_syslog.cpp
//----------------------------------------------------------------------------
/// \brief Back-end plugin implementating syslog writer for the
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
#include <stdarg.h>
#include <syslog.h>
#include <thread>
#include <boost/algorithm/string.hpp>
#ifndef UTXX_SKIP_LOG_MACROS
#   define __UTXX_TEMP_SET_UTXX_SKIP_LOG_MACROS__
#   define UTXX_SKIP_LOG_MACROS
#endif
#include <utxx/logger/logger_impl.hpp>
#include <utxx/logger/logger_impl_syslog.hpp>
#ifdef __UTXX_TEMP_SET_UTXX_SKIP_LOG_MACROS__
#   undef UTXX_SKIP_LOG_MACROS
#endif
namespace utxx {

static logger_impl_mgr::impl_callback_t f = &logger_impl_syslog::create;
static logger_impl_mgr::registrar reg("syslog", f);

static int parse_syslog_facility(const std::string& facility) 
    throw(std::runtime_error)
{
    std::string s = facility;
    boost::to_lower(s);
    if      (s == "log-user")   return LOG_USER;
    else if (s == "log-local0") return LOG_LOCAL0;
    else if (s == "log-local1") return LOG_LOCAL1;
    else if (s == "log-local2") return LOG_LOCAL2;
    else if (s == "log-local3") return LOG_LOCAL3;
    else if (s == "log-local4") return LOG_LOCAL4;
    else if (s == "log-local5") return LOG_LOCAL5;
    else if (s == "log-local6") return LOG_LOCAL6;
    else if (s == "log-local6") return LOG_LOCAL7;
    else if (s == "log-daemon") return LOG_DAEMON;
    else throw std::runtime_error("Unsupported syslog facility: " + s);
}

static int get_priority(log_level level) {
    switch (level) {
        case LEVEL_DEBUG:   return LOG_DEBUG;
        case LEVEL_INFO:    return LOG_INFO;
        case LEVEL_NOTICE:  return LOG_NOTICE;
        case LEVEL_WARNING: return LOG_WARNING;
        case LEVEL_ERROR:   return LOG_ERR;
        case LEVEL_FATAL:   return LOG_CRIT;
        case LEVEL_ALERT:   return LOG_ALERT;
        default:            return 0;
    }
}

std::ostream& logger_impl_syslog::dump(std::ostream& out,
    const std::string& a_prefix) const
{
    out << a_prefix << "logger." << name() << '\n'
        << a_prefix << "    levels         = " << logger::log_levels_to_str(m_levels) << '\n'
        << a_prefix << "    facility       = " << m_facility << '\n'
        << a_prefix << "    show-pid       = " << (m_show_pid ? "true" : "false") << '\n';
    return out;
}

bool logger_impl_syslog::init(const variant_tree& a_config)
    throw(badarg_error, io_error) 
{
    BOOST_ASSERT(this->m_log_mgr);
    finalize();

    int facility;

    m_levels = logger::parse_log_levels(
        a_config.get<std::string>("logger.syslog.levels", logger::default_log_levels))
        & ~(LEVEL_TRACE  | LEVEL_TRACE1 | LEVEL_TRACE2 |
            LEVEL_TRACE3 | LEVEL_TRACE4 | LEVEL_TRACE5 | LEVEL_LOG);
    m_facility = 
        a_config.get<std::string>("logger.syslog.facility", "log_local6");
    facility   = parse_syslog_facility(m_facility);
    m_show_pid = a_config.get<bool>("logger.syslog.show-pid", true);

    if (m_levels != NOLOGGING) {
        openlog(this->m_log_mgr->ident().c_str(), (m_show_pid ? LOG_PID : 0), facility);

        // Install log_msg callbacks from appropriate levels
        for(int lvl = 0; lvl < logger::NLEVELS; ++lvl) {
            log_level level = logger::signal_slot_to_level(lvl);
            if ((m_levels & static_cast<int>(level)) != 0)
                this->add(level, logger::on_msg_delegate_t::from_method
                     <logger_impl_syslog, &logger_impl_syslog::log_msg>(this));
        }
    }
    return true;
}

void logger_impl_syslog::log_msg
    (const logger::msg& a_msg, const char* a_buf, size_t a_size) throw(io_error)
{
    int priority = get_priority(a_msg.level());
    if (priority)
        ::syslog(priority, "%s", a_buf);
}

} // namespace utxx
