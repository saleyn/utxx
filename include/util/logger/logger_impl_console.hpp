/**
 * Back-end plugin implementing console message writer for the <logger>
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
#ifndef _UTIL_LOGGER_CONSOLE_HPP_
#define _UTIL_LOGGER_CONSOLE_HPP_

#include <util/logger.hpp>

namespace util { 

class logger_impl_console: public logger_impl {
    int  m_stdout_levels;
    int  m_stderr_levels;
    bool m_show_location;
    bool m_show_ident;

    static const int s_def_stdout_levels = LEVEL_INFO | LEVEL_WARNING;
    static const int s_def_stderr_levels = LEVEL_ERROR | LEVEL_FATAL | LEVEL_ALERT;

    logger_impl_console()
        : m_stdout_levels(s_def_stdout_levels)
        , m_stderr_levels(s_def_stderr_levels)
        , m_show_location(true)
        , m_show_ident(false)
    {}

public:
    static logger_impl_console* create() {
        return new logger_impl_console();
    }

    virtual ~logger_impl_console() {}

    bool init(const boost::property_tree::ptree& a_config)
        throw(badarg_error, io_error);

    void log_msg(const log_msg_info& info, const timestamp& a_tv,
        const char* fmt, va_list args) throw(std::runtime_error);
};

} // namespace util

#endif

