/**
 * Base implementation of lock-free fifo queues.
 *-----------------------------------------------------------------------------
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
 * Created: 2010-02-03
 * $Id$
 */

#ifndef _UTIL_CONCURRENT_FIFO_BASE_HPP_
#define _UTIL_CONCURRENT_FIFO_BASE_HPP_

#include <boost/noncopyable.hpp>
#include <util/atomic.hpp>
#include <util/container/concurrent_stack.hpp>
#include <util/container/detail/base_allocator.hpp>
#include <ostream>

namespace util {
namespace container {
namespace detail {

//-----------------------------------------------------------------------------
/// @class lock_free_queue
/// Implementation can be bound or unbound depending on the chosen allocator.
//-----------------------------------------------------------------------------

template <typename T, typename AllocT> 
class lock_free_queue: node_t<T>, boost::noncopyable {
    typedef node_t<T>   node_type;

    volatile node_type* m_head;
    volatile node_type* m_tail;

    AllocT&             m_allocator;
    bool                m_empty_on_destruction;

public:
    lock_free_queue(AllocT& alloc, bool empty_on_destruction = true)
        : m_head(this), m_tail(this), m_allocator(alloc)
        , m_empty_on_destruction(empty_on_destruction)
    {}

    ~lock_free_queue() {
        if (!m_empty_on_destruction)
            return;
        volatile node_type* h = m_head;
        while (h) {
            volatile node_type* next = h->next;
            if (likely(h != this))
                m_allocator.free(const_cast<node_type*>(h));
            h = next;
        }
    }

    bool enqueue(const T& item) {
        node_type* nd = m_allocator.allocate();

        if (nd == NULL)
            return false;  // out of memory or queue is full

        new (nd) node_type(item);

        volatile node_type* old_tail;

        while(1) {
            old_tail        = m_tail;
            volatile node_type* next = old_tail->next;

            if (old_tail != m_tail)
                continue;

            if (next != NULL) {
                // Tail is falling behind
                atomic::cmpxchg(&m_tail, old_tail, next);
                continue;
            }

            if (atomic::cas(&old_tail->next, (node_type*)NULL, nd))
                break;
        }
        
        // Possibly replace tail
        atomic::cmpxchg(&m_tail, const_cast<node_type*>(old_tail), nd);
        return true;
    }


    bool dequeue(T& item) {
        volatile node_type* old_head;
        while(1) {
            old_head        = m_head;
            volatile node_type* next = old_head->next;

            if (m_head != old_head)
                continue;

            if (next == NULL)
                return false;

            if (m_head != old_head)
                continue;

            volatile node_type* old_tail = m_tail;
            if (old_head == old_tail) { // tail is falling behind
                atomic::cmpxchg(&m_tail, old_tail, next);
                continue;
            }

            item = const_cast<const T&>(next->data);

            if (atomic::cas(&m_head, old_head, next))
                break;
        }

        if (likely(old_head != this))
            m_allocator.free(const_cast<node_type*>(old_head));
        return true;
    }

    /// Returns true if the queue is empty.
    bool empty() const { return m_head->next == NULL; }

    /// This method is not thread safe! Use for debugging only!
    /// @return size of the queue.
    unsigned int unsafe_size() const {
        unsigned int result = 0;
        for (volatile node_type* h = m_head->next; h != NULL; h = h->next)
            result++;
        return result;
    }

    #ifdef DEBUG
    std::ostream& dump(std::ostream& out, std::string (*f)(const T&) = NULL) const;
    #endif
};

//-----------------------------------------------------------------------------
/// @class blocking_lock_free_queue
/// A queue class that supports blocking put/get operations in cases when the
/// queue is full/empty.
/// Implementation can be bound or unbound depending on the chosen allocator.
//-----------------------------------------------------------------------------
template <typename T, typename AllocT, bool IsBound, typename EventT = synch::futex>
class blocking_lock_free_queue {
    AllocT&                     m_allocator;
    lock_free_queue<T, AllocT>  m_queue;
    EventT                      m_not_empty_condition;
    EventT                      m_not_full_condition;
    bool                        m_terminated;
public:
    blocking_lock_free_queue(AllocT& alloc)
        : m_allocator(alloc)
        , m_queue(alloc), m_not_empty_condition(true)
        , m_not_full_condition(true)
        , m_terminated(false)
    {}

    void reset() {
        m_not_empty_condition.reset();
        m_not_full_condition.reset();
        m_terminated = false;
    }

    bool try_enqueue(const T& item) { return m_queue.enqueue(item); }
    bool try_dequeue(T& item)       { return m_queue.dequeue(item); }

    int  enqueue(const T& item, struct timespec* timeout = NULL) {
        if (m_terminated)
            return -1;
        if (!IsBound) {
            bool ok = try_enqueue(item);
            if (ok)
                m_not_empty_condition.signal();
            return ok ? 0 : -1;
        }
        int sync_val = m_not_full_condition.value();
        if (try_enqueue(item)) {
            m_not_empty_condition.signal();
            return 0;
        }
        int res = m_not_full_condition.wait(timeout, &sync_val);
        if (res < 0 || m_terminated) {
            m_not_empty_condition.signal();
            return res;
        }
        if (try_enqueue(item)) {
            m_not_empty_condition.signal();
            return 0;
        }
        return -1;
    }

    int  dequeue(T& item, struct timespec* timeout = NULL) {
        if (m_terminated)
            return -2;
        int sync_val = m_not_empty_condition.value();
        if (try_dequeue(item)) {
            if (IsBound)
                m_not_full_condition.signal();
            return 0;
        }
        int res = m_not_empty_condition.wait(timeout, &sync_val);
        if (res < 0 || m_terminated) {
            if (IsBound)
                m_not_full_condition.signal();
            return res;
        }
        if (try_dequeue(item)) {
            if (IsBound)
                m_not_full_condition.signal();
            return 0;
        }
        return -1;
    }

    /// Returns true if the queue is empty.
    bool empty() const { return m_queue.empty(); }

    void terminate() {
        m_terminated = true;
        m_not_empty_condition.signal_all();
        m_not_full_condition.signal_all();
    }

    /// This method is not thread safe! Use for debugging only!
    /// @return size of the queue.
    unsigned int unsafe_size() const { return m_queue.unsafe_size; }

    #ifdef DEBUG
    /// Dump the queue content.
    std::ostream& dump(std::ostream& out, std::string (*f)(const T&) = NULL) const {
        return m_queue.dump(out, f);
    }
    #endif
};

//-----------------------------------------------------------------------------
// IMPLEMENTATION
//-----------------------------------------------------------------------------
#ifdef DEBUG
template<class T, class AllocT>
std::ostream& lock_free_queue<T, AllocT>
::dump(std::ostream& s, std::string (*f)(const T&)) const {
    size_t width;
    {
        std::stringstream s;
        s << unsafe_size();
        width = s.str().size();
    }
    std::stringstream out;
    out << "Dumping queue:" << std::endl;
    unsigned int i = 0;
    for (volatile node_type* h = m_head->next; h != NULL; h = h->next) {
        T v = h->data;
    	out << " [" << std::setw(width) << i << "] = "
            << h << " " << (f ? (*f)(v).c_str() : "") << std::endl;
        i++;
    }
    return s << out.str();
}
#endif

} // namespace detail
} // namespace container
} // namespace util

#endif // _UTIL_CONCURRENT_FIFO_BASE_HPP_

