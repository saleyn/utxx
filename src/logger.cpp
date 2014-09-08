//----------------------------------------------------------------------------
/// \file  utxx.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of general purpose functions and classes.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-10-04
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

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
#include <vector>
#include <string>
#include <algorithm>
#include <utxx/error.hpp>
#include <utxx/meta.hpp>
#include <utxx/string.hpp>
#include <utxx/timestamp.hpp>
#include <utxx/synch.hpp>
#include <utxx/bits.hpp>
#include <utxx/logger/logger.hpp>
#include <utxx/variant_tree_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/locks.hpp>
#include <stdio.h>


namespace utxx {

std::string logger::log_levels_to_str(int a_levels) noexcept
{
    std::stringstream s;
    bool l_empty = true;
    for (int i = LEVEL_TRACE; i <= LEVEL_LOG; i <<= 1)
        if (i & a_levels) {
            s << (l_empty ? "" : "|")
              << log_level_to_str(static_cast<log_level>(i));
            l_empty = false;
        }
    return s.str();
}

const char* logger::log_level_to_str(log_level level) noexcept
{
    switch (level) {
        case LEVEL_TRACE5   :
        case LEVEL_TRACE4   :
        case LEVEL_TRACE3   :
        case LEVEL_TRACE2   :
        case LEVEL_TRACE1   :
        case LEVEL_TRACE    : return "TRACE";
        case LEVEL_DEBUG    : return "DEBUG";
        case LEVEL_INFO     : return "INFO";
        case LEVEL_WARNING  : return "WARNING";
        case LEVEL_ERROR    : return "ERROR";
        case LEVEL_FATAL    : return "FATAL";
        case LEVEL_ALERT    : return "ALERT";
        case LEVEL_LOG      : return "LOG";
        default             : assert(false);
    }
}

size_t logger::log_level_size(log_level level) noexcept
{
    switch (level) {
        case LEVEL_TRACE5   :
        case LEVEL_TRACE4   :
        case LEVEL_TRACE3   :
        case LEVEL_TRACE2   :
        case LEVEL_TRACE1   :
        case LEVEL_TRACE    :
        case LEVEL_DEBUG    : return 5;
        case LEVEL_INFO     : return 4;
        case LEVEL_WARNING  : return 7;
        case LEVEL_ERROR    :
        case LEVEL_FATAL    :
        case LEVEL_ALERT    : return 5;
        case LEVEL_LOG      : return 3;
        default             : assert(false);
    }
}

int logger::level_to_signal_slot(log_level level) noexcept
{
    switch (level) {
        case LEVEL_TRACE5   :
        case LEVEL_TRACE4   :
        case LEVEL_TRACE3   :
        case LEVEL_TRACE2   :
        case LEVEL_TRACE1   :
        case LEVEL_TRACE    : return 0;
        case LEVEL_DEBUG    : return 1;
        case LEVEL_INFO     : return 2;
        case LEVEL_WARNING  : return 3;
        case LEVEL_ERROR    : return 4;
        case LEVEL_FATAL    : return 5;
        case LEVEL_ALERT    : return 6;
        default             : assert(false);
    }
}

log_level logger::signal_slot_to_level(int slot) noexcept
{
    switch (slot) {
        case 0: return LEVEL_TRACE;
        case 1: return LEVEL_DEBUG;
        case 2: return LEVEL_INFO;
        case 3: return LEVEL_WARNING;
        case 4: return LEVEL_ERROR;
        case 5: return LEVEL_FATAL;
        case 6: return LEVEL_ALERT;
        default: assert(false);
    }
}

const char* logger::default_log_levels = "INFO|WARNING|ERROR|ALERT|FATAL";

void logger_impl_mgr::register_impl(
    const char* config_name, boost::function<logger_impl*(const char*)>& factory)
{
    boost::lock_guard<boost::mutex> guard(m_mutex);
    m_implementations[config_name] = factory;
}

void logger_impl_mgr::unregister_impl(const char* config_name)
{
    BOOST_ASSERT(config_name);
    boost::lock_guard<boost::mutex> guard(m_mutex);
    m_implementations.erase(config_name);
}

logger_impl_mgr::impl_callback_t*
logger_impl_mgr::get_impl(const char* config_name) {
    boost::lock_guard<boost::mutex> guard(m_mutex);
    impl_map_t::iterator it = m_implementations.find(config_name);
    return (it != m_implementations.end()) ? &it->second : NULL;
}

void logger::init(const char* filename)
{
    config_tree pt;
    read_config(filename, pt);
    init(pt);
}

void logger::init(const config_tree& config)
{
    finalize();

    try {
        m_show_location = config.get<bool>       ("logger.show-location", m_show_location);
        m_show_ident    = config.get<bool>       ("logger.show-ident",    m_show_ident);
        m_ident         = config.get<std::string>("logger.ident",         m_ident);
        std::string ts  = config.get<std::string>("logger.timestamp",     "time-usec");
        m_timestamp_type= parse_stamp_type(ts);
        std::string ls  = config.get<std::string>("logger.min-level-filter", "info");
        set_min_level_filter(static_cast<log_level>(parse_log_levels(ls)));

        if ((int)m_timestamp_type < 0)
            throw std::runtime_error("Invalid timestamp type: " + ts);

        //logger_impl::msg_info info(NULL, 0);
        //query_timestamp(info);

        logger_impl_mgr& lim = logger_impl_mgr::instance();

        boost::lock_guard<boost::mutex> guard(lim.mutex());

        // Check the list of registered implementations. If corresponding
        // configuration section is found, initialize the implementation.
        for(logger_impl_mgr::impl_map_t::iterator it=lim.implementations().begin();
            it != lim.implementations().end();
            ++it)
        {
            std::string path = std::string("logger.") + it->first;
            if (config.get_child_optional(path)) {
                // Determine if implementation of this type is already
                // registered with the logger
                bool found = false;
                for(implementations_vector::iterator
                        im = m_implementations.begin(), iend = m_implementations.end();
                        im != iend; ++im)
                    if (it->first == (*im)->name()) {
                        found = true;
                        break;
                    }

                if (found)
                    throw badarg_error("Implementation '", it->first,
                                       "' is already registered with the logger!");

                // A call to it->second() creates a logger_impl* pointer.
                // We need to call implementation's init function that may throw,
                // so use RAII to guarantee proper cleanup.
                logger_impl_mgr::impl_callback_t& f = it->second;
                impl impl( f(it->first.c_str()) );
                impl->set_log_mgr(this);
                impl->init(config);
                m_implementations.push_back(impl);
            }
        }
    } catch (std::runtime_error& e) {
        if (m_error)
            m_error(e.what());
        else
            throw;
    }
}

void logger::finalize()
{
    for(auto& impl : m_implementations)
        impl.reset();
    m_implementations.clear();
}

void logger::delete_impl(const std::string& a_name)
{
    boost::lock_guard<boost::mutex> guard(logger_impl_mgr::instance().mutex());
    for (implementations_vector::iterator
            it = m_implementations.begin(), end = m_implementations.end();
            it != end; ++it)
        if ((*it)->name() == a_name)
            m_implementations.erase(it);
}

int logger::parse_log_levels(const std::string& a_levels)
    throw(std::runtime_error)
{
    std::vector<std::string> str_levels;
    boost::split(str_levels, a_levels, boost::is_any_of(" |,;"),
        boost::algorithm::token_compress_on);
    int result = LEVEL_NONE;
    for (std::vector<std::string>::iterator it=str_levels.begin();
        it != str_levels.end(); ++it) {
        boost::to_upper(*it);
        if (*it == "WIRE")
            *it = "DEBUG";  // Backward compatibility
        if (*it == "NONE" || *it == "FALSE") {
            result  = LEVEL_NONE;
            break;
        }
        else if (*it == "TRACE")   result |= LEVEL_TRACE;
        else if (*it == "TRACE1")  result |= LEVEL_TRACE | LEVEL_TRACE1;
        else if (*it == "TRACE2")  result |= LEVEL_TRACE | LEVEL_TRACE2;
        else if (*it == "TRACE3")  result |= LEVEL_TRACE | LEVEL_TRACE3;
        else if (*it == "TRACE4")  result |= LEVEL_TRACE | LEVEL_TRACE4;
        else if (*it == "TRACE5")  result |= LEVEL_TRACE | LEVEL_TRACE5;
        else if (*it == "DEBUG")   result |= LEVEL_DEBUG;
        else if (*it == "INFO")    result |= LEVEL_INFO;
        else if (*it == "WARNING") result |= LEVEL_WARNING;
        else if (*it == "ERROR")   result |= LEVEL_ERROR;
        else if (*it == "FATAL")   result |= LEVEL_FATAL;
        else if (*it == "ALERT")   result |= LEVEL_ALERT;
        else throw std::runtime_error(std::string("Invalid log level: ") + *it);
    }
    return result;
}

void logger::set_level_filter(log_level a_level) {
    m_level_filter = static_cast<int>(a_level);
}

void logger::set_min_level_filter(log_level a_level) {
    uint32_t n = (uint32_t)a_level ? (1u << bits::bit_scan_forward(a_level)) - 1 : 0u;
    m_level_filter = static_cast<uint32_t>(~n);
}

int logger::add_msg_logger(log_level level, on_msg_delegate_t subscriber)
{
    return m_sig_msg[level_to_signal_slot(level)].connect(subscriber);
}

int logger::add_bin_logger(on_bin_delegate_t subscriber)
{
    return m_sig_bin.connect(subscriber);
}

void logger::remove_msg_logger(log_level a_lvl, int a_id)
{
    m_sig_msg[level_to_signal_slot(a_lvl)].disconnect(a_id);
}

/// To be called by <logger_impl> child to unregister a delegate
void logger::remove_bin_logger(int a_id)
{
    m_sig_bin.disconnect(a_id);
}

std::ostream& logger::dump(std::ostream& out) const
{
    std::stringstream s;
    s   << "Logger settings:\n"
        << "    level-filter   = " << log_levels_to_str(m_level_filter) << '\n'
        << "    show-location  = " << (m_show_location ? "true" : "false") << '\n'
        << "    show-ident     = " << (m_show_ident    ? "true" : "false") << '\n'
        << "    ident          = " << m_ident << '\n'
        << "    timestamp-type = " << to_string(m_timestamp_type) << '\n';

    // Check the list of registered implementations. If corresponding
    // configuration section is found, initialize the implementation.
    for(implementations_vector::const_iterator it = m_implementations.begin();
            it != m_implementations.end(); ++it)
        (*it)->dump(s, "        ");

    return out << s.str();
}

} // namespace utxx
