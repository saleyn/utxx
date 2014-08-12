//----------------------------------------------------------------------------
/// \file  futex.cpp
//----------------------------------------------------------------------------
/// \brief Concurrent notification primitives.
/// Futex class is an enhanced C++ version of Rusty Russell's furlock
/// interface found in:
/// http://www.kernel.org/pub/linux/kernel/people/rusty/futex-2.2.tar.gz
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-09-10
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
#include <utxx/futex.hpp>
#include <utxx/meta.hpp>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/futex.h>

namespace utxx {

//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------
#ifdef __linux__

#if defined(SYS_futex)
inline int futex_wait(volatile void* futex, int val,
                        const struct timespec* timeout = NULL) {
    return ::syscall(SYS_futex,(futex),FUTEX_WAIT,(val),(timeout));
}
inline int futex_wake(volatile void* futex, int cnt_to_wake,
                        const struct timespec* timeout = NULL) {
    return ::syscall(SYS_futex,(futex),FUTEX_WAKE,(cnt_to_wake),(timeout));
}
#else
#  error "Missing SYS_futex definition!"
#endif

#endif

const char* to_string(wakeup_result res)
{
    static const char* s_values[] =
        {"ERROR", "SIGNALED", "CHANGED", "TIMEDOUT" };
    return s_values[to_underlying(res)+1];
}


futex::futex(int initialize) {
    int pagesize = sysconf(_SC_PAGESIZE);

    // Align pointer to previous start of page
    void* area = (void *)((unsigned long)this / pagesize * pagesize);

    // Round size up to page size
    size_t size = (sizeof(futex) + pagesize - 1) / pagesize * pagesize;

    #ifndef PROT_SEM
    static const int PROT_SEM = 0x08;
    #endif

    // PROT_SEM is currently a noop. By the call below we are
    // explicitely pinning the page owning this futex to memory, which
    // strictly speaking is unnecessary because futex calls do this
    // implicitely anyway.
    mprotect(area, size, PROT_READ|PROT_WRITE|PROT_SEM);

    if (initialize)
        commit(initialize);

    #ifdef PERF_STATS
    m_wait_count      = m_wake_count      = m_wake_signaled_count = 0;
    m_wait_fast_count = m_wake_fast_count = m_wait_spin_count     = 0;
    #endif
}

wakeup_result futex::wait(const struct timespec *timeout, int* old_val) {
    wakeup_result res;
    int val = old_val ? *old_val : value();

    while ((res = wait_fast(&val)) == wakeup_result::TIMEDOUT) {
        switch (res = wait_slow(val, timeout)) {
            case wakeup_result::ERROR:
            case wakeup_result::CHANGED:
                val = value();
                break;                  // Value changed.
            case wakeup_result::SIGNALED:
                val = 0;
                commit(val);            // Slept and got woken
                break;                  // someone else might be sleeping too
                                        // set the count to wake them up.
            case wakeup_result::TIMEDOUT:
                return res;
            default:
                #ifdef PERF_STATS
                ++m_wait_spin_count;
                #endif
                continue;
        }
        goto wait_done;
    }

wait_done:
    if (old_val)
        *old_val = val;
    return res;
}

/// @returns  SIGNALED  if the process was woken by a FUTEX_WAKE call.
///           CHANGED   if value changed before futex_wait call
///           TIMEDOUT  if timed out
///           ERROR     if some other error
wakeup_result futex::wait_slow(int val, const struct timespec *rel) {
    #ifdef PERF_STATS
    ++m_wait_count;
    #endif
    int res;
    // futex_wake returns 0 or -1
    // see: http://man7.org/linux/man-pages/man2/futex.2.html
    while ((res = futex_wait(reinterpret_cast<int*>(&m_count), val, rel)) < 0
           && errno == EINTR);
    if (res == 0) {
        #ifdef PERF_STATS
        ++m_wake_signaled_count;
        #endif
        return wakeup_result::SIGNALED;
    } else if (errno == EWOULDBLOCK)
        return wakeup_result::CHANGED;   // value changed before futex_wait
    else if (errno == ETIMEDOUT)
        return wakeup_result::TIMEDOUT;
    else
        return wakeup_result::ERROR;     // This really should never happen
}

int futex::signal_slow(int count) {
    //commit(1);
    #ifdef PERF_STATS
    ++m_wake_count;
    #endif
    int res;
    while ((res = futex_wake(reinterpret_cast<int*>(&m_count), count, NULL)) < 0
           && errno == EINTR);
    return res;
}

}   // namespace utxx

