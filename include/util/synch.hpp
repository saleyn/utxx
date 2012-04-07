/**
 * Concurrent notification primitives.
 *
 * Futex class is an enhanced C++ version of Rusty Russell's furlock
 * C interface found in:
 * http://www.kernel.org/pub/linux/kernel/people/rusty/futex-2.2.tar.gz
 */
/*-----------------------------------------------------------------------------
 * Copyright (c) 2009 Serge Aleynikov <serge@aleynikov.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *-----------------------------------------------------------------------------
 * Created: 2009-11-19
 * $Id$
 */

#ifndef _UTIL_SYNCH_HPP_
#define _UTIL_SYNCH_HPP_

#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <util/atomic.hpp>

//-----------------------------------------------------------------------------

namespace util {

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


namespace synch {

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
    ///    it during wait_fast call. Otherwise current value is returned.
    int wait_fast(int* old_value = NULL) {
    	int val = m_count;

        if (old_value && *old_value != val)
            return 0;

    	// Don't decrement if already negative.
    	if (val < 0) return val;

    	//int cur_val = atomic::cmpxchg(&m_count, val, val-1);
        unsigned char eqz;
        
        // decrement the counter and check if it's changed to 0.
        // If so, it's an uncontended case - the operation is done.
        // Otherwise, return -1 to indicate that the thread should
        // wait for another thread to "up" the futex.
        __asm__ __volatile__(
            "lock; decl %0; sete %1"
            :"=m" (m_count), "=qm" (eqz)
            :"m"  (m_count) : "memory");

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
    futex(bool initialize = true);

    /// This is mainly for debugging
    int  value() const { return m_count; }
    void reset()       { commit(1); }

    #ifdef PERF_STATS
    unsigned int wake_count()      const { return m_wake_count; }
    unsigned int wait_count()      const { return m_wait_count; }
    unsigned int wake_fast_count() const { return m_wake_fast_count; }
    unsigned int wait_fast_count() const { return m_wait_fast_count; }
    #endif

    /// Signal the futex by incrementing the internal
    /// variable and optionally making a system call.
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

    /// Wait for signaled condition up to <timeout>. Note that
    /// the call ignores spurious wakeups.
    /// @param <timeout> - max time to wait (NULL means infinity)
    /// @param <old_val> - pointer to the old <value()> of futex
    ///         known just before calling <wait()> function.
    /// @return 0           - woken up or value changed before sleep
    ///         -ETIMEDOUT  - timed out
    ///         -N          - some other error occured
    int wait(const struct timespec *timeout = NULL, int* old_val = NULL);
};

// Use this event when futex is not available.
class posix_event {
    volatile int m_count;
    pthread_mutex_t m;
    pthread_cond_t  c;
public:
    posix_event(bool initialize=true) {
        pthread_mutex_init(&m, NULL);
        pthread_cond_init(&c, NULL);
        if (initialize)
            m_count = 1;
    }
    ~posix_event() {
        pthread_mutex_destroy(&m);
        pthread_cond_destroy(&c);
    }

    int  value() const { return m_count; }

    void reset(int val = 1) {
        pthread_mutex_lock(&m);
        m_count = val;
        pthread_mutex_unlock(&m);
    }

    int signal() {
        pthread_mutex_lock(&m);
        m_count++;
        pthread_mutex_unlock(&m);
        // Note: sending signal outside of a critical section
        // is ok by POSIX and likely more efficient (though may
        // lead to unpredictable scheduling (see diagram below)
        pthread_cond_signal(&c);
        return 0;
    }

    int wait(struct timespec *timeout = NULL, int* old_val = NULL) {
        int val = 0;
        if (old_val && *old_val != m_count)
            return 0;

        pthread_mutex_lock(&m);
        if (old_val && *old_val == m_count)
            while (m_count < 0) {
                m_count--;
                val = timeout ? pthread_cond_timedwait(&c, &m, timeout)
                              : pthread_cond_wait(&c, &m);
            }
        pthread_mutex_unlock(&m);
        return val;
    }
};

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

class mutex_lock {
    pthread_mutex_t m;
public:
    mutex_lock()    { pthread_mutex_init(&m, NULL); }
    ~mutex_lock()   { pthread_mutex_destroy(&m); }
    void lock()     { pthread_mutex_lock(&m); }
    int  try_lock() { return pthread_mutex_trylock(&m); }
    void unlock()   { pthread_mutex_unlock(&m); }
};

struct null_lock {
    void lock()     {}
    int  try_lock() { return 0; }
    void unlock()   {}
};

//-----------------------------------------------------------------------------

template <typename Lock>
class lock_guard {
    Lock* m_lock;
public:
    lock_guard(Lock& lock) : m_lock(&lock) {
        m_lock->lock();
    }

    lock_guard(lock_guard<Lock>& guard): m_lock(guard.m_lock) { 
        guard.m_lock = NULL; 
    }

    void operator= (lock_guard<Lock>& guard) {
        m_lock = guard.m_lock;
        guard.m_lock = NULL;
    }

    ~lock_guard() {
        if (m_lock)
            m_lock->unlock();
    }
};

}   // namespace sync
}   // namespace util

//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------

#endif // _UTIL_SYNCH_HPP_

