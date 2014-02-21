//----------------------------------------------------------------------------
/// \file  synch.cpp
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
#include <utxx/synch.hpp>

namespace utxx {
namespace synch {

//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------

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
    m_wait_count      = m_wake_count      = 0;
    m_wait_fast_count = m_wake_fast_count = 0;
    #endif
}

int futex::wait(const struct timespec *timeout, int* old_val) {
    int  val, res = 0;
    bool woken = false;

    while ((val = wait_fast(old_val)) != 0) {
        int v = old_val ? *old_val : m_count;
        switch (res = wait_slow(v, timeout)) {
            case -1: break;             // Error or timeout.
            case  1: break;             // Passed.
            case  0:
                woken = true;           // Slept and got woken
                break;                  // someone else might be sleeping too
                                        // set the count to wake them up.
            case -ETIMEDOUT:
                break;
            default:
                continue;
        }
        goto wait_done;
    }

    #ifdef PERF_STATS
    ++m_wait_fast_count;
    #endif

wait_done:
    if (woken)
        m_count = -1;
    if (old_val)
        *old_val = m_count;
    return res;
}

/// @returns  0         if the process was woken by a FUTEX_WAKE call.
///           1         if someone signalled the futex by passing FUTEX_PASSED token
///           2         if value changed before futex_wait call
///          -ETIMEDOUT if timed out
///          -1         if some other error
int futex::wait_slow(int val, const struct timespec *rel) {
    #ifdef PERF_STATS
    ++m_wait_count;
    #endif
    int res;
    // futex_wake returns 0 or -1
    // see: http://man7.org/linux/man-pages/man2/futex.2.html
    while ((res = futex_wait(&m_count, val, const_cast<struct timespec*>(rel))) == -EINTR);
    if (res == 0) {
        // <= in case someone else decremented it
        if (m_count <= FUTEX_PASSED) {
            m_count = -1;
            return 1;
        }
        return 0;
    } else if (errno == EWOULDBLOCK)
        return 2;   // EWOULDBLOCK means value changed before futex_wait
    else if (errno == ETIMEDOUT)
        return -ETIMEDOUT;
    else
        return -1;
}

int futex::signal_slow(int count) {
    commit(1);
    #ifdef PERF_STATS
    ++m_wake_count;
    #endif
    return futex_wake(&m_count, count, NULL);
}

int futex::signal_fair() {
    // Is anyone waiting?
    if (!signal_fast()) {
        commit(FUTEX_PASSED);
        #ifdef PERF_STATS
        ++m_wake_count;
        #endif
        // If we wake one, they'll see it's a direct pass.
        if (futex_wake(&m_count, 1, NULL) == 1)
            return 0;
        // Otherwise do normal slow case
        return signal_slow();
    }
    return 0;
}

}   // namespace sync
}   // namespace utxx

