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
#include <mutex>
#include <utxx/error.hpp>
#include <utxx/meta.hpp>
#include <utxx/string.hpp>
#include <utxx/timestamp.hpp>
#include <utxx/compiler_hints.hpp>
#include <utxx/synch.hpp>
#include <utxx/bits.hpp>
#include <utxx/logger/logger.hpp>
#include <utxx/logger/logger_impl.hpp>
#include <utxx/variant_tree_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/thread/locks.hpp>
#include <stdio.h>

#if DEBUG_ASYNC_LOGGER == 2
#   define ASYNC_DEBUG_TRACE(x) do { printf x; fflush(stdout); } while(0)
#   define ASYNC_TRACE(x)
#elif defined(DEBUG_ASYNC_LOGGER)
#   define ASYNC_TRACE(x) do { printf x; fflush(stdout); } while(0)
#   define ASYNC_DEBUG_TRACE(x) ASYNC_TRACE(x)
#else
#   define ASYNC_TRACE(x)
#   define ASYNC_DEBUG_TRACE(x)
#endif

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

void logger::add_macro(const std::string& a_macro, const std::string& a_value)
{
    m_macro_var_map[a_macro] = a_value;
}

std::string logger::replace_macros(const std::string& a_value) const
{
    using namespace boost::posix_time;
    using namespace boost::xpressive;
    sregex re = "$(" >> (s1 = +_w) >> ')';
    auto replace = [this](const smatch& what) {
        auto it = this->m_macro_var_map.find(what[1].str());
        return it == this->m_macro_var_map.end()
                ? what[1].str() : it->second;
    };
    return regex_replace(a_value, re, replace);
}

void logger::init(const char* filename)
{
    config_tree pt;
    read_config(filename, pt);
    init(pt);
}

void logger::init(const config_tree& config)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    do_finalize();

    try {
        m_show_location  = config.get<bool>       ("logger.show-location", m_show_location);
        m_show_ident     = config.get<bool>       ("logger.show-ident",    m_show_ident);
        m_ident          = config.get<std::string>("logger.ident",         m_ident);
        std::string ts   = config.get<std::string>("logger.timestamp",     "time-usec");
        m_timestamp_type = parse_stamp_type(ts);
        std::string ls   = config.get<std::string>("logger.min-level-filter", "info");
        set_min_level_filter(static_cast<log_level>(parse_log_levels(ls)));
        long timeout_ms  = config.get<int>        ("logger.wait-timeout-ms", 2000);
        m_wait_timeout   = timespec{timeout_ms / 1000, timeout_ms % 1000 * 1000000000L};

        if ((int)m_timestamp_type < 0)
            throw std::runtime_error("Invalid timestamp type: " + ts);

        //logger_impl::msg_info info(NULL, 0);
        //query_timestamp(info);

        logger_impl_mgr& lim = logger_impl_mgr::instance();

        std::lock_guard<std::mutex> guard(lim.mutex());

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
                m_implementations.emplace_back( f(it->first.c_str()) );
                auto& i = m_implementations.back();
                i->set_log_mgr(this);
                i->init(config);
            }
        }

        m_abort = false;

        m_thread.reset(new std::thread([this]() { this->run(); }));

    } catch (std::runtime_error& e) {
        if (m_error)
            m_error(e.what());
        else
            throw;
    }
}

void logger::run()
{
    int event_val = 1;
    while (!m_abort)
    {
        event_val        = m_event.value();
        //wakeup_result rc = wakeup_result::TIMEDOUT;

        while (!m_abort && m_queue.empty()) {
            m_event.wait(&m_wait_timeout, &event_val);

            ASYNC_DEBUG_TRACE(
                ("  %s LOGGER awakened (res=%s, val=%d, futex=%d), abort=%d, head=%s\n",
                 timestamp::to_string().c_str(), to_string(rc).c_str(),
                 event_val, m_event.value(), m_abort,
                 m_queue.empty() ? "empty" : "data")
            );
        }

        // CPU-friendly spin for 250us
        if (m_queue.empty()) {
            time_val deadline(rel_time(0, 250));
            while (m_queue.empty()) {
                if (m_abort)
                    goto DONE;
                if (now_utc() > deadline)
                    break;
                sched_yield();
            }
        }

        // Get all pending items from the queue
        for (auto* item = m_queue.pop_all(), *next=item; item; item = next) {
            next = item->next();
            do_log(item->data());

            m_queue.free(item);
            item = next;
        }
    }

DONE:
    const msg s_msg
        (LEVEL_INFO, "",
         [](const char* pfx, size_t psz, const char* sfx, size_t ssz)
            { return std::string(pfx, psz) +
                     std::string("Logger thread finished") +
                     std::string(sfx, ssz); },
         UTXX_FILE_SRC_LOCATION);
    do_log(s_msg);
}

void logger::finalize()
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_abort = true;
    if (m_thread)
        m_thread->join();
    m_thread.reset();
    do_finalize();
}

void logger::do_finalize()
{
    m_abort = true;

    for(auto& impl : m_implementations)
        impl.reset();
    m_implementations.clear();
}

char* logger::
format_header(const logger::msg& a_msg, char* a_buf, const char* a_end)
{
    // Message mormat: Timestamp|Level|Ident|Category|Message|File:Line
    // Write everything up to Message to the m_data:

    // Write Timestamp
    char*  p = a_buf;
    p   += timestamp::format(timestamp_type(), a_msg.m_timestamp, p, a_end - p);
    *p++ = '|';
    // Write Level
    *p++ = logger::log_level_to_str(a_msg.m_level)[0];
    *p++ = '|';
    if (show_ident())
        p = stpncpy(p, ident().c_str(), ident().size());
    *p++ = '|';
    if (!a_msg.m_category.empty())
        p = stpncpy(p, a_msg.m_category.c_str(), a_msg.m_category.size());
    *p++ = '|';
    return p;
}

char* logger::
format_footer(const logger::msg& a_msg, char* a_buf, const char* a_end)
{
    char* p = a_buf;

    // Format the message in the form:
    // Timestamp|Level|Ident|Category|Message|File:Line\n
    if (a_msg.src_loc_len() && show_location() &&
        likely(a_buf + a_msg.src_loc_len() + 2 < a_end))
    {
        static const char s_sep =
            boost::filesystem::path("/").native().c_str()[0];
        if (*(p-1) == '\n')
            p--;

        *p++ =  '|';

        const char* q = strrchr(a_msg.src_location(), s_sep);
        q = q ? q+1 : a_msg.src_location();
        // extra byte for possible '\n'
        auto len = std::min<size_t>
            (a_end - a_buf, a_msg.src_location() + a_msg.src_loc_len() - q + 1);
        p = stpncpy(p, a_msg.src_location(), len);
        *p++ = '\n';
    } else
        // We reached the end of the streaming sequence:
        // log_msg_info lmi; lmi << a << b << c;
        if (*p != '\n') *p++ = '\n';

    *p++ = '\0'; // Writes terminating '\0'
    return p;
}

void logger::do_log(const logger::msg& a_msg) {
    try {
        switch (a_msg.m_type) {
            case payload_t::CHAR_FUN: {
                assert(a_msg.m_fun.cf);
                char  buf[4096];
                auto* end = buf + sizeof(buf);
                char*   p = format_header(a_msg, buf,  end);
                int     n = (a_msg.m_fun.cf)(p,  end - p);
                        p = format_footer(a_msg, p+n,  end);
                m_sig_slot[level_to_signal_slot(a_msg.level())](
                    on_msg_delegate_t::invoker_type(a_msg, buf, p - buf));
                break;
            }
            case payload_t::STR_FUN: {
                assert(a_msg.m_fun.cf);
                char  pfx[256], sfx[256];
                char*   p = format_header(a_msg, pfx, pfx + sizeof(pfx));
                char*   q = format_footer(a_msg, sfx, sfx + sizeof(sfx));
                auto  res = (a_msg.m_fun.sf)(pfx, p - pfx, sfx, q - sfx);
                m_sig_slot[level_to_signal_slot(a_msg.level())](
                    on_msg_delegate_t::invoker_type(a_msg, res.c_str(), res.size()));
                break;
            }
        }
    } catch (std::runtime_error& e) {
        if (m_error)
            m_error(e.what());
        else
            throw;
    }
}

void logger::delete_impl(const std::string& a_name)
{
    std::lock_guard<std::mutex> guard(logger_impl_mgr::instance().mutex());
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

int logger::add(log_level level, on_msg_delegate_t subscriber)
{
    return m_sig_slot[level_to_signal_slot(level)].connect(subscriber);
}

void logger::remove(log_level a_lvl, int a_id)
{
    m_sig_slot[level_to_signal_slot(a_lvl)].disconnect(a_id);
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

//-----------------------------------------------------------------------------
// logger_impl
//-----------------------------------------------------------------------------
logger_impl::logger_impl()
    : m_log_mgr(NULL)
{
    for (int i=0; i < logger::NLEVELS; ++i)
        m_msg_sink_id[i] = -1;
}

logger_impl::~logger_impl()
{
    if (m_log_mgr) {
        for (int i=0; i < logger::NLEVELS; ++i)
            if (m_msg_sink_id[i] != -1) {
                log_level level = logger::signal_slot_to_level(i);
                m_log_mgr->remove(level, m_msg_sink_id[i]);
                m_msg_sink_id[i] = -1;
            }
    }
}

void logger_impl::add(log_level level, logger::on_msg_delegate_t subscriber)
{
    m_msg_sink_id[logger::level_to_signal_slot(level)] =
        m_log_mgr->add(level, subscriber);
}

} // namespace utxx
