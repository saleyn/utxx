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

#ifndef NDEBUG
#include <utxx/string.hpp>
#endif

namespace utxx {

//===========================================================================//
// concurrent_spsc_queue is a one producer and one consumer queue            //
// without locks.                                                            //
//===========================================================================//
template<class T, uint32_t StaticCapacity=0>
class concurrent_spsc_queue : private boost::noncopyable
{
private:
    //=======================================================================//
    // Implementation:                                                       //
    //=======================================================================//
    //-----------------------------------------------------------------------//
    // Header (can also be located in ShMem along with the data):            //
    //-----------------------------------------------------------------------//
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
                    ("utxx::concurrent_spsc_queue: invalid capacity=" +
                     std::to_string(m_capacity));
        }
    };

    //-----------------------------------------------------------------------//
    // Index increment and decrement, with roll-over:                        //
    //-----------------------------------------------------------------------//
    uint32_t increment(uint32_t h, int val = 1) const
      { return (h + val) & m_mask; }

    uint32_t decrement(uint32_t h, int val = 1) const
      { return (h - val) & m_mask; }

public:
    //=======================================================================//
    // External API: Synchronous Operations:                                 //
    //=======================================================================//
    typedef T value_type;

    /// @return memory size needed for allocating internal queue data.
    /// Note that the actual capacity may be lower (a rounded-down power of 2).
    static uint32_t memory_size(uint32_t a_capacity)
      { return sizeof(header) + a_capacity * sizeof(T); }

    /// Role: Producer / Consumer / Both (eg in a single-threaded testing mode):
    enum class side_t
    {
        invalid  = 0,
        producer = 1,
        consumer = 2,
        both     = 3
    };

    //-----------------------------------------------------------------------//
    // Ctors, Dtor:                                                          //
    //-----------------------------------------------------------------------//
    /// Non-Default Ctor for using external memory (eg shared memory).
    /// Size must be obtained by the call to memory_size(). Only enabled if the
    /// StaticCapacity is 0.
    /// NB: In this case, Arg2 is really a memory size in bytes, NOT the capac-
    /// ity which is the items count!
    concurrent_spsc_queue
    (
        void*    a_storage,
        uint32_t a_size,
        side_t   a_side
    )
        : m_header     ((a_size - sizeof(header)) / sizeof(T))
        , m_header_ptr (static_cast<header*>(a_storage))
        , m_rec_ptr    (reinterpret_cast<T*>(static_cast<char*>(a_storage) +
                                             sizeof(header)))
        , m_shared_data(true)
        , m_side       (a_side)
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
    /// XXX: Because the "side" cannot be made thread-local here as yet, we
    /// have to set it to "side_t::both":
    ///
    explicit concurrent_spsc_queue(uint32_t a_capacity)
        : m_header     (a_capacity)
        , m_header_ptr (&m_header)
        , m_rec_ptr    (reinterpret_cast<T*>
                       (::malloc(sizeof(T)*m_header.m_capacity)))
        , m_shared_data(false)
        , m_side       (side_t::both)
        , m_mask       (m_header.m_capacity-1)
    {
        if (unlikely(StaticCapacity != 0))
            throw std::logic_error
                  ("utxx::concurrent_spsc_queue::Ctor: both static and dynamic "
                   "capacity are specified");
    }

    /// Default Ctor which uses the "StaticCapacity" template parameter. If
    /// StaticCapacity is 0, using this Ctor will result in a run-time error.
    /// Again, "side" has to be set to "both" here (unless we can make it
    /// thread-local):
    ///
    concurrent_spsc_queue()
        : m_header     (StaticCapacity)
        , m_header_ptr (&m_header)
        , m_rec_ptr    ((T*)(&m_records)) // (T*) needed for StaticCapacity==0
        , m_shared_data(false)
        , m_side       (side_t::both)
        , m_mask       (m_header.m_capacity-1)
    {}

    /// Dtor:
    /// We need to destruct anything that may still exist in our queue,   XXX
    /// even if the queue space is shared, but only if our side is "consumer"
    /// XXX: no synchronisation is performed at Dtor time, so it is a rerspon-
    /// sibility of the caller to ensure that the Producer is not trying to
    /// write into the queue at the same time:
    ///
    ~concurrent_spsc_queue()
    {
        // If the data are shared, don't clear or de-allocate the queue: it may
        // (or may not) need to be persistent, so its lifetime is managed by
        // the callers:
        if (m_shared_data)
            return;

        // Otherwise: If necessary, invoke the Dtors on the stored contents (but
        // pass a flag indicating that we are calling "clear" from the Dtor, so
        // it will not check which side we are on):
        clear(true);

        // If the memory allocation is dynamic (ie non-shared and non-static),
        // then de-allocate the storage space:
        if (StaticCapacity == 0)
        {
            assert(m_rec_ptr != NULL && m_rec_ptr != m_records);
            ::free(m_rec_ptr);
        }
    }

    //-----------------------------------------------------------------------//
    // Data Push / Pop / Peek operations:                                    //
    //-----------------------------------------------------------------------//
    /// Write a T object constructed with the "recordArgs" to the queue.
    /// @return the ptr to the installed entry on success, or NULL when
    /// the queue is full:
    /// NB: In particular, this allows us to write a pre-constructed object
    /// into the queue using a Copy or Move ctor:
    ///
    template<class ...Args>
    T* push(Args&&... recordArgs)
    {
        // Must NOT be on the Consumer side:
        assert(m_side != side_t::consumer);

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
        // Must NOT be on the Producer side:
        assert(m_side != side_t::producer && record != NULL);

        uint32_t h = head().load(std::memory_order_relaxed);
        if (h == tail().load(std::memory_order_acquire))
            // queue is empty:
            return false;

        uint32_t next = increment(h);
        *record = std::move(m_rec_ptr[h]);
        if (!std::is_trivially_destructible<T>::value)
            m_rec_ptr[h].~T();
        head().store(next, std::memory_order_release);
        return true;
    }

    /// Pop an element from the front of the queue.
    /// Queue must not be empty!
    void pop()
    {
        // Must NOT be on the Producer side:
        assert(m_side != side_t::producer);

        uint32_t h = head().load(std::memory_order_relaxed);
        assert(h  != tail().load(std::memory_order_acquire));

        uint32_t next = increment(h);
        if (!std::is_trivially_destructible<T>::value)
            m_rec_ptr[h].~T();
        head().store(next, std::memory_order_release);
    }

    /// Pointer to the value at the front of the queue (for use in-place) or
    /// nullptr if empty.
    T const* peek() const
    {
        // NB: the side check is for ShM only, and in the debug mode only:
        // must NOT be on the Producer side:
        assert(m_side != side_t::producer);

        uint32_t h = head().load(std::memory_order_relaxed);
        return
            (h == tail().load(std::memory_order_acquire))
            ? nullptr    // queue is empty
            : (m_rec_ptr + h);
    }

    /// Pointer to the value at the front of the queue (for use in-place) or
    /// nullptr if empty.
    T* peek()
    {
        return const_cast<T*>
              (const_cast<concurrent_spsc_queue const*>(this)->peek());
    }

    /// Clear: Remove all entries from the queue. Only safe if invoked on the
    /// Consumer side:
    void clear(bool force = false)
    {
        // Unless the "force" mode is set (eg invoked from the Dtor), we must
        // NOT be on the Producer side:
        assert(force || m_side != side_t::producer);

        if (std::is_trivially_destructible<T>::value)
            head().store
                (tail().load(std::memory_order_acquire),
                 std::memory_order_release);
        else
            // Have to do it by-one so the Dtor is called every time:
            while (!empty())
                pop();
    }

    //-----------------------------------------------------------------------//
    // Queue Status:                                                         //
    //-----------------------------------------------------------------------//
    /// Test for the queue being empty, safe if invoked from the consumer side:
    bool empty() const
    {
        assert(m_side != side_t::producer);
        return head().load(std::memory_order_relaxed)
            == tail().load(std::memory_order_acquire);
    }

    /// Test for the queue begin full, safe if invoked from the producer side.
    /// Physically, the queue is not completely full yet when full() returns
    /// "true" -- there is still an empty entry at tail, but it cannot be used
    /// as otherwise we would not be able to distinguish between "empty" and
    /// "full" states:
    bool full() const
    {
        assert(m_side != side_t::consumer);
        uint32_t next =  increment(tail().load(std::memory_order_relaxed));
        return   next == head().load(std::memory_order_acquire);
    }

    /// UNSAFE current count of T objects stored in the queue:
    /// If called by consumer, then true count may be more (because producer may
    ///   be adding items concurrently).
    /// If called by producer, then true count may be less (because consumer may
    ///   be removing items concurrently).
    /// It is undefined to call this from any other thread:
    /// NB: Here the caller can explicitly provide their Role for better preci-
    ///   sion:
    uint32_t count(side_t side = side_t::invalid) const
    {
        if (side == side_t::invalid)
            // Side arg not specified, use the recoded one:
            side = m_side;

        // If we are a Producer (or Both), can use "relaxed" memory order on the
        // side under our own control (tail):
        int tn = int(tail().load
                       ((side != side_t::consumer)
                        ? std::memory_order_relaxed
                        : std::memory_order_consume));

        // Similarly, if we are a Consumer (or Both), can use "relaxed" memory
        // order on the head side which is under our own control:
        int hn = int(head().load
                        ((side != side_t::producer)
                         ? std::memory_order_relaxed
                         : std::memory_order_consume));
                                  
        int ret = tn - hn;
        if (ret < 0)
            ret += m_header_ptr->m_capacity;
        assert(ret >= 0);
        return uint32_t(ret);
    }

    /// Queue Capacity (static or dynamic):
    uint32_t capacity() const { return m_header.m_capacity; }

    //=======================================================================//
    // UNSAFE iterators over the queue:                                      //
    //=======================================================================//
    // No locking is performed by this class, it is a responsibility of the
    // caller!
private:
    //-----------------------------------------------------------------------//
    // "iterator_gen":                                                       //
    //-----------------------------------------------------------------------//
    // The base class providing common functionality for "iterator", "reverse_
    // iterator", "const_iterator" and "const_reverse_iterator" below:
    //
    template<bool IsConst, bool IsReverse>
    friend class  iterator_gen;

    template<bool IsConst, bool IsReverse>
    class iterator_gen
    {
    private:
        uint32_t                             m_ind;
        typename std::conditional
          <IsConst,
           concurrent_spsc_queue const*,
           concurrent_spsc_queue*
          >::type                            m_queue;

        // Iterator verification: chec whether the curr iteratir is suitable
        // for de-referencing and as a base of arithmetic operations (though
        // results of such operations may of course become invalid):
        //
        void verify(char const* where) const
        {
#       ifndef NDEBUG
            assert(m_queue != NULL);
            // XXX: If enabled, "verify" is currently NOT optimised wrt the
            // side (producer or consumer) -- using the same generic "consume"
            // memory order (NB: it is NOT related to the consumer side!):
            //
            uint32_t h = m_queue->head().load(std::memory_order_consume);
            uint32_t t = m_queue->tail().load(std::memory_order_consume);
            assert(h < m_queue->capacity() && t < m_queue->capacity());

            // The following is OK:
            // h <= m_ind < t ||
            // (t < h) && (m_ind < t  || h <= m_ind)
            //
            // So the following is NOT OK:
            // (h == t)                                ||
            // (h  < t) && (m_ind < h || t <= m_ind)   ||
            // (t  < h) && (t <= m_ind < h)
            //
            if ((h == t)                               ||
               ((h  < t) && (m_ind < h || t <= m_ind)) ||
               ((t  < h) && (t <= m_ind) && (m_ind < h)))
                throw std::runtime_error
                      (to_string("concurrent_spsc_queue::iterator_gen::verify "
                                 "FAILED: ", where, ": head=", h, ", tail=", t,
                                 ", ind=", m_ind));
#       endif
        }

        // This ctor is only visible from the outer class which is made a "fri-
        // end":
        friend class concurrent_spsc_queue;

        // NB: Do NOT invoke "verify" here as this Ctor may very well produce
        // an invalid iterator (end(), cend(), rend(), crend()):
        iterator_gen
        (
            uint32_t                         ind,
            typename std::conditional
              <IsConst,
               concurrent_spsc_queue const*,
               concurrent_spsc_queue*
              >::type                        queue
        )
            : m_ind(ind),
            m_queue(queue)
        {}

    public:
        // NB: The following ctor requires that "entry" and "queue" must be
        // valid non-NULL ptrs:
        iterator_gen
        (
            typename std::conditional<IsConst, T const*, T*>::type entry,
            typename std::conditional
              <IsConst, concurrent_spsc_queue const*,
                        concurrent_spsc_queue*>::type              queue
        )
            : m_ind(entry - queue->m_rec_ptr),
            m_queue(queue)
        {
            verify("iterator_gen::Ctor(entry*,queue)");
        }

        // Default Ctor: creates an invalid "iterator":
        iterator_gen()
            : m_ind(0),
            m_queue(nullptr)
        {}

        // De-referencing:
        typename std::conditional<IsConst, T const*, T*>::type
        operator->() const
        {
            verify("operator->");
            return m_queue->m_rec_ptr + m_ind;
        }

        typename std::conditional<IsConst, T const&, T&>::type
        operator*()  const
        {
            verify("operator*");
            return m_queue->m_rec_ptr  [m_ind];
        }

        // Equality: NB: Verification is not required:
        bool operator==(iterator_gen const& right) const
            { return m_ind == right.m_ind && m_queue == right.m_queue; }

        bool operator!=(iterator_gen const& right) const
            { return m_ind != right.m_ind || m_queue != right.m_queue; }

        // Increment / Decrement:
        // XXX: no checks are performed on whether the iterator is valid;
        // (++): in case of IsReverse, the underlying index is actually
        //       DECREMENTED;
        // (--): inverse behaviour wrt (++):
        //
        iterator_gen& operator++()
        {
            verify("operator++");
            m_ind =
                IsReverse
                ? m_queue->decrement(m_ind)
                : m_queue->increment(m_ind);
            return *this;
        }

        iterator_gen& operator+=(int val)
        {
            verify("operator+=");
            m_ind =
                IsReverse
                ? m_queue->decrement(m_ind, val)
                : m_queue->increment(m_ind, val);
            return *this;
        }

        iterator_gen operator+(int val) const
        {
            verify("operator+");
            iterator_gen res = *this;
            res += val;
            return res;
        }

        iterator_gen& operator--()
        {
            verify("operator--");
            m_ind =
                IsReverse
                ? m_queue->increment(m_ind)
                : m_queue->decrement(m_ind);
            return *this;
        }

        iterator_gen& operator-=(int val)
        {
            verify("operator-=");
            m_ind =
                IsReverse
                ? m_queue->increment(m_ind, val)
                : m_queue->decrement(m_ind, val);
            return *this;
        }

        iterator_gen operator-(int val) const
        {
            verify("operator-");
            iterator_gen res = *this;
            res -= val;
            return res;
        }
    };

    //-----------------------------------------------------------------------//
    // "begend": for "*begin" and "*end" iterator values:                    //
    //-----------------------------------------------------------------------//
    // Iterating goes from the oldest to the most recent items, ie from front
    // to back, ie from head (where the items are popped from) to tail (where
    // the items are pushed into):
    //   begin = head,   end  = tail;    and
    //  rbegin = tail-1, rend = head-1:
    // XXX: The method is marked "const" so it can always be invoked from both
    // const and non-const user-visible methods:
    //
    template<bool IsConst, bool IsReverse, bool IsBegin>
    iterator_gen <IsConst, IsReverse> begend(side_t side)
    const
    {
        if (side == side_t::invalid)
            // Side arg not specified, use the recoded one:
            side = m_side;

        // Here again, the memory order depends on which side we are on:
        // for tail, can use "relaxed" if Producer or Both (ie !Consumer);
        // for head, can use "relaxed" if Consumer or Both (ie !Producer):
        //
        bool constexpr IsHead = IsReverse ^ IsBegin;
        std::memory_order ord =
            (( IsHead && (side != side_t::producer)) ||
             (!IsHead && (side != side_t::consumer)))
            ? std::memory_order_relaxed
            : std::memory_order_consume;

        uint32_t ind = (IsHead ? head() : tail()).load(ord);
        if (IsReverse)
            ind = decrement(ind);

        return iterator_gen<IsConst, IsReverse>
            (
             ind,
             // NB: As this method is a "const" one, "this" ptr is a "const" ptr
             // as well. This is OK for "const" iterators (when IsConst is set);
             // otherwise, we need to cast it into a non-const ptr. What we act-
             // ually do, is cast it in any case -- if IsConst is set, it is au-
             // tomatically (and safely) cast back to "const":
             //
             const_cast<concurrent_spsc_queue*>(this)
            );
    }

  public:
    //-----------------------------------------------------------------------//
    // Client-Visible Iterators and Their Limiting Values:                   //
    //-----------------------------------------------------------------------//
    // NB: Iterators are "friends"  of "concurrent_spsc_queue" (and conversely,
    // the latter is a "friend" of "iterator_gen" -- see above):
    //
    // "iterator":               IsConst=false, IsReverse=false:
    //
    typedef iterator_gen<false, false> iterator;

    iterator begin(side_t side = side_t::invalid)
        { return begend<false, false, true> (side); }

    iterator end  (side_t side = side_t::invalid)
        { return begend<false, false, false>(side); }

    // "const_iterator":         IsConst=true,  IsReverse=false:
    //
    typedef iterator_gen<true,  false> const_iterator;

    const_iterator cbegin(side_t side = side_t::invalid) const
        { return begend<true,  false, true> (side); }

    const_iterator cend  (side_t side = side_t::invalid) const
        { return begend<true,  false, false>(side); }

    // "reverse_iterator":       IsConst=false, IsReverse=true:
    //
    typedef iterator_gen<false, true>  reverse_iterator;

    reverse_iterator rbegin(side_t side = side_t::invalid)
        { return begend<false, true,  true> (side); }

    reverse_iterator rend  (side_t side = side_t::invalid)
        { return begend<false, true,  false>(side); }

    // "const_reverse_iterator": IsConst=true,  IsReverse=true:
    //
    typedef iterator_gen<true,  true>  const_reverse_iterator;

    const_reverse_iterator crbegin(side_t side = side_t::invalid) const
        { return begend<true,  true,  true> (side); }

    const_reverse_iterator crend  (side_t side = side_t::invalid) const
        { return begend<true,  true,  false>(side); }

    //-----------------------------------------------------------------------//
    // "erase":                                                              //
    //-----------------------------------------------------------------------//
    /// Remove the entry specified by the iterator, which must be a valid one
    /// (NOT *end). No explicit validity checks on the iterator are performed.
    /// This method is safe only on the Consumer side:
    ///
    template<bool IsReverse>
    void erase(iterator_gen<false, IsReverse> const& it)
    {
        it.verify("erase");
        assert(m_side != side_t::producer);
        if (utxx::unlikely(it.m_queue != this))
            throw std::invalid_argument
                  ("concurrent_spsc_queue::erase: Invalid Iterator");

        // But if we are on the right side, can use the "relaxed" memory order:
        uint32_t h = head().load(std::memory_order_relaxed);

        // Shift the data items backwards, freeing the front:
        for (uint32_t i = it.m_ind; i != h; )
        {
            uint32_t  p = decrement(i);
            m_rec_ptr[i] = m_rec_ptr[p];
            i = p;
        }
        pop();
        // NB: The iteratir itself is NOT invalidated, and now points to ano-
        // ther data item (or the end...)
    }

    //-----------------------------------------------------------------------//
    // "set_side":                                                           //
    //-----------------------------------------------------------------------//
    // XXX: This is currenytly only possible if shared data are used. A valid
    // use case is altering one's side after a process "fork":
    //
    void set_side(side_t side)
    {
        if (utxx::unlikely(!m_shared_data || side == side_t::invalid))
            throw std::logic_error
                  ("concurrent_spsc_queue::set_side: Side must be valid, and "
                   "only allowed with Shared Data");
        m_side = side;
    }

private:
    //=======================================================================//
    // Data Flds:                                                            //
    //=======================================================================//
    header          m_header;
    header*  const  m_header_ptr;   // Ptr to the actual hdr  (mb to m_header)
    T*       const  m_rec_ptr;      // Ptr to the actual data (mb to m_records)
    bool     const  m_shared_data;
    side_t          m_side;
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
