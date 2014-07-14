// vim:ts=4:et:sw=4
//----------------------------------------------------------------------------
/// \file   concurrent_spsc_queue.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Producer/consumer queue.
///
/// Based on code from:
/// https://github.com/facebook/folly/blob/master/folly/concurrent_spsc_queue.h
/// Original public domain version authored by Facebook
/// Modifications by Serge Aleynikov & Leonid Timochouk
//----------------------------------------------------------------------------
// Created: 2014-06-10
//----------------------------------------------------------------------------
/*
 ***** BEGIN LICENSE BLOCK *****

 This file is part of the utxx open-source project.

 Copyright (C) 2014 Serge Aleynikov  <saleyn@gmail.com>
 Copyright (C) 2014 Leonid Timochouk <l.timochouk@gmail.com>

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
#ifndef _UTXX_CONCURRENT_SPSC_QUEUE_HPP_
#define _UTXX_CONCURRENT_SPSC_QUEUE_HPP_

#include <utxx/math.hpp>
#include <utxx/compiler_hints.hpp>
#include <boost/noncopyable.hpp>
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace utxx {

//===========================================================================//
// concurrent_spsc_queue is a one producer and one consumer queue            //
// without locks.                                                            //
//===========================================================================//
template<class T, uint32_t StaticCapacity=0>
class concurrent_spsc_queue : private boost::noncopyable
{
    //=======================================================================//
    // Implementation:                                                       //
    //=======================================================================//
    // header (can also be located in ShMem along with the data):
    //
    struct header
    {
        std::atomic<uint32_t>  m_head;
        std::atomic<uint32_t>  m_tail;
        uint32_t    const      m_capacity;
        T                      __padding[0];

        static uint32_t adjust_capacity(uint32_t a_capacity)
        {
            uint32_t n = math::upper_power(a_capacity, 2);
            // If the result was not equal to the original "a_capacity", it
            // was rounded UP, and we need to round it DOWN:
            if (n != a_capacity)
            {
                assert(n > a_capacity);
                n /= 2;
            }
            return n;
        }

        header()
            : m_head    (0)
            , m_tail    (0)
            , m_capacity(0)
        {}

        header(uint32_t a_capacity)
            : m_head(0)
            , m_tail(0)
            , m_capacity(adjust_capacity(a_capacity))
        {
            assert((m_capacity & (m_capacity-1)) == 0);  // Power of 2 indeed
            if (m_capacity < 2)
                throw std::runtime_error
                    ("utxx::concurrent_spsc_queue: capacity must be a positive "
                     "power of 2");
        }
    };

    uint32_t increment(uint32_t h) const { return (h + 1) & m_mask; }

public:
    //=======================================================================//
    // External API: Synchronous Operations:                                 //
    //=======================================================================//
    typedef T value_type;

    /// @return memory size needed for allocating internal queue data.
    /// XXX: this does not tale into account that the actual capacity may be
    /// lower (a rounded-down power of 2):
    ///
    static uint32_t memory_size(uint32_t a_capacity)
      { return sizeof(header) + a_capacity * sizeof(T); }

    /// Non-Default Ctor for using external memory (e.g. shared memory)
    /// Size must be obtained by the call to memory_size(). Only enabled if the
    /// StaticCapacity is 0.
    /// NB: In this case, Arg2 is really a memory size in bytes, NOT the capac-
    /// ity which is the items count!
    concurrent_spsc_queue
    (
        void*    a_storage,
        uint32_t a_size,
        bool     a_is_producer
    )
        : m_header     ((a_size - sizeof(header)) / sizeof(T))
        , m_header_ptr (static_cast<header*>(a_storage))
        , m_rec_ptr    (reinterpret_cast<T*>(static_cast<char*>(a_storage) +
                                             sizeof(header)))
        , m_shared_data(true)
        , m_is_producer(a_is_producer)
        , m_mask       (m_header.m_capacity-1)
    {
        // Verify that the sizes are correct (as would indeed be the case if
        // "a_size" was computed by "memory_size" above):
        if (a_size <= sizeof(header) ||
           (a_size  - sizeof(header)) % sizeof(T) != 0)
            throw std::invalid_argument
                  ("utxx::concurrent_spsc_queue::Ctor: Invalid storage size");

        if (unlikely(StaticCapacity != 0))
            throw std::logic_error
                  ("utxx::concurrent_spsc_queue::Ctor: both static and dynamic "
                   "capacity are specified");
    }

    /// Non-Default Ctor with automatic memory allocation on the heap;
    /// capacity will be rounded to the nearest power of two up to or below
    /// the value specified in the arg. Enabled if the StaticCapacity is 0.
    /// Also, note that the number of usable slots in the queue at any given
    /// time is actually (\a capacity-1), so if you start with an empty queue,
    /// full() will return true after \a capacity-1 insertions.
    ///
    explicit concurrent_spsc_queue(uint32_t a_capacity)
        : m_header     (a_capacity)
        , m_header_ptr (&m_header)
        , m_rec_ptr    (reinterpret_cast<T*>
                       (::malloc(sizeof(T)*m_header.m_count)))
        , m_shared_data(false)
        , m_is_producer(false)   // irrelevant in this case
        , m_mask       (m_header.m_capacity-1)
    {
        if (unlikely(StaticCapacity != 0))
            throw std::logic_error
                  ("utxx::concurrent_spsc_queue::Ctor: both static and dynamic "
                   "capacity are specified");
    }

    /// Default Ctor:
    /// Uses the StaticCapacity template parameter. Always enabled (as there
    /// are no typed arguments), but if StaticCapacity is 0, using this Ctor
    /// will result in a run-time error:
    ///
    concurrent_spsc_queue()
        : m_header     (StaticCapacity)
        , m_header_ptr (&m_header)
        , m_rec_ptr    (&m_records)
        , m_shared_data(false)
        , m_is_producer(false)  // irrelevant in this case
        , m_mask       (m_header.m_capacity-1)
    {}

    /// Dtor:
    /// We need to destruct anything that may still exist in our queue,   XXX
    /// even if the queue space is shared, but only if our role is "consumer"
    /// XXX: no synchronisation is performed at Dtor time, so it is a rerspon-
    /// sibility of the caller to ensure that the Producer is not trying to
    /// write into the queue at the same time:
    ///
    ~concurrent_spsc_queue()
    {
      if (!std::is_trivially_destructible<T>::value &&
          !(m_shared_data && m_is_producer))
          for (uint32_t read = head(), end  = tail();
               read != end;    read = increment(read))
              m_rec_ptr[read].~T();

      // If the memory allocation is dynamic (ie non-shared, non-static), then
      // de-allocate the storage space:
      if (StaticCapacity == 0 && !m_shared_data)
      {
        assert(m_rec_ptr != NULL && m_rec_ptr != &m_records);
        ::free(m_rec_ptr);
      }
    }

    /// Write a T object constructed with the "recordArgs" to the queue.
    /// @return the ptr to the installed entry on success, or NULL when
    /// the queue is full:
    /// NB: In particular, this allows us to write a pre-constructed object
    /// into the queue using a Copy or Move ctor:
    ///
    template<class ...Args>
    T* push(Args&&... recordArgs)
    {
        // NB: the side check is for ShM only, and in the debug mode only:
        // must be on the Producer side:
        assert(!m_shared_data || m_is_producer);

        uint32_t t    = tail().load(std::memory_order_relaxed);
        uint32_t next = increment(t);

        if (next != head().load(std::memory_order_acquire))
        {
            T* at = m_rec_ptr + t;
            new (at) T(std::forward<Args>(recordArgs)...);
            tail().store(next, std::memory_order_release);
            return at;
        }
        // Otherwise: queue is full, nothing is 
        return NULL;
    }

    /// Move (or copy) the value at the front of the queue to given variable
    bool pop(T* record)
    {
        // NB: the side check is for ShM only, and in the debug mode only:
        // must be on the Consumer side:
        assert(record != NULL && (!m_shared_data || !m_is_producer));

        uint32_t h = head().load(std::memory_order_relaxed);
        if (h == tail().load(std::memory_order_acquire))
            // queue is empty:
            return false;

        uint32_t next = increment(h);
        *record = std::move(m_rec_ptr[h]);
        m_rec_ptr[h].~T();
        head().store(next, std::memory_order_release);
        return true;
    }

    /// Pointer to the value at the front of the queue (for use in-place) or
    /// nullptr if empty.
    T* peek()
    {
        // NB: the side check is for ShM only, and in the debug mode only:
        // must be on the Consumer side:
        assert(!m_shared_data || !m_is_producer);

        uint32_t h = head().load(std::memory_order_relaxed);
        return
            (h == tail().load(std::memory_order_acquire))
            ? nullptr /* queue is empty */
            : (m_rec_ptr + h);
    }

    /// Pointer to the value at the front of the queue (for use in-place) or
    /// nullptr if empty.
    T const* peek() const
    {
        // NB: the side check is for ShM only, and in the debug mode only:
        // must be on the Consumer side:
        assert(!m_shared_data || !m_is_producer);

        uint32_t h = head().load(std::memory_order_relaxed);
        return
            (h == tail().load(std::memory_order_acquire))
            ? nullptr /* queue is empty */
            : (m_rec_ptr + h);
    }

    /// Pop an element from the front of the queue.
    /// Queue must not be empty!
    void pop()
    {
        // NB: the side check is for ShM only, and in the debug mode only:
        // must be on the Consumer side:
        assert(!m_shared_data || !m_is_producer);

        uint32_t h = head().load(std::memory_order_relaxed);
        assert(h  != tail().load(std::memory_order_acquire));

        uint32_t next = increment(h);
        m_rec_ptr[h].~T();
        head().store(next, std::memory_order_release);
    }

    /// UNSAFE test for the queue being empty (because == is not synchronised):
    bool empty() const
    {
        return head().load(std::memory_order_consume)
            == tail().load(std::memory_order_consume);
    }

    /// UNSAFE test for the queue begin full (because == is not synchronised):
    bool full() const
    {
        uint32_t next =
             increment(tail().load(std::memory_order_consume));
        return next == head().load(std::memory_order_consume);
    }

    /// UNSAFE current count of T objects stored in the queue:
    /// If called by consumer, then true count may be more (because producer may
    ///   be adding items concurrently).
    /// If called by producer, then true count may be less (because consumer may
    ///   be removing items concurrently).
    /// It is undefined to call this from any other thread:
    ///
    uint32_t count() const
    {
        int ret = int(tail().load(std::memory_order_consume))
                - int(head().load(std::memory_order_consume));
        if (ret < 0)
            ret += m_header_ptr->m_capacity;
        assert(ret >= 0);
        return uint32_t(ret);
    }

    //=======================================================================//
    // UNSAFE iterators over the queue:                                      //
    //=======================================================================//
    // No locking is performed by this class, it is a responsibility of the
    // caller!
    //-----------------------------------------------------------------------//
    // The "iterator" class:                                                 //
    //-----------------------------------------------------------------------//
    class iterator
    {
    private:
        uint32_t                  m_ind;
        concurrent_spsc_queue<T>* m_queue;

        // Non-default Ctor is available for use from the outer class only:
        friend class concurrent_spsc_queue<T>;

        iterator(uint32_t ind, concurrent_spsc_queue<T>* queue)
            : m_ind  (ind),
            m_queue(queue)
        {}

    public:
        // Default Ctor: creates an invalid "iterator":
        iterator()
            : m_ind  (0),
            m_queue(nullptr)
        {};
        // Dtor, copy ctor, assignemnt and equality are auto-generated

        // Increment: XXX: no checks are performed on whether the iterator is
        // valid:
        iterator& operator++() { m_ind = m_queue->increment(m_ind); }

        // De-referencing:
        T* operator->() const { return m_queue->m_rec_ptr + m_ind;  }
        T& operator*()  const { return m_queue->m_rec_ptr  [m_ind]; }
    };

    //-----------------------------------------------------------------------//
    // The "const_iterator" class:                                           //
    //-----------------------------------------------------------------------//
    struct const_iterator
    {
    private:
        uint32_t                        m_ind;
        concurrent_spsc_queue<T> const* m_queue;

        // Non-default Ctor is available for use from the outer class only:
        friend class concurrent_spsc_queue<T>;

        const_iterator(uint32_t ind, concurrent_spsc_queue<T> const* queue)
            : m_ind  (ind),
            m_queue(queue)
        {}

    public:
        // Default Ctor: creates an invalid "const_iterator":
        const_iterator()
            : m_ind  (0),
            m_queue(nullptr)
        {};

        // Dtor, copy ctor, assignemnt and equality are auto-generated

        // Increment: XXX: no checks are performed on whether this
        // const_iterator is valid:
        const_iterator& operator++() { m_ind = m_queue->increment(m_ind); }

        // De-referencing:
        T const* operator->() const { return m_queue->m_rec_ptr + m_ind;  }
        T const& operator*()  const { return m_queue->m_rec_ptr  [m_ind]; }
    };

    //-----------------------------------------------------------------------//
    // "*begin" and "*end" iterators:                                        //
    //-----------------------------------------------------------------------//
    // Iterating goes from the oldest to the most recent items, ie from front
    // to back, ie from head (where the items are popped from) to tail (where
    // the items are pushed into):
    //
    // "begin" is non-const because the queue can be modified via the iterator
    // returned:
    iterator begin()
        { return iterator(head().load(), this); }

    // "cbegin":
    const_iterator cbegin() const
        { return iterator(head().load(), this); }

    // "end" is non-const for same reason as "begin":
    iterator end()
        { return iterator(tail().load(), this); }

    // "cend":
    const_iterator cend() const
        { return iterator(tail().load(), this); }

private:
    //=======================================================================//
    // Data Flds:                                                            //
    //=======================================================================//
    header          m_header;
    header*  const  m_header_ptr;   // Ptr to the actual hdr  (mb to m_header)
    T*       const  m_rec_ptr;      // Ptr to the actual data (mb to m_records)
    bool     const  m_shared_data;
    bool     const  m_is_producer;  // Only relevant if "m_shared_data" is set
    uint32_t const  m_mask;
    T               m_records[StaticCapacity];

    //-----------------------------------------------------------------------//
    // Accessors (for internal use only):                                    //
    //-----------------------------------------------------------------------//
    // NB: "head" is the actual front of the queue (where items are consumed);
    //     "tail" is beyond the last stored entry, so a new item would be app-
    //     ended at the curr "tail":
    std::atomic<uint32_t>&       head()       { return m_header_ptr->m_head; }
    std::atomic<uint32_t>&       tail()       { return m_header_ptr->m_tail; }
    std::atomic<uint32_t> const& head() const { return m_header_ptr->m_head; }
    std::atomic<uint32_t> const& tail() const { return m_header_ptr->m_tail; }
};

} // namespace utxx

#endif //_UTXX_CONCURRENT_SPSC_QUEUE_HPP_
