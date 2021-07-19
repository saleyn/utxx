//----------------------------------------------------------------------------
/// \file  logger.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of logger functions and classes.
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
#include <utxx/logger/logger_util.hpp>
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

void finalize_logger_at_exit()
{
    try { logger::instance().finalize(); } catch(...) {}
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

void logger::add_level_filter(log_level level)
{
    static const unsigned s_nwe   = LEVEL_NOTICE | LEVEL_WARNING | LEVEL_ERROR;
    static const unsigned s_inwe  = LEVEL_INFO   | s_nwe;
    static const unsigned s_other = LEVEL_DEBUG  | s_inwe;

    switch (level) {
        case LEVEL_TRACE5   : m_level_filter |= LEVEL_TRACE  | LEVEL_TRACE1 |
                                                LEVEL_TRACE2 | LEVEL_TRACE3 |
                                                LEVEL_TRACE4 | LEVEL_TRACE5 |
                                                s_other;
                              break;
        case LEVEL_TRACE4   : m_level_filter |= LEVEL_TRACE  | LEVEL_TRACE1 |
                                                LEVEL_TRACE2 | LEVEL_TRACE3 |
                                                LEVEL_TRACE4 | s_other;
                              break;
        case LEVEL_TRACE3   : m_level_filter |= LEVEL_TRACE  | LEVEL_TRACE1 |
                                                LEVEL_TRACE2 | LEVEL_TRACE3 |
                                                LEVEL_DEBUG  | LEVEL_INFO   |
                                                LEVEL_NOTICE | s_other;
                              break;

        case LEVEL_TRACE2   : m_level_filter |= LEVEL_TRACE  | LEVEL_TRACE1 |
                                                LEVEL_TRACE2 | s_other;
                              break;
        case LEVEL_TRACE1   : m_level_filter |= LEVEL_TRACE  | LEVEL_TRACE1 | s_other;
                              break;
        case LEVEL_TRACE    : m_level_filter |= LEVEL_TRACE  | s_other; break;
        case LEVEL_DEBUG    : m_level_filter |= s_other;                break;
        case LEVEL_INFO     : m_level_filter |= s_inwe;                 break;
        case LEVEL_NOTICE   : m_level_filter |= s_nwe;                  break;
        default             : m_level_filter |= (unsigned)level;        break;
    }
}

const char* logger::default_log_levels = "INFO|NOTICE|WARNING|ERROR|ALERT|FATAL";
std::atomic<sigset_t*> logger::m_crash_sigset;

void logger::add_macro(const std::string& a_macro, const std::string& a_value)
{
    m_macro_var_map[a_macro] = a_value;
}

void logger::add_macros(const config_macros& a_macros)
{
    for (auto& p : a_macros)
      m_macro_var_map[p.first] = p.second;
}

std::string logger::replace_macros(const std::string& a_value) const
{
    return path::replace_macros(a_value, m_macro_var_map);
}

std::string logger::replace_env_and_macros(std::string const& a_value,
                                           time_val    const* a_now,
                                           bool               a_utc) const
{
    auto t = a_now ? a_now->to_tm(a_utc) : tm();
    return path::replace_env_and_macros(a_value, m_macro_var_map, &t);
}

void logger::init(const char* filename, const sigset_t* a_ignore_signals,
                  bool a_install_finalizer)
{
    config_tree pt;
    read_config(filename, pt);
    init(pt, a_ignore_signals, a_install_finalizer);
}

void logger::init(const config_tree& a_cfg, const sigset_t* a_ignore_signals,
                  bool a_install_finalizer)
{
    if (m_initialized)
        UTXX_THROW_RUNTIME_ERROR("Logger already initialized!");

    std::lock_guard<std::mutex> guard(m_mutex);
    do_finalize();

    try {
        m_show_location  = a_cfg.get<bool>       ("logger.show-location",    m_show_location);
        m_show_fun_namespaces = a_cfg.get<int>   ("logger.show-fun-namespaces",
                                                  m_show_fun_namespaces);
        m_show_category  = a_cfg.get<bool>       ("logger.show-category",    m_show_category);
        m_show_ident     = a_cfg.get<bool>       ("logger.show-ident",       m_show_ident);
        auto stt         = a_cfg.get_child_optional("logger.show-thread");
        std::string st("false");
        if (stt) {
            if      (stt->data().is_bool())  { if  (stt->data().to_bool()) st = "true"; }
            else if (stt->data().is_string())  st = stt->data().to_str();
            else     st = "";
        }
        m_show_thread    = st == "false" ? thr_id_type::NONE :
                           st == "true"  ? thr_id_type::NAME :
                           st == "name"  ? thr_id_type::NAME :
                           st == "id"    ? thr_id_type::ID   :
                           UTXX_THROW_RUNTIME_ERROR("Invalid logger.show-thread setting!");

        m_fatal_kill_signal = a_cfg.get<int>     ("logger.fatal-kill-signal",m_fatal_kill_signal);
        m_ident          = a_cfg.get<std::string>("logger.ident",            m_ident);
        m_ident          = replace_macros(m_ident);
        std::string ts   = a_cfg.get<std::string>("logger.timestamp",     "time-usec");
        m_timestamp_type = parse_stamp_type(ts);
        std::string levs = a_cfg.get<std::string>("logger.levels", "");
        if (!levs.empty())
            set_level_filter(static_cast<log_level>(parse_log_levels(levs)));
        std::string ls   = a_cfg.get<std::string>("logger.min-level-filter", "info");
        if (!levs.empty() && !ls.empty())
            UTXX_THROW_RUNTIME_ERROR
                ("Logger configurtion: either 'levels' or 'min-level-filter' option is permitted!");
        set_min_level_filter(parse_log_level(ls));
        long timeout_ms  = a_cfg.get<int>        ("logger.wait-timeout-ms", 1000);
        m_wait_timeout   = timespec{timeout_ms / 1000, timeout_ms % 1000 *  1000000L};
        m_sched_yield_us = a_cfg.get<long>       ("logger.sched-yield-us",  -1);
        m_silent_finish  = a_cfg.get<bool>       ("logger.silent-finish",   false);
        m_block_signals  = a_cfg.get<bool>       ("logger.block-signals",   true);

        if ((int)m_timestamp_type < 0)
            UTXX_THROW_RUNTIME_ERROR("Invalid logger timestamp type: ", ts);

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
                    UTXX_THROW_BADARG_ERROR("Implementation '", it->first,
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

        if (!m_finalizer_installed && a_install_finalizer) {
            atexit(&finalize_logger_at_exit);
            m_finalizer_installed = true;
        }
    } catch (utxx::runtime_error& e) {
        if (m_error)
            m_error(e.what());
        else
            throw;
    }
}

void logger::run()
{
    utxx::signal_block block_signals(m_block_signals);

    if (m_on_before_run)
        m_on_before_run();

    if (!m_ident.empty())
        pthread_setname_np(pthread_self(), m_ident.c_str());

    int  event_val;
    bool ok = true;

    while (!m_abort && ok) {
        event_val = m_event.value();
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
        // sched_yield may cause a system slowdown, so this option is
        // configurable by m_sched_yield_us:
        if (m_queue.empty() && m_sched_yield_us >= 0) {
            time_val deadline(rel_time(0, m_sched_yield_us));
            while (!m_abort && m_queue.empty()) {
                if (now_utc() > deadline)
                    break;
                sched_yield();
            }
        }

        // Lastly, flush the queue of pending messages
        ok = flush_internal();
    }

    // Flush the queue in case the logger is aborted while there are some
    // pending messages since last call to flush above
    if (ok)
        flush_internal();

    if (!m_silent_finish) {
        const msg msg(LEVEL_INFO, "", std::string("Logger thread finished"),
                      UTXX_LOG_SRCINFO);
        try { dolog_msg(msg); } catch (...) {}
    }

    if (m_on_after_run)
        m_on_after_run();
}

bool logger::flush_internal()
{
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
            basic_buffered_print<1024> buf;
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

            return false;
        }

        m_queue.free(item);
        item = next;
    }

    return true;
}

void logger::finalize()
{
    if (!m_initialized)
        return;

    std::lock_guard<std::mutex> g(m_mutex);
    if (!m_initialized)
        return;

    m_abort = true;
    if (m_thread)
        m_thread->join();
    m_thread.reset();
    do_finalize();
    m_abort       = false;
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
    if (show_thread() != thr_id_type::NONE) {
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

                if (fatal_kill_signal() && a_msg.level() == LEVEL_FATAL) {
                    m_abort = true;
                    dolog_fatal_msg(buf, p - buf);
                }

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

                if (fatal_kill_signal() && a_msg.level() == LEVEL_FATAL) {
                    m_abort = true;
                    dolog_fatal_msg(res.c_str(), res.size());
                }

                break;
            }
#if __cplusplus >= 201703L
            case payload_t::STR_VIEW:
#endif
            case payload_t::STR: {
                basic_buffered_print<1024> buf;
                char  pfx[256], sfx[256];
                char* p = format_header(a_msg, pfx, pfx + sizeof(pfx));
                char* q = format_footer(a_msg, sfx, sfx + sizeof(sfx));
                auto ps = p - pfx;
                auto qs = q - sfx;
                buf.reserve(a_msg.m_fun.str.size() + ps + qs + 1);
                buf.sprint(pfx, ps);
                auto& s = a_msg.m_fun.str;
                // Remove trailing new lines
                auto sz = int(s.size());
                while (sz && s[sz-1] == '\n') --sz;
                buf.sprint(s.c_str(), sz);
                buf.sprint(sfx, qs);
                m_sig_slot[level_to_signal_slot(a_msg.level())](
                    on_msg_delegate_t::invoker_type(a_msg, buf.str(), buf.size()));

                if (fatal_kill_signal() && a_msg.level() == LEVEL_FATAL) {
                    m_abort = true;
                    dolog_fatal_msg(buf.c_str(), buf.size());
                }

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

void logger::dolog_fatal_msg(const char* buf, size_t sz)
{
    // Expecting a Rethrowing signal string of
    // this format
    // (signal number)

    int   signum    = fatal_kill_signal(); //DEFAULT SIGNAL SIGABRT
    char* signo_str = (char*)memchr(static_cast<const void*>(buf), '(', sz);

    if (signo_str) {
        signo_str++;
        auto sig = atoi(signo_str);
        signum = sig;
    }

    if (signum) {
        std::cerr << "logger exiting from unknown fatal event" << signum << std::endl;
        detail::exit_with_default_sighandler(signum);
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

void logger::set_level_filter(log_level a_level) {
    m_level_filter = static_cast<int>(a_level);
}

void logger::set_min_level_filter(log_level a_level) {
    m_level_filter = detail::mask_bsf(a_level);
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
        << "    show-thread         = " << (m_show_thread==thr_id_type::ID   ? "id"   :
                                            m_show_thread==thr_id_type::NAME ? "name" :
                                            "false")                    << '\n'
        << "    ident               = " << m_ident                      << '\n'
        << "    timestamp-type      = " << to_string(m_timestamp_type)  << '\n';

    // Check the list of registered implementations. If corresponding
    // configuration section is found, initialize the implementation.
    for(auto it = m_implementations.begin(); it != m_implementations.end(); ++it)
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
