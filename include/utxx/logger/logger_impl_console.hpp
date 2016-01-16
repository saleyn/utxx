//----------------------------------------------------------------------------
/// \file   logger_impl_console.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Back-end plugin implementing console message writer for the
///        <logger> class.
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
#ifndef _UTXX_LOGGER_CONSOLE_HPP_
#define _UTXX_LOGGER_CONSOLE_HPP_

#include <utxx/logger.hpp>
#include <utxx/time_val.hpp>

namespace utxx { 

class logger_impl_console: public logger_impl {
    std::string  m_name;
    bool         m_color;
    bool         m_stdout_is_tty = true;
    bool         m_stderr_is_tty = true;
    int          m_stdout_levels;
    int          m_stderr_levels;

    static const int s_def_stdout_levels = LEVEL_NOTICE | LEVEL_INFO  | LEVEL_WARNING;
    static const int s_def_stderr_levels = LEVEL_ERROR  | LEVEL_FATAL | LEVEL_ALERT;

    logger_impl_console(const char* a_name)
        : m_name(a_name)
        , m_color(true)
        , m_stdout_levels(s_def_stdout_levels)
        , m_stderr_levels(s_def_stderr_levels)
    {}

    void colorize(log_level a_ll, bool a_color, std::ostream&, const std::string&);
public:
    static logger_impl_console* create(const char* a_name) {
        return new logger_impl_console(a_name);
    }

    virtual ~logger_impl_console() {}

    const std::string& name()          const { return m_name; }
    int                stdout_levels() const { return m_stdout_levels; }
    int                stderr_levels() const { return m_stderr_levels; }

    /// Dump all settings to stream
    std::ostream& dump(std::ostream& out, const std::string& a_prefix) const;

    bool init(const variant_tree& a_config)
        throw(badarg_error, io_error);

    void log_msg(const logger::msg& a_msg, const char* a_buf, size_t a_size)
         throw  (io_error);
};

} // namespace utxx

#endif

