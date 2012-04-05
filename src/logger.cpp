//----------------------------------------------------------------------------
/// \file  util.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of general purpose functions and classes.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-10-04
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the UDPR project.

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
#include <util/error.hpp>
#include <util/meta.hpp>
#include <util/string.hpp>
#include <util/synch.hpp>
#include <util/bits.hpp>
#include <util/logger/logger.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/locks.hpp>
#include <boost/property_tree/info_parser.hpp>
//#include <boost/property_tree/json_parser.hpp>    // FIXME
//#include <boost/property_tree/xml_parser.hpp>     // FIXME
#include <stdio.h>


namespace util {

    namespace {
        static const char* s_timestamp_types[] = {
            "NO_TIMESTAMP", "TIME", "TIME_WITH_MSEC", "TIME_WITH_USEC",
            "DATE_TIME", "DATE_TIME_WITH_MSEC", "DATE_TIME_WITH_USEC"
        };

        stamp_type str_to_timestamp_type(const std::string& a_str) {
            std::string ts(a_str);
            boost::to_upper(ts);
            BOOST_STATIC_ASSERT(
                sizeof(s_timestamp_types)/sizeof(s_timestamp_types[0]) == DATE_TIME_WITH_USEC + 1);
            return find_index<stamp_type>(s_timestamp_types, ts);
        }

        const char* timestamp_type_to_str(stamp_type a_type) {
            BOOST_ASSERT(a_type < sizeof(s_timestamp_types)/sizeof(s_timestamp_types[0]));
            return s_timestamp_types[a_type];
        }
    }

std::string logger::log_levels_to_str(int a_levels)
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

const char* logger::log_level_to_str(log_level level)
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
        default             : return "UNDEFINED";
    }
}       

int logger::level_to_signal_slot(log_level level) throw(badarg_error)
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
        default             : throw badarg_error("Unsupported level", (int)level);
    }
}

log_level logger::signal_slot_to_level(int slot) throw(badarg_error)
{
    switch (slot) {
        case 0: return LEVEL_TRACE;
        case 1: return LEVEL_DEBUG;
        case 2: return LEVEL_INFO;
        case 3: return LEVEL_WARNING;
        case 4: return LEVEL_ERROR;
        case 5: return LEVEL_FATAL;
        case 6: return LEVEL_ALERT;
        default: throw badarg_error("Unsupported slot", slot);
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

void logger::init(const char* filename, init_file_type type)
{
    variant_tree pt;
    switch (type) {
        case INFO_FILE: read_info(filename, pt); break;
        case JSON_FILE: throw std::runtime_error("JSON config not implemented!"); break;
                        //read_json(filename, pt); break;
        case XML_FILE:  throw std::runtime_error("XML config not implemented!"); break;
                        //read_xml (filename, pt); break;  // FIXME: This doesn't compile
    }
    init(pt);
}

void logger::init(const variant_tree& config)
{
    finalize();

    try {
        m_show_location = config.get<bool>       ("logger.show_location", m_show_location);
        m_show_ident    = config.get<bool>       ("logger.show_ident",    m_show_ident);
        m_ident         = config.get<std::string>("logger.ident",         m_ident);
        std::string ts  = config.get<std::string>("logger.timestamp",     "time_with_usec");
        m_timestamp_type= str_to_timestamp_type(ts);
        std::string ls  = config.get<std::string>("logger.min_level_filter", "info");
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
                // A caLEVEL_ to it->second() creates a logger_impl* pointer.
                // We need to caLEVEL_ implementation's init function that may throw,
                // so use RAII to guarantee proper cleanup.
                logger_impl_mgr::impl_callback_t& f = it->second;
                std::auto_ptr<logger_impl> impl( f(it->first.c_str()) );
                impl->set_log_mgr(this);
                impl->init(config);
                m_implementations.push_back(&*impl);
                impl.release();
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
    for(std::vector<logger_impl*>::reverse_iterator it=m_implementations.rbegin();
        it != m_implementations.rend(); ++it) {
        delete *it;
    }
    m_implementations.clear();
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

void logger::add_msg_logger(log_level level,
    event_binder<on_msg_delegate_t>& binder, on_msg_delegate_t subscriber)
{
    binder.bind(m_sig_msg[level_to_signal_slot(level)], subscriber);
}

void logger::add_bin_logger(event_binder<on_bin_delegate_t>& binder, on_bin_delegate_t subscriber)
{
    binder.bind(m_sig_bin, subscriber);
}

std::ostream& logger::dump(std::ostream& out) const
{
    std::stringstream s;
    s   << "Logger settings:\n"
        << "    level_filter   = " << log_levels_to_str(m_level_filter) << '\n'
        << "    show_location  = " << (m_show_location ? "true" : "false") << '\n'
        << "    show_ident     = " << (m_show_ident    ? "true" : "false") << '\n'
        << "    ident          = " << m_ident << '\n'
        << "    timestamp_type = " << timestamp_type_to_str(m_timestamp_type) << '\n';

    // Check the list of registered implementations. If corresponding
    // configuration section is found, initialize the implementation.
    for(implementations_vector::const_iterator it = m_implementations.begin();
            it != m_implementations.end(); ++it)
        (*it)->dump(s, "        ");

    return out << s.str();
}

} // namespace util
