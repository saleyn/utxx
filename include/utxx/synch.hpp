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

#include <pthread.h>
#include <utxx/futex.hpp>
#include <utxx/meta.hpp>

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
namespace synch {

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
};

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

};

#endif

#ifdef __linux__

/*
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
#if __cplusplus >= 201103L
        std::atomic_thread_fence(std::memory_order_acquire);
#else
        atomic::memory_barrier();
#endif
    }

public:
    read_write_lock(): count(0) {
        // count 0 means "completely unlocked"
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
        // Last one out wakes and writer waiting.
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
*/
#endif

//-----------------------------------------------------------------------------

enum class lock_state { UNLOCKED = 0, LOCKED = 1 };

//-----------------------------------------------------------------------------

class read_write_spin_lock {
    std::atomic<long> m_lock;

    long value() { return m_lock.load(std::memory_order_relaxed); }
public:
    read_write_spin_lock() : m_lock(to_underlying(lock_state::UNLOCKED)) {}

    void write_lock() {
        while (1) {
            while (value() == to_underlying(lock_state::LOCKED));
            long old = to_underlying(lock_state::UNLOCKED);
            if (m_lock.compare_exchange_weak(old, to_underlying(lock_state::LOCKED),
                    std::memory_order_release, std::memory_order_relaxed))
                return;
        }
    }

    void write_unlock() {
        m_lock.store(
            to_underlying(lock_state::UNLOCKED), std::memory_order_relaxed);
    }

    void read_lock() {
        long oldval, newval;
        while (1) {
            // lower bit is 1 when there's a write lock
            while ((oldval = value()) == to_underlying(lock_state::LOCKED));
            newval = oldval + 2;
            if (m_lock.compare_exchange_weak(oldval, newval,
                    std::memory_order_release, std::memory_order_relaxed))
                break;
        }
    }

    void read_unlock() {
        long oldval, newval;
        while (1) {
            oldval = value();
            newval = oldval - 2;
            if (m_lock.compare_exchange_weak(oldval, newval,
                    std::memory_order_release, std::memory_order_relaxed))
                break;
        }
    }
};

//-----------------------------------------------------------------------------

class spin_lock {
    std::atomic<long> m_lock;
    long value() { return m_lock.load(std::memory_order_relaxed); }
public:
    spin_lock() : m_lock(to_underlying(lock_state::UNLOCKED)) {}

    void lock() {
        while (1) {
            while (m_lock == to_underlying(lock_state::LOCKED));
            long old = to_underlying(lock_state::UNLOCKED);
            if (m_lock.compare_exchange_weak(old, to_underlying(lock_state::LOCKED),
                    std::memory_order_release, std::memory_order_relaxed))
                return;
        }
    }

    int try_lock() {
        if (value())
            return -1;
        long old = to_underlying(lock_state::UNLOCKED);
        if (m_lock.compare_exchange_weak(old, to_underlying(lock_state::LOCKED),
                    std::memory_order_release, std::memory_order_relaxed))
            return 0;
        else
            return -1;
    }

    void unlock() {
        m_lock.store(to_underlying(lock_state::UNLOCKED), std::memory_order_relaxed);
    }
};

#if __cplusplus >= 201103L

typedef std::mutex mutex_lock;

struct null_lock {
    typedef std::lock_guard<null_lock> scoped_lock;
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

} // namespace synch
} // namespace utxx

#endif // _UTXX_SYNCH_HPP_

