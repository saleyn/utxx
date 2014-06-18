//----------------------------------------------------------------------------
/// \file   concurrent_spsc_queue.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Producer/consumer queue.
///
/// Based on code from:
/// https://github.com/facebook/folly/blob/master/folly/concurrent_spsc_queue.h
/// Original public domain version authored by Facebook
/// Modifications by Serge Aleynikov
//----------------------------------------------------------------------------
// Created: 2014-06-10
//----------------------------------------------------------------------------
/*
 ***** BEGIN LICENSE BLOCK *****

 This file is part of the utxx open-source project.

 Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <boost/noncopyable.hpp>
#include <boost/type_traits.hpp>
#include <utxx/math.hpp>

namespace utxx {

/*
 * concurrent_spsc_queue is a one producer and one consumer queue
 * without locks.
 */
template<class T>
class concurrent_spsc_queue : private boost::noncopyable {
    struct header {
        std::atomic<int>  head;
        std::atomic<int>  tail;
        uint32_t const    size;
        uint32_t          __padding;

        static size_t calc_size(size_t a_size) {
            size_t n = math::upper_power(a_size, 2);
            if (n != a_size) n /= 2;
            return n;
        }

        header() {}
        header(size_t a_size)
            : size(calc_size(a_size))
            , head(0)
            , tail(0)
        {
            assert((size & (size-1)) == 0); // Power of 2.
            if (size < 2)
                throw std::runtime_error
                    ("utxx::concurrent_spsc_queue: size must be a power of 2");
        }
    };

    concurrent_spsc_queue(header* a_header, void* a_storage, uint32_t a_size, bool a_own)
        : m_header_data(a_size)
        , m_header(a_header)
        , m_records(a_own
            ? reinterpret_cast<T*>(::malloc(sizeof(T)*m_header_data.size))
            : static_cast<T*>(a_storage))
        , m_own_data(a_own)
        , m_mask(m_header_data.size - 1)
    {
        new (m_header) header(a_size);
    }

public:
    typedef T value_type;

    /// @return memory size needed for allocating internal queue data
    static size_t memory_size(size_t a_item_count) {
        return sizeof(header) + a_item_count * sizeof(T);
    }

    // Ctor for using external memory (e.g. shared memory)
    // Size must be obtained by the call to memory_size().
    concurrent_spsc_queue(void* a_storage, uint32_t a_size)
        : concurrent_spsc_queue(
            static_cast<header*>(a_storage),
            reinterpret_cast<T*>(static_cast<char*>(a_storage) + sizeof(m_header)),
            a_size    - sizeof(header),
            false
          )
    {}

    // size will be rounded to the nearest power of two up to or below \a a_size.
    //
    // Also, note that the number of usable slots in the queue at any
    // given time is actually (\a a_size-1), so if you start with an empty queue,
    // full() will return true after \a a_size-1 insertions.
    explicit concurrent_spsc_queue(uint32_t a_size)
        : concurrent_spsc_queue(&m_header_data, nullptr, a_size, true)
    {}

    ~concurrent_spsc_queue() {
        if (m_own_data) {
            // We need to destruct anything that may still exist in our queue.
            // (No real synchronization needed at destructor time: only one
            // thread can be doing this.)
            if (!boost::has_trivial_destructor<T>::value)
                for (int read = head(), end = tail(); read != end; read = (read + 1) & m_mask)
                    m_records[read].~T();

            if (m_records)
                ::free(m_records);
        }
    }

    /// Write aruments to the queue.
    /// @return true on success, false when the queue is full
    template<class ...Args>
    bool push(Args&&... recordArgs) {
        const int t = tail().load(std::memory_order_relaxed);
        const int next = (t + 1) & m_mask;
        if (next != head().load(std::memory_order_acquire)) {
            new (&m_records[t]) T(std::forward<Args>(recordArgs)...);
            tail().store(next, std::memory_order_release);
            return true;
        }

        // queue is full
        return false;
    }

    /// Move (or copy) the value at the front of the queue to given variable
    bool pop(T& record) {
        const int h = head().load(std::memory_order_relaxed);
        if (h == tail().load(std::memory_order_acquire)) {
            // queue is empty
            return false;
        }

        const int next = (h + 1) & m_mask;
        record = std::move(m_records[h]);
        m_records[h].~T();
        head().store(next, std::memory_order_release);
        return true;
    }

    // Pointer to the value at the front of the queue (for use in-place) or
    // nullptr if empty.
    T* peek() {
        const int h = head().load(std::memory_order_relaxed);
        return    h == tail().load(std::memory_order_acquire)
            ? nullptr /* queue is empty */
            : &m_records[h];
    }

    // Pointer to the value at the front of the queue (for use in-place) or
    // nullptr if empty.
    T* peek() const {
        const int h = head().load(std::memory_order_relaxed);
        return    h == tail().load(std::memory_order_acquire)
            ? nullptr /* queue is empty */
            : &m_records[h];
    }

    /// Pop an element from the front of the queue.
    ///
    /// Queue must not be empty!
    void pop() {
        const int h = head().load(std::memory_order_relaxed);
        assert(h != tail().load(std::memory_order_acquire));

        size_t next = (h + 1) & m_mask;
        m_records[h].~T();
        head().store(next, std::memory_order_release);
    }

    bool empty() const {
        return head().load(std::memory_order_consume)
            == tail().load(std::memory_order_consume);
    }

    bool full() const {
        const int next = (tail().load(std::memory_order_consume) + 1) & m_mask;
        return    next == head().load(std::memory_order_consume);
    }

    // * If called by consumer, then true size may be more (because producer may
    //   be adding items concurrently).
    // * If called by producer, then true size may be less (because consumer may
    //   be removing items concurrently).
    // * It is undefined to call this from any other thread.
    size_t unsafe_size() const {
        int ret = tail().load(std::memory_order_consume)
                - head().load(std::memory_order_consume);
        return ret < 0 ? ret + size() : ret;
    }

private:
    header       m_header_data;
    header*      m_header;
    T* const     m_records;
    bool         m_own_data;
    size_t const m_mask;

    size_t                  size() const { return m_header->size; }
    std::atomic<int>&       head()       { return m_header->head; }
    std::atomic<int>&       tail()       { return m_header->tail; }
    const std::atomic<int>& head() const { return m_header->head; }
    const std::atomic<int>& tail() const { return m_header->tail; }
};

} // namespace utxx

#endif //_UTXX_CONCURRENT_SPSC_QUEUE_HPP_
