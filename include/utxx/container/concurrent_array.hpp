//----------------------------------------------------------------------------
/// \file   concurrent_array.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Concurrent array with templetized locking primitive.
/// 
/// The concurrent_array class implemented in this module permits
/// concurrent access to its elements.  The access is guarded by a number of 
/// locks (defaults to synch::spin_lock) that are balanced by the item's index.
/// All access happens by either by copying data in and out of array's item 
/// (which is a good choice for small items) or by returning a reference to 
/// an item with the associated scoped lock.
///
/// The array doesn't own any memory - storage is provided in the constructor.
/// Consequently it can be used for stack, heap and shared memory placement.
//----------------------------------------------------------------------------
// Created: 2009-11-25
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
#ifndef _UTXX_CONCURRENT_ARRAY_HPP_
#define _UTXX_CONCURRENT_ARRAY_HPP_

#include <boost/tuple/tuple.hpp>
#include <boost/static_assert.hpp>
#include <utxx/meta.hpp>

#ifndef CACHELINE_SIZE
#  define CACHELINE_SIZE 64
#endif

namespace utxx {
namespace container {

/** 
 * Implements a concurrent array with an ability to have fine-grain
 * locking over a managed set of <LocksCount> entries.
 * Template arguments:
 *      T           - type of array's element
 *      Lock        - lock primitive to use for safe concurrent access
 *      LocksCount  - number of locks balanced accross access to array's items
 *      CacheAlign  - Perform cacheline alignment of managed locks
 */
template <
      class T
    , class Lock        = synch::spin_lock
    , int   LocksCount  = 16                // must be power of 2 (>=1)
    , bool  CacheAlign  = false
>
class concurrent_array {
    typedef concurrent_array<T, SyncSlots, CacheAlign> self_t;
    static const unsigned int max_lock_num = LocksCount-1;

    struct header {
        Lock lock[LocksCount];
        char __pad[CacheAlign ? CACHELINE_SIZE - sizeof(Lock) : 0];

        const size_t size;

        header(size_t sz) : size(sz) {}
    };
    
    header      m_header;
    T*          m_data;

    concurrent_array(void* data, size_t n_items)
        : m_header(n_items), m_data(data)
    {}

public:
    typedef Lock lock_type;
    typedef T    value_type;

    static self_t& create(void* storage, size_t sz, size_t n_items) {
        const size_t expected_size = sizeof(self_t) + n_items*sizeof(T);
        if (expected_size < sz)
            THROW_RUNTIME_ERROR(
                "Storage pool too small (expected " << expected_size << ')');
        STATIC_CAST((LocksCount & (LocksCount-1)) == 0, Must_be_power_of_2);

        char* data = static_cast<char*>(storage) + sizeof(header);
        new (storage) concurrent_array(data, n_items);
    }

    T operator[] (int i) {
        ASSERT(i < m_header.size, "Index " << i << " beyond array boundaries");
        int lock_num = i & max_lock_num;
        scoped_lock guard(m_header.lock[lock_num]);
        return m_data[i];
    }

    void operator[] (int i, const T& value) {
        ASSERT(i < m_header.size, "Index " << i << " beyond array boundaries");
        int lock_num = i & max_lock_num;
        scoped_lock guard(m_header.lock[lock_num]);
        m_data[i] = value;
    }

    /// This is a raw reference to internally stored item.
    /// Use only in the context protected by scoped lock!
    /// <code>
    ///     {
    ///         typedef synch::scoped_lock(concurrent_array<T>::lock_type lock_t;
    ///         boost::tuple<lock_t, data_t&> tuple = array.locked_get(10);
    ///         ... Do something with item without making blocking or system calls!
    ///         ... The lock will be released automatically at the scope exit.
    ///     }
    /// </code>
    boost::tuple<synch::lock_guard<Lock>, T&> 
    locked_get(int i) {
        int lock_num = i & max_lock_num;
        synch::lock_guard<Lock> guard(m_header.lock[lock_num]);
        return boost::tuple<synch::lock_guard<Lock>, T&>(guard, m_data[i]);
    }

    T       get(int i)              { return operator[] (i); }
    void    set(int i, const T& v)  { operator[] (i, v); }
    
    size_t  size() const            { return m_header.size; }
};

/** 
 * Implements a concurrent array with a lock per data item.
 * The structure is safe for multiple readers and writers
 *
 * Template arguments:
 *      T           - type of array's element
 *      N           - number of items in circular buffer
 */
template <class T, int N = 16>
class concurrent_atomic_array {
    typedef concurrent_atomic_array<T, N> self_t;
    enum { 
          IDLE, READING, WRITING, UNASSIGNED = 0xFFFFFFFFu
    };

    struct atomic_data {
        T             data;
        unsigned char status;
        atomic_data() : status(IDLE) {}
    };
    volatile size_t    m_index;
    atomic_data        m_data[N];  // must be last data member

    concurrent_atomic_array() : m_index(UNASSIGNED) {
        STATIC_ASSERT((N & (N-1)) == 0, N_must_be_power_of_2);
    }
public:
    typedef T value_type;

    static self_t& create(void* storage, size_t sz) {
        if (sizeof(self_t) <= sz)
            THROW_RUNTIME_ERROR(
                "Storage pool too small (expected " << sizeof(self_t) << ')');
        new (storage) concurrent_atomic_array();
    }

    void put(T& item) {
        size_t old_idx = m_index;
        size_t new_idx = (old_idx+1) & (N-1);
        while(1) {
            versioned_data& v = m_data[new_idx];
            if (!atomic::cas(&v.status, IDLE, WRITING))
                new_idx = (new_idx+1) & (N-1);
            else {
                v.data   = item;
                v.status = IDLE;
                break;
            }
        }
        // We are assuming only one writer is allowed
        m_index = new_idx;
        atomic::memory_barrier();
    }

    /// Get the latest version of the item.
    bool get(T& item) {
        if (m_index == UNASSIGNED)
            return false;
        while(1) {
            size_t old_idx = m_index;
            versioned_data& v = m_data[old_idx];
            if (atomic::cas(&v.status, IDLE, READING)) {
                item = v.data;
                v.status = IDLE;
                atomic::memory_barrier();
                return true;
            }
        }
    }

    size_t size() const { return N; }
};

} // namespace container
} // namespace utxx

#endif
