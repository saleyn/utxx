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
#include <utxx/logger/logger_crash_handler.hpp>
#include <utxx/logger/logger_impl.hpp>
#include <utxx/signal_block.hpp>
#include <utxx/variant_tree_parser.hpp>
#include <utxx/logger/generated/logger_options.generated.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/thread/locks.hpp>
#include <stdio.h>
#include <stdlib.h>

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

std::string logger::log_levels_to_str(uint32_t a_levels) noexcept
{
    std::stringstream s;
    bool l_empty = true;
    for (uint32_t i = 1; i <= LEVEL_LOG; i <<= 1) {
        auto level = i < LEVEL_TRACE ? (i | LEVEL_TRACE) : i;
        if  ((level & a_levels) == level) {
            s << (l_empty ? "" : "|")
              << log_level_to_string(static_cast<log_level>(level), false);
            l_empty = false;
        }
    }
    return s.str();
}

const std::string& logger::log_level_to_abbrev(log_level level) noexcept
{
    static const std::string s_levels[] = {
        "T", "D", "I", "N", "W", "E", "F", "A", "L", " "
    };

    switch (level) {
        case LEVEL_TRACE5   :
        case LEVEL_TRACE4   :
        case LEVEL_TRACE3   :
        case LEVEL_TRACE2   :
        case LEVEL_TRACE1   :
        case LEVEL_TRACE    : return s_levels[0];
        case LEVEL_DEBUG    : return s_levels[1];
        case LEVEL_INFO     : return s_levels[2];
        case LEVEL_NOTICE   : return s_levels[3];
        case LEVEL_WARNING  : return s_levels[4];
        case LEVEL_ERROR    : return s_levels[5];
        case LEVEL_FATAL    : return s_levels[6];
        case LEVEL_ALERT    : return s_levels[7];
        case LEVEL_LOG      : return s_levels[8];
        default             : break;
    }
    assert(false);
    return s_levels[9];
}

const std::string& logger::log_level_to_string(log_level lvl, bool merge_trace) noexcept
{
    static const std::string s_traces[] = {
        "TRACE1", "TRACE2", "TRACE3", "TRACE4", "TRACE5"
    };

    static const std::string s_levels[] = {
        "TRACE",   "DEBUG", "INFO",  "NOTICE",
        "WARNING", "ERROR", "FATAL", "ALERT", "LOG", "NONE"
    };

    switch (lvl) {
        case LEVEL_TRACE5   : return merge_trace ? s_levels[0] : s_traces[4];
        case LEVEL_TRACE4   : return merge_trace ? s_levels[0] : s_traces[3];
        case LEVEL_TRACE3   : return merge_trace ? s_levels[0] : s_traces[2];
        case LEVEL_TRACE2   : return merge_trace ? s_levels[0] : s_traces[1];
        case LEVEL_TRACE1   : return merge_trace ? s_levels[0] : s_traces[0];
        case LEVEL_TRACE    : return s_levels[0];
        case LEVEL_DEBUG    : return s_levels[1];
        case LEVEL_INFO     : return s_levels[2];
        case LEVEL_NOTICE   : return s_levels[3];
        case LEVEL_WARNING  : return s_levels[4];
        case LEVEL_ERROR    : return s_levels[5];
        case LEVEL_FATAL    : return s_levels[6];
        case LEVEL_ALERT    : return s_levels[7];
        case LEVEL_LOG      : return s_levels[8];
        default             : break;
    }
    assert(false);
    return s_levels[9];
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
        case LEVEL_NOTICE   : return 6;
        case LEVEL_WARNING  : return 7;
        case LEVEL_ERROR    :
        case LEVEL_FATAL    :
        case LEVEL_ALERT    : return 5;
        case LEVEL_LOG      : return 3;
        default             : break;
    }
    assert(false);
    return 0;
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
        case LEVEL_NOTICE   : return 3;
        case LEVEL_WARNING  : return 4;
        case LEVEL_ERROR    : return 5;
        case LEVEL_FATAL    : return 6;
        case LEVEL_ALERT    : return 7;
        case LEVEL_LOG      : return 8;
        default             : break;
    }
    assert(false);
    return 0;

}

log_level logger::signal_slot_to_level(int slot) noexcept
{
    switch (slot) {
        case 0: return LEVEL_TRACE;
        case 1: return LEVEL_DEBUG;
        case 2: return LEVEL_INFO;
        case 3: return LEVEL_NOTICE;
        case 4: return LEVEL_WARNING;
        case 5: return LEVEL_ERROR;
        case 6: return LEVEL_FATAL;
        case 7: return LEVEL_ALERT;
        default: break;
    }
    assert(false);
    return LEVEL_NONE;
}

const char* logger::default_log_levels = "INFO|NOTICE|WARNING|ERROR|ALERT|FATAL";
std::atomic<sigset_t*> logger::m_crash_sigset;

void logger::add_macro(const std::string& a_macro, const std::string& a_value)
{
    m_macro_var_map[a_macro] = a_value;
}

std::string logger::replace_macros(const std::string& a_value) const
{
    using namespace boost::posix_time;
    using namespace boost::xpressive;
    sregex re = "{{" >> (s1 = +_w) >> "}}";
    auto replace = [this](const smatch& what) {
        auto it = this->m_macro_var_map.find(what[1].str());
        return it == this->m_macro_var_map.end()
                ? what[1].str() : it->second;
    };
    return regex_replace(a_value, re, replace);
}

void logger::init(const char* filename, const sigset_t* a_ignore_signals)
{
    config_tree pt;
    read_config(filename, pt);
    init(pt);
}

void logger::init(const config_tree& a_cfg, const sigset_t* a_ignore_signals)
{
    if (m_initialized)
        throw std::runtime_error("Logger already initialized!");

    std::lock_guard<std::mutex> guard(m_mutex);
    do_finalize();

    try {
        m_show_location  = a_cfg.get<bool>       ("logger.show-location", m_show_location);
        m_show_fun_namespaces = a_cfg.get<int>   ("logger.show-fun-namespaces",
                                                  m_show_fun_namespaces);
        m_show_category  = a_cfg.get<bool>       ("logger.show-category", m_show_category);
        m_show_ident     = a_cfg.get<bool>       ("logger.show-ident",    m_show_ident);
        m_show_thread    = a_cfg.get<bool>       ("logger.show-thread",   m_show_thread);
        m_fatal_kill_signal = a_cfg.get<int>       ("logger.fatal-kill-signal",   m_fatal_kill_signal);
        m_ident          = a_cfg.get<std::string>("logger.ident",         m_ident);
        m_ident          = replace_macros(m_ident);
        std::string ts   = a_cfg.get<std::string>("logger.timestamp",     "time-usec");
        m_timestamp_type = parse_stamp_type(ts);
        std::string levs = a_cfg.get<std::string>("logger.levels", "");
        if (!levs.empty())
            set_level_filter(static_cast<log_level>(parse_log_levels(levs)));
        std::string ls   = a_cfg.get<std::string>("logger.min-level-filter", "info");
        if (!levs.empty() && !ls.empty())
            std::runtime_error
                ("Either 'levels' or 'min-level-filter' option is permitted!");
        set_min_level_filter(parse_log_level(ls));
        long timeout_ms  = a_cfg.get<int>        ("logger.wait-timeout-ms", 1000);
        m_wait_timeout   = timespec{timeout_ms / 1000, timeout_ms % 1000 * 1000000L};
        m_sched_yield_us = a_cfg.get<long>       ("logger.sched-yield-us", -1);
        m_silent_finish  = a_cfg.get<bool>       ("logger.silent-finish",  false);

        if ((int)m_timestamp_type < 0)
            throw std::runtime_error("Invalid timestamp type: " + ts);

        // Install crash signal handlers
        // (SIGABRT, SIGFPE, SIGILL, SIGSEGV, SIGTERM)
        if (a_cfg.get("logger.handle-crash-signals", true)) {
            sigset_t sset = sig_members_parse
                (a_cfg.get("logger.handle-crash-signals.signals",""), UTXX_SRC);

            // Remove signals from the sset that are handled externally
            if (a_ignore_signals)
                for (uint i=1; i < sig_names_count(); ++i)
                    if (sigismember(a_ignore_signals, i))
                        sigdelset(&sset, i);

            if (install_sighandler(true, &sset)) {
                auto old = m_crash_sigset.exchange(new sigset_t(sset));
                if (!old)
                    delete old;
            }
        }

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
            if (a_cfg.get_child_optional(path)) {
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
                i->init(a_cfg);
            }
        }

        m_initialized = true;
        m_abort       = false;

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
    if (m_on_before_run)
        m_on_before_run();

    if (!m_ident.empty())
        pthread_setname_np(pthread_self(), m_ident.c_str());

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

        // When running with maximum priority, occasionally excessive use of
        // sched_yield may use to system slowdown, so this option is
        // configurable by m_sched_yield_us:
        if (m_queue.empty() && m_sched_yield_us >= 0) {
            time_val deadline(rel_time(0, m_sched_yield_us));
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

            try   { dolog_msg(item->data()); }
            catch ( std::exception const& e  )
            {
                // Unhandled error writing data to some destination
                // Print error report to stderr (can't do anything better --
                // the error happened in the m_on_error callback!)
                const msg msg(LEVEL_INFO, "",
                              std::string("Fatal exception in logger"),
                              UTXX_LOG_SRCINFO);
                detail::basic_buffered_print<1024> buf;
                char  pfx[256], sfx[256];
                char* p = format_header(msg, pfx, pfx + sizeof(pfx));
                char* q = format_footer(msg, sfx, sfx + sizeof(sfx));
                auto ps = p - pfx;
                auto qs = q - sfx;
                buf.reserve(msg.m_fun.str.size() + ps + qs + 1);
                buf.sprint(pfx, ps);
                buf.print(msg.m_fun.str);
                buf.sprint(sfx, qs);
                std::cerr << buf.str() << std::endl;

                m_abort = true;

                // TODO: implement attempt to store transient messages to some
                // other medium

                // Free all pending messages
                while (item) {
                    m_queue.free(item);
                    item = next;
                    next = item->next();
                }

                goto DONE;
            }

            m_queue.free(item);
            item = next;
        }
    }

DONE:
    if (!m_silent_finish) {
        const msg msg(LEVEL_INFO, "", std::string("Logger thread finished"),
                      UTXX_LOG_SRCINFO);
        try { dolog_msg(msg); } catch (...) {}
    }

    if (m_on_after_run)
        m_on_after_run();
}

void logger::finalize()
{
    if (!m_initialized)
        return;

    std::lock_guard<std::mutex> g(m_mutex);
    m_abort = true;
    if (m_thread)
        m_thread->join();
    m_thread.reset();
    do_finalize();
    m_abort = false;
    m_initialized = false;
}

void logger::do_finalize()
{
    m_abort = true;

    for(auto& impl : m_implementations)
        impl.reset();
    m_implementations.clear();

    auto sset = m_crash_sigset.exchange(nullptr);
    if  (sset)
        delete sset;
}

char* logger::
format_header(const logger::msg& a_msg, char* a_buf, const char* a_end)
{
    // Message mormat: Timestamp|Level|Ident|Category|Message|File:Line FunName
    // Write everything up to Message to the m_data:
    char*  p = a_buf;

    // Write Timestamp
    if (timestamp_type() != stamp_type::NO_TIMESTAMP) {
        p   += timestamp::format(timestamp_type(), a_msg.m_timestamp, p, a_end - p);
        *p++ = '|';
    }
    // Write Level
    *p++ = logger::log_level_to_str(a_msg.m_level)[0];
    *p++ = '|';
    if (show_ident()) {
        p = stpncpy(p, ident().c_str(), ident().size());
        *p++ = '|';
    }
    if (show_thread()) {
        if (a_msg.m_thread_name[0] == '\0') {
            char* q = const_cast<char*>(a_msg.m_thread_name);
            itoa(a_msg.m_thread_id, q, 10);
        }
        p = stpcpy(p, a_msg.m_thread_name);
        *p++ = '|';
    }
    if (show_category()) {
        if (!a_msg.m_category.empty())
            p = stpncpy(p, a_msg.m_category.c_str(), a_msg.m_category.size());
        *p++ = '|';
    }

    return p;
}

char* logger::
format_footer(const logger::msg& a_msg, char* a_buf, const char* a_end)
{
    char* p = a_buf;
    auto  n = a_msg.src_loc_len() + 2;

    // Format the message in the form:
    // Timestamp|Level|Ident|Category|Message|File:Line FunName\n
    if (a_msg.src_loc_len() && show_location() && likely(a_buf + n < a_end)) {
        if (*(p-1) == '\n') p--;
        *p++ = ' ';
        *p++ = '[';

        p = src_info::to_string(p, a_end - p,
                a_msg.src_location(), a_msg.src_loc_len(),
                a_msg.src_fun_name(), a_msg.src_fun_len(),
                show_fun_namespaces());
        *p++ = ']';
    }

    // We reached the end of the streaming sequence:
    // log_msg_info lmi; lmi << a << b << c;
    if (*p != '\n') *p++ = '\n';
    else p++;

    *p = '\0'; // Writes terminating '\0' (note: p is not incremented)
    return p;
}

void logger::dolog_msg(const logger::msg& a_msg) {
    try {
        switch (a_msg.m_type) {
            case payload_t::CHAR_FUN: {
                assert(a_msg.m_fun.cf);
                char  buf[4096];
                auto* end = buf + sizeof(buf);
                char*   p = format_header(a_msg, buf,  end);
                int     n = (a_msg.m_fun.cf)(p,  end - p);
                if (p[n-1] == '\n') --p;
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
            case payload_t::STR: {
                detail::basic_buffered_print<1024> buf;
                char  pfx[256], sfx[256];
                char* p = format_header(a_msg, pfx, pfx + sizeof(pfx));
                char* q = format_footer(a_msg, sfx, sfx + sizeof(sfx));
                auto ps = p - pfx;
                auto qs = q - sfx;
                buf.reserve(a_msg.m_fun.str.size() + ps + qs + 1);
                buf.sprint(pfx, ps);
                auto& s = a_msg.m_fun.str;
                // Remove trailing new lines
                auto sz = s.size();
                while (sz && s[sz-1] == '\n') --sz;
                buf.sprint(s.c_str(), sz);
                buf.sprint(sfx, qs);
                m_sig_slot[level_to_signal_slot(a_msg.level())](
                    on_msg_delegate_t::invoker_type(a_msg, buf.str(), buf.size()));

                if (fatal_kill_signal() && a_msg.level() == LEVEL_FATAL)
                    dolog_fatal_msg(buf.str());

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

void logger::dolog_fatal_msg(char* buf)
{
    // Expecting a Rethrowing signal string of
    // this format
    // (signal number)

    int   signum    = fatal_kill_signal(); //DEFAULT SIGNAL SIGABRT
    char* signo_str = strstr(buf, "(");

    if (signo_str) {
        signo_str++;
        signum = atoi(signo_str);

        if (signum) {
            std::cerr << "logger exiting after receiving fatal event " << signum
                      << std::endl;
            detail::exit_with_default_sighandler(signum);
            return;
        }
    }
    std::cerr << "logger exiting from unknown fatal event" << signum << std::endl;
    detail::exit_with_default_sighandler(fatal_kill_signal());
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

const logger_impl* logger::get_impl(const std::string& a_name) const
{
    std::lock_guard<std::mutex> guard(logger_impl_mgr::instance().mutex());
    for (implementations_vector::const_iterator
            it = m_implementations.cbegin(), end = m_implementations.cend();
            it != end; ++it)
        if ((*it)->name() == a_name)
            return it->get();
    return nullptr;
}

int logger::parse_log_levels(const std::string& a_levels)
    throw(std::runtime_error)
{
    std::vector<std::string> str_levels;
    boost::split(str_levels, a_levels, boost::is_any_of(" |,;"),
        boost::algorithm::token_compress_on);
    int result = LEVEL_NONE;
    for (std::vector<std::string>::iterator it=str_levels.begin();
        it != str_levels.end(); ++it)
        result |= parse_log_level(*it);
    return result;
}

log_level logger::parse_log_level(const std::string& a_level)
    throw(std::runtime_error)
{
    if (a_level.empty()) return LEVEL_NONE;

    auto s = boost::to_upper_copy(a_level);
    if (s == "WIRE")     return LEVEL_DEBUG;  // Backward compatibility
    if (s == "NONE" ||
        s == "FALSE")    return LEVEL_NONE;
    if (s == "TRACE")    return LEVEL_TRACE;
    if (s == "TRACE1")   return log_level(LEVEL_TRACE | LEVEL_TRACE1);
    if (s == "TRACE2")   return log_level(LEVEL_TRACE | LEVEL_TRACE2);
    if (s == "TRACE3")   return log_level(LEVEL_TRACE | LEVEL_TRACE3);
    if (s == "TRACE4")   return log_level(LEVEL_TRACE | LEVEL_TRACE4);
    if (s == "TRACE5")   return log_level(LEVEL_TRACE | LEVEL_TRACE5);
    if (s == "DEBUG")    return LEVEL_DEBUG;
    if (s == "INFO")     return LEVEL_INFO;
    if (s == "NOTICE")   return LEVEL_NOTICE;
    if (s == "WARNING")  return LEVEL_WARNING;
    if (s == "ERROR")    return LEVEL_ERROR;
    if (s == "FATAL")    return LEVEL_FATAL;
    if (s == "ALERT")    return LEVEL_ALERT;
    try   { auto n = std::stoi(s); return as_log_level(n); }
    catch (...) {}
    throw std::runtime_error("Invalid log level: " + a_level);
}

static inline int mask_bsf(log_level a_level) {
    auto l = static_cast<uint32_t>(a_level);
    return l ? ~((1u << (__builtin_ffs(l)-1))-1) : 0;
}

int logger::parse_min_log_level(const std::string& a_level) throw(std::runtime_error) {
    auto l = parse_log_level(a_level);
    return mask_bsf(l);
}

void logger::set_level_filter(log_level a_level) {
    m_level_filter = static_cast<int>(a_level);
}

void logger::set_min_level_filter(log_level a_level) {
    m_level_filter = mask_bsf(a_level);
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
    auto val = [](bool a) { return a ? "true" : "false"; };
    std::stringstream s;
    s   << "Logger settings:\n"
        << "    level-filter        = " << boost::to_upper_copy
                                    (log_levels_to_str(m_level_filter)) << '\n'
        << "    show-location       = " << val(m_show_location)         << '\n'
        << "    show-fun-namespaces = " << m_show_fun_namespaces        << '\n'
        << "    show-ident          = " << val(m_show_ident)            << '\n'
        << "    show-thread         = " << val(m_show_thread)           << '\n'
        << "    ident               = " << m_ident                      << '\n'
        << "    timestamp-type      = " << to_string(m_timestamp_type)  << '\n';

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
