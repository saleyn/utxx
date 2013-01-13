//----------------------------------------------------------------------------
/// \file   logger_impl_syslog.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Back-end plugin implementating syslog writer for the <logger>
/// class.  
///
/// Configuration options:
///  - logger.syslog.levels = LEVELS::string()
///      where LEVELS is pipe separated list of log levels to be sent to
///      syslog. Only levels llDEBUG ... llALERT can be used.
///  - logger.syslog.facility = FACILITY::string()
///      where FACILITY is the syslog facility. Only facilities LOG_USER,
///      LOG_LOCAL0-7, LOG_DAEMON are supported. Default is LOG_LOCAL6.
///  - logger.syslog.show_pid = bool()
///      If true the process's PID is included in output of syslog. Default 
///      is true.
///
/// Testing:
/// <code>
/// $(ROOT)/test/test_logger --run_test=test_logger_syslog
/// </code>
/// After running the test, check <"/var/log/messages"> file for corresponding
/// messages.  Usually the file has root only access rights, so manual checking
/// is needed.
//----------------------------------------------------------------------------
// Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>
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
#ifndef _UTXX_LOGGER_FILE_HPP_
#define _UTXX_LOGGER_FILE_HPP_

#include <utxx/logger.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <boost/thread.hpp>

namespace utxx {

class logger_impl_syslog: public logger_impl {
    std::string m_name;
    int         m_levels;
    std::string m_facility;
    bool        m_show_pid;

    logger_impl_syslog(const char* a_name)
        : m_name(a_name), m_levels(LEVEL_NO_DEBUG & ~LEVEL_LOG)
    {}

    void finalize() {
        if (m_levels != NOLOGGING)
            ::closelog();
    }
public:
    static logger_impl_syslog* create(const char* a_name) {
        return new logger_impl_syslog(a_name);
    }

    virtual ~logger_impl_syslog() {
        finalize();
    }

    const std::string& name() const { return m_name; }

    /// Dump all settings to stream
    std::ostream& dump(std::ostream& out, const std::string& a_prefix) const;

    bool init(const variant_tree& a_config)
        throw(badarg_error, io_error);

    void log_msg(const log_msg_info& info, const timestamp& a_tv,
        const char* fmt, va_list args) throw(io_error);
};

} // namespace utxx

#endif

