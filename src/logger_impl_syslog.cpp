/**
 * Back-end plugin implementating syslog writer for the <logger>
 * class.
 *-----------------------------------------------------------------------------
 * Copyright (c) 2009 Serge Aleynikov <serge@aleynikov.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *-----------------------------------------------------------------------------
 * Created: 2009-11-25
 * $Id$
 */
#include <stdarg.h>
#include <syslog.h>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#define HPCL_SKIP_LOG_MACROS
#include <util/logger/logger_impl_syslog.hpp>

namespace util {

static boost::function< logger_impl* (void) > f = &logger_impl_syslog::create;
static logger_impl_mgr::registrar reg("syslog", f);

static int parse_syslog_facility(const std::string& facility) 
    throw(std::runtime_error)
{
    std::string s = facility;
    boost::to_upper(s);
    if      (s == "LOG_USER")   return LOG_USER;
    else if (s == "LOG_LOCAL0") return LOG_LOCAL0;
    else if (s == "LOG_LOCAL1") return LOG_LOCAL1;
    else if (s == "LOG_LOCAL2") return LOG_LOCAL2;
    else if (s == "LOG_LOCAL3") return LOG_LOCAL3;
    else if (s == "LOG_LOCAL4") return LOG_LOCAL4;
    else if (s == "LOG_LOCAL5") return LOG_LOCAL5;
    else if (s == "LOG_LOCAL6") return LOG_LOCAL6;
    else if (s == "LOG_LOCAL6") return LOG_LOCAL7;
    else if (s == "LOG_DAEMON") return LOG_DAEMON;
    else throw std::runtime_error("Unsupported syslog facility: " + s);
}

static int get_priority(log_level level) {
    switch (level) {
        case LEVEL_DEBUG:   return LOG_DEBUG;
        case LEVEL_INFO:    return LOG_INFO;
        case LEVEL_WARNING: return LOG_WARNING;
        case LEVEL_ERROR:   return LOG_ERR;
        case LEVEL_FATAL:   return LOG_CRIT;
        case LEVEL_ALERT:   return LOG_ALERT;
        default:        return 0;
    }
}

bool logger_impl_syslog::init(const boost::property_tree::ptree& a_config)
    throw(badarg_error, io_error) 
{
    BOOST_ASSERT(this->m_log_mgr);
    finalize();
    new (this) logger_impl_syslog();

    int facility;
    bool show_pid;

    m_levels = logger::parse_log_levels(
        a_config.get<std::string>("logger.syslog.levels", logger::default_log_levels))
        & ~(LEVEL_TRACE | LEVEL_TRACE1 | LEVEL_TRACE2 |
            LEVEL_TRACE3 | LEVEL_TRACE4 | LEVEL_TRACE5 | LEVEL_LOG);
    facility = parse_syslog_facility(
        a_config.get<std::string>("logger.syslog.facility", "log_local6"));
    show_pid = a_config.get<bool>("logger.syslog.show_pid", true);

    if (m_levels != NOLOGGING) {
        openlog(this->m_log_mgr->ident().c_str(), (show_pid ? LOG_PID : 0), facility);

        // Install log_msg callbacks from appropriate levels
        for(int lvl = 0; lvl < logger_impl::NLEVELS; ++lvl) {
            log_level level = logger::signal_slot_to_level(lvl);
            if ((m_levels & static_cast<int>(level)) != 0)
                this->m_log_mgr->add_msg_logger(level, msg_binder[lvl],
                    on_msg_delegate_t::from_method<logger_impl_syslog, &logger_impl_syslog::log_msg>(this));
        }
    }
    return true;
}

int get_priority(log_level level);

void logger_impl_syslog::log_msg(
    const log_msg_info& info, const timestamp&, const char* fmt, va_list args)
    throw(io_error) 
{
    int priority = get_priority(info.level());
    if (priority)
        ::vsyslog(priority, fmt, args);
}

} // namespace util
