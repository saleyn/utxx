//----------------------------------------------------------------------------
/// \file  logger_impl_console.cpp
//----------------------------------------------------------------------------
/// \brief Back-end plugin implementating console message writer for the
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
#include <utxx/logger/logger_impl_console.hpp>
#include <utxx/logger/logger_impl.hpp>
#include <iostream>
#include <stdio.h>

namespace utxx {

static logger_impl_mgr::impl_callback_t f = &logger_impl_console::create;
static logger_impl_mgr::registrar reg("console", f);

std::ostream& logger_impl_console::dump(std::ostream& out,
    const std::string& a_prefix) const
{
    out << a_prefix << "logger." << name() << '\n'
        << a_prefix << "    color          = " << to_string(m_color) << '\n'
        << a_prefix << "    stdout-levels  = " << logger::log_levels_to_str(m_stdout_levels) << '\n'
        << a_prefix << "    stderr-levels  = " << logger::log_levels_to_str(m_stderr_levels) << '\n';
    return out;
}

bool logger_impl_console::init(const variant_tree& a_config)
    throw(badarg_error, io_error)
{
    using boost::property_tree::ptree;
    BOOST_ASSERT(this->m_log_mgr);

    ptree::const_assoc_iterator it;
    m_color         = a_config.get("logger.console.color", true);
    std::string s   = a_config.get("logger.console.stdout-levels", "");
    m_stdout_levels = !s.empty() ? logger::parse_log_levels(s) : s_def_stdout_levels;

    s = a_config.get("logger.console.stderr-levels", "");
    m_stderr_levels = !s.empty() ? logger::parse_log_levels(s) : s_def_stderr_levels;

    int all_levels = m_stdout_levels | m_stderr_levels;
    for(int lvl = 0; lvl < logger::NLEVELS; ++lvl) {
        log_level level = logger::signal_slot_to_level(lvl);
        if ((all_levels & static_cast<int>(level)) != 0)
            this->add(level, logger::on_msg_delegate_t::from_method
                    <logger_impl_console, &logger_impl_console::log_msg>(this));
    }
    return true;
}

void logger_impl_console::log_msg(
    const logger::msg& a_msg, const char* a_buf, size_t a_size) throw(io_error)
{
    if (a_msg.level() & m_stdout_levels) {
        bool color = m_color && isatty(fileno(stdout));
        colorize(a_msg.level(), color, std::cout, std::string(a_buf, a_size));
        std::flush(std::cout);
    } else if (a_msg.level() & m_stderr_levels) {
        bool color = m_color && isatty(fileno(stderr));
        colorize(a_msg.level(), color, std::cerr, std::string(a_buf, a_size));
    }
}

void logger_impl_console::colorize
    (log_level a_ll, bool a_color, std::ostream& out, const std::string& a_str)
{
    static const char YELLOW[] = "\E[1;33;40m";
    static const char RED[]    = "\E[1;31;40m";
    static const char MAGENTA[]= "\E[1;35;40m";
    static const char NORMAL[] = "\E[0m";

    if (!a_color || a_ll <  LEVEL_WARNING) out << a_str;
    else if        (a_ll <= LEVEL_WARNING) out << YELLOW  << a_str << NORMAL;
    else if        (a_ll >= LEVEL_FATAL)   out << MAGENTA << a_str << NORMAL;
    else if        (a_ll >= LEVEL_ERROR)   out << RED     << a_str << NORMAL;
}


} // namespace utxx
