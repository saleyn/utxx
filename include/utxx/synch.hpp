//----------------------------------------------------------------------------
/// \file  synch.hpp
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
#ifndef _UTXX_SYNCH_HPP_
#define _UTXX_SYNCH_HPP_

#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <utxx/atomic.hpp>

#if __cplusplus >= 201103L
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#else
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#endif

//-----------------------------------------------------------------------------

namespace utxx {

#ifdef __linux__

#if defined(SYS_futex)
inline int futex_wait(volatile void* futex, int val, struct timespec* timeout = NULL) {
    return ::syscall(SYS_futex,(futex),FUTEX_WAIT,(val),(timeout));
}
inline int futex_wake(volatile void* futex, int val, struct timespec* timeout = NULL) {
    return ::syscall(SYS_futex,(futex),FUTEX_WAKE,(val),(timeout));
}
#else
#  error "Missing SYS_futex definition!"
#endif

#endif

namespace synch {

#ifdef __linux__

/** Fast futex-based concurrent notification primitive.
 * Supports signal/wait semantics.
 */
class futex {
    volatile int m_count;
    #ifdef PERF_STATS
    unsigned int m_wait_count;
    unsigned int m_wake_count;
    unsigned int m_wait_fast_count;
    unsigned int m_wake_fast_count;
    #endif

    static const int FUTEX_PASSED = -(1 << 30);

    /// @return -1 - fail, 0 - wakeup, 1 - pass, 2 - didn't sleep
    int wait_slow(int val, const struct timespec *rel = NULL);
    int signal_slow(int count = 1);

    /// Atomic dec of internal counter.
    /// @return 0 when value is different from old_value or someone updated
    ///           it during wait_fast call.
    ///       < 0 when current value is negative
    ///        -1 when the thread should wait for another signal
    int wait_fast(int* old_value = NULL) {
        int val = m_count;

        if (old_value && *old_value != val) {
            if (old_value)
                *old_value = val;
            return 0;
        }

        // Don't decrement if already negative.
        if (val < 0) return val;

        //int cur_val = atomic::cmpxchg(&m_count, val, val-1);
        unsigned char eqz;

        // decrement the counter and check if it's changed to 0.
        __asm__ __volatile__(
            "lock; decl %0; sete %1"
            :"=m" (m_count), "=qm" (eqz)
            :"m"  (m_count) : "memory");

        // If eqz - we know m_count is 0 - it's an uncontended case, waiting is done.
        // Otherwise, return we have no way of knowing the value. Return -1 to
        // indicate that the thread should wait for another thread to "up" the futex.
        return eqz ? 0 : -1;
    }

    /// Atomic inc
    /// @return 1 if counter incremented from 0 to 1.
    ///         0 otherwise.
    int signal_fast() {
        int res = 1;

        // r = ++m_count >= 1 ? 1 : 0;
        __asm__ __volatile__ (
            "   lock; incl %1\n"
            "   jg 1f\n"
            "   decl %0\n"
            "1:\n"
                : "=q"(res), "=m"(m_count) : "0"(res));
        return res;
    }

    void commit(int n) {
        m_count = n;
        // Probably overkill, but some non-Intel clones support
        // out-of-order stores, according to 2.5.5-pre1's
        // linux/include/asm-i386/system.h
        //__asm__ __volatile__ ("lock; addl $0,0(%%esp)": : :"memory");
        atomic::memory_barrier();
    }

public:
    futex(int initialize = 1);

    /// This is mainly for debugging
    int  value() const { return m_count; }
    int  reset()       { commit(1); return 1; }

    #ifdef PERF_STATS
    unsigned int wake_count()      const { return m_wake_count; }
    unsigned int wait_count()      const { return m_wait_count; }
    unsigned int wake_fast_count() const { return m_wake_fast_count; }
    unsigned int wait_fast_count() const { return m_wait_fast_count; }
    #endif

    /// Signal the futex by incrementing the internal
    /// variable and optionally making a system call.
    /// @return 0 if someone was waiting
    ///         1 if a process was waken up by a futex system call
    int signal() {
        if (!signal_fast())
            return signal_slow();
        #ifdef PERF_STATS
        ++m_wake_fast_count;
        #endif
        return 0;
    }

    /// If signal_fast() increments count from 0 -> 1, no one was waiting.
    /// Otherwise, set to 1 and tell kernel to wake them up.  Because of
    /// an additional memory barrier and a sys_futex call this method
    /// is slower than <signal()>.  This function passes a token to one of
    /// the waiters to prevent starvation - all waiters are queued up
    /// behind each other in the order they started waiting.
    int signal_fair();

    /// Signal all waiting threads.
    /// @return number of processes woken up.
    int signal_all() { return signal_slow(INT_MAX); };

    /// Non-blocking attempt to wait for signal
    /// @return 0 - success, -1 - no pending signal
    int try_wait(int* old_val = NULL) {
        return wait_fast(old_val) == 0 ? 0 : -1;
    }

    /// Wait for signaled condition up to \a timeout. Note that
    /// the call ignores spurious wakeups.
    /// @param <old_val> - pointer to the old <value()> of futex
    ///         known just before calling <wait()> function.
    /// @return 0           - woken up or value changed before sleep
    ///         -1          - timeout or some other error occured
    ///         -ETIMEDOUT  - timed out (FIXME: this error code is presently not
    ///                                  being returned)
    int wait(int* old_val = NULL) { return wait(NULL, old_val); }

    /// Wait for signaled condition up to \a timeout. Note that
    /// the call ignores spurious wakeups.
    /// @param <timeout> - max time to wait (NULL means infinity)
    /// @param <old_val> - pointer to the old <value()> of futex
    ///         known just before calling <wait()> function.
    /// @return 0           - woken up or value changed before sleep
    ///         -1          - timeout or some other error occured
    ///         -ETIMEDOUT  - timed out (FIXME: this error code is presently not
    ///                                  being returned)
    int wait(const struct timespec *timeout, int* old_val = NULL);

#if __cplusplus >= 201103L

    /// Wait for signaled condition until \a wait_until_abs_time.
    /// \copydetails wait()
    template<class Clock, class Duration>
    int wait
    (
        const std::chrono::time_point<Clock, Duration>& wait_until_abs_time,
        int* old_val = NULL
    ) {
        auto sec =
            std::chrono::duration_cast<std::chrono::seconds>(wait_until_abs_time);
        auto nsec =
            std::chrono::duration_cast<std::chrono::nanoseconds>(wait_until_abs_time);
        struct timespec ts = { sec.count(), nsec.count() };
        return wait(&ts, old_val);
    }

#else

    /// Wait for signaled condition until \a wait_until_abs_time.
    /// \copydetails wait()
    int wait(const boost::system_time& wait_until_abs_time, int* old_val = NULL) {
        struct timespec ts =
            #if BOOST_VERSION >= 105300
            boost::detail::to_timespec(wait_until_abs_time);
            #else
            boost::detail::get_timespec(wait_until_abs_time);
            #endif
        return wait(&ts, old_val);
    }

#endif

};

#endif

// Use this event when futex is not available.
#if __cplusplus >= 201103L

class posix_event {
    std::atomic<long> m_count;
    std::mutex m_lock;
    std::condition_variable m_cond;
public:
    posix_event(bool initialize=true) {
        if (initialize)
            m_count = 1;
    }

    int  value() const { return m_count; }

    void reset(long val = 1) {
        m_count.store(val, std::memory_order_release);
    }

    int signal() {
        m_count.fetch_add(1, std::memory_order_relaxed);
        m_cond.notify_one();
        return 0;
    }

    int signal_all() {
        std::unique_lock<std::mutex> g(m_lock);
        m_count.fetch_add(1, std::memory_order_relaxed);
        m_cond.notify_all();
        return 0;
    }

    int wait(long* old_val = NULL) {
        if (old_val) {
            long cur_val = m_count.load(std::memory_order_relaxed);
            if (*old_val != cur_val) {
                *old_val = cur_val;
                return 0;
            }
        }
        std::unique_lock<std::mutex> g(m_lock);
        try { m_cond.wait(g); } catch(...) { return -1; }
        return 0;
    }

    /// Wait for signaled condition until \a wait_until_abs_time.
    /// \copydetails wait()
    template<class Clock, class Duration>
    int wait
    (
        const std::chrono::time_point<Clock, Duration>& wait_until_abs_time,
        long* old_val = NULL
    ) {
        if (old_val) {
            long cur_val = m_count.load(std::memory_order_relaxed);
            if (*old_val != cur_val) {
                *old_val = cur_val;
                return 0;
            }
        }
        std::unique_lock<std::mutex> g(m_lock);
        return m_cond.wait_until(g, wait_until_abs_time) == std::cv_status::no_timeout
             ? 0 : ETIMEDOUT;
    }

#else

class posix_event {
    volatile int m_count;
    boost::mutex m_lock;
    boost::condition_variable m_cond;
    typedef boost::mutex::scoped_lock scoped_lock;

public:
    posix_event(bool initialize=true) {
        if (initialize)
            m_count = 1;
    }

    int  value() const { return m_count; }

    void reset(int val = 1) {
        scoped_lock g(m_lock);
        m_count = val;
    }

    int signal() {
        scoped_lock g(m_lock);
        m_count = (m_count + 1) & 0xEFFFFFFF;
        m_cond.notify_one();
        return 0;
    }

    int signal_all() {
        scoped_lock g(m_lock);
        m_count = (m_count + 1) & 0xEFFFFFFF;
        m_cond.notify_all();
        return 0;
    }

    int wait(int* old_val = NULL) {
        if (old_val && *old_val != m_count) {
            *old_val = m_count;
            return 0;
        }
        scoped_lock g(m_lock);
        try { m_cond.wait(g); } catch(...) { return -1; }
        return 0;
    }

    int wait(const boost::system_time& wait_until_abs_time, int* old_val = NULL) {
        #if 0
        if (old_val && *old_val != m_count) {
            *old_val = m_count;
            return 0;
        }
        #endif
        scoped_lock g(m_lock);
        try {
            return m_cond.timed_wait(g, wait_until_abs_time) ? 0 : ETIMEDOUT;
        } catch (std::exception&) {
            return -1;
        }
    }

#endif

};

#ifdef __linux__

/// Futex based read-write lock.  This is a 
/// C++ implementation of Rusty Russell's furwock.
class read_write_lock {
    futex gate;           // Protects the data.
    volatile int count;   // If writer waiting, gate held and
                          // counter = # readers - 1.
                          // Otherwise, counter = # readers.
    futex wait;           // Simple downed semaphore for writer to sleep on.

    int dec_negative() {
        unsigned char r;
        __asm__ __volatile__(
              "lock; decl %0; setl %1"
            : "=m" (count), "=qm" (r)
            : "m" (count) : "memory");
        return r;
    }

    void commit(int n) {
        count = n;
        // Probably overkill, but some non-Intel clones support
        // out-of-order stores, according to 2.5.5-pre1's
        // linux/include/asm-i386/system.h
        // __asm__ __volatile__ ("lock; addl $0,0(%%esp)": : :"memory");
        atomic::memory_barrier();
    }

public:
    read_write_lock(): count(0) {
        /* count 0 means "completely unlocked" */
        wait.try_wait();
    }

    int read_lock() {
        int ret = gate.wait();
        if (ret == 0) {
            atomic::inc((volatile long*)&count);
            gate.signal();
        }
        return ret;
    }

    int try_read_lock() {
        int ret = gate.try_wait();
        if (ret == 0) {
            if (count >= 0)
                atomic::inc((volatile long*)&count);
            gate.signal();
        }
        return ret;
    }

    void read_unlock() {
        /* Last one out wakes and writer waiting. */
        if (dec_negative())
            wait.signal();
    }

    int write_lock() {
        int ret = gate.wait();
        if (ret == 0) {
            if (dec_negative())
                return ret;
            wait.signal();
        }
        return ret;
    }

    int try_write_lock() {
        int ret = gate.try_wait();
        if (ret == 0) {
            if (dec_negative())
                return ret;
            wait.signal();
        }
        return ret;
    }

    void write_unlock() {
        commit(0);
        gate.signal();
    }
};

#endif

//-----------------------------------------------------------------------------

enum lock_state { UNLOCKED = 0, LOCKED = 1 };

//-----------------------------------------------------------------------------

class read_write_spin_lock {
    volatile unsigned long m_lock;
public:
    read_write_spin_lock() : m_lock(UNLOCKED) {}

    void write_lock() {
        while (1) {
            while (m_lock == LOCKED);
            if (atomic::cas(&m_lock, UNLOCKED, LOCKED))
                return;
        }
    }

    void write_unlock() {
        m_lock = UNLOCKED;
        //atomic::memory_barrier();
    }

    void read_lock() {
        unsigned long oldval, newval;
        while (1) {
            while ((oldval = m_lock) == LOCKED); // lower bit is 1 when there's a write lock
            newval = oldval + 2;
            if (atomic::cas(&m_lock, oldval, newval))
                break;
        }
    }

    void read_unlock() {
        unsigned long oldval, newval;
        while (1) {
            oldval = m_lock;
            newval = oldval - 2;
            if (atomic::cas(&m_lock, oldval, newval))
                break;
        }
    }
};

//-----------------------------------------------------------------------------

class spin_lock {
    volatile unsigned long m_lock;
public:
    spin_lock() : m_lock(UNLOCKED) {}

    void lock() {
        while (1) {
            while (m_lock == LOCKED);
            if (atomic::cas(&m_lock, UNLOCKED, LOCKED))
                return;
        }
    }

    int try_lock() {
        if (m_lock)                                      return -1;
        else if (atomic::cas(&m_lock, UNLOCKED, LOCKED)) return 0;
        else                                             return -1;
    }

    void unlock() {
        m_lock = UNLOCKED;
        //atomic::memory_barrier();
    }
};

#if __cplusplus >= 201103L

typedef std::mutex mutex_lock;

struct null_lock {
    typedef std::lock_guard<null_lock> scoped_lock;
    //typedef std::try_lock_wrapper<null_lock> scoped_try_lock;
    void lock()     {}
    int  try_lock() { return 0; }
    void unlock()   {}
};

#else

typedef boost::mutex mutex_lock;

struct null_lock {
    typedef boost::lock_guard<null_lock> scoped_lock;
    typedef boost::detail::try_lock_wrapper<null_lock> scoped_try_lock;
    void lock()     {}
    int  try_lock() { return 0; }
    void unlock()   {}
};

#endif

}   // namespace sync
}   // namespace utxx

//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------

#endif // _UTXX_SYNCH_HPP_

