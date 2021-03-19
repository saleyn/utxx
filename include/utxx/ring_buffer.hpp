// vim:ts=4:et:sw=4
//------------------------------------------------------------------------------
/// \file   ring_buffer.hpp
/// \author Leonid Timochouk
/// \author Serge Aleynikov
//------------------------------------------------------------------------------
/// \brief Wait-free circular buffer for single reader multi-writer use cases
//------------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-08-20
//------------------------------------------------------------------------------
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
#pragma once

#include <utxx/meta.hpp>
#include <utxx/math.hpp>
#include <utxx/error.hpp>
#include <utxx/compiler_hints.hpp>
#include <cstddef>
#include <type_traits>
#include <atomic>
#include <cassert>
#include <stdlib.h>

namespace utxx {

//==============================================================================
// "ring_buffer" Class:
//==============================================================================
/// A ring-buffer suitable to use with heap or shared memory in non-concurrent
/// and single writer multi-reader applications.
///
/// This structure is WAIT-FREE; it is assumed that it has only 1 Writer
/// thread and any number of PASSIVE reader threads (which only
/// access items stored on the ring buffer but do not remove them)
//==============================================================================
template<
    typename T,
    size_t   StaticCapacity = 0,
    bool     Atomic         = true,
    bool     SizeIsPow2     = true
>
struct ring_buffer {
    /// Default constructor does not make much sense unless StaticCapacity > 0
    ring_buffer() : ring_buffer(0u) {}

    /// A factory function which allocates a new "ring_buffer" of a given
    /// "capacity" in a given memory, and optionally constructs it
    static ring_buffer* create
    (
        int    a_capacity,
        void*  a_memory    = NULL,
        size_t a_mem_sz    = 0,
        bool   a_construct = false
    ) {
        // Does it already exist?
        assert((a_memory && a_mem_sz >  0) ||
              (!a_memory && a_mem_sz == 0));
        // If memory is not given - object must be constructed
        assert(a_memory || a_construct);

        if (a_capacity <= 0)
            throw badarg_error
                ("ring_buffer::create: invalid capacity: ", a_capacity);

        // Calculate the full object size:
        size_t expect_sz = memory_size(a_capacity);

        if (a_mem_sz && a_mem_sz != expect_sz)
            throw badarg_error
                ("ring_buffer::create: invalid memory size (expected=",
                 expect_sz, ", got=", a_mem_sz);

        bool allocated_externally = !!a_memory;

        if (!allocated_externally) {
            a_memory    = ::malloc(expect_sz);
            a_construct = true;
        }

        ring_buffer<T, StaticCapacity, Atomic, SizeIsPow2>* p;

        // Optionally run the "placement new" constructor on the given memory:
        if (a_construct)
            p = new (a_memory) ring_buffer<T, StaticCapacity, Atomic, SizeIsPow2>
                               (a_capacity, allocated_externally);
        else {
            p = reinterpret_cast
                    <ring_buffer<T, StaticCapacity, Atomic, SizeIsPow2>*>(a_memory);
            if ((p->m_version & ~0x1) != s_version)
                throw runtime_error
                    ("ring_buffer::create: invalid version of existing "
                     " ring buffer at given memory address ", a_memory);
        }

        assert(p);
        assert(!allocated_externally || p->is_externally_allocated());

        return p;
    }

    /// Destroy previously allocated buffer
    static void destroy_ptr(ring_buffer* a_ptr) { destroy(a_ptr); }
    static void destroy(ring_buffer*&    a_ptr) {
        if (!a_ptr)
            return;

        if (!a_ptr->is_externally_allocated()) {
            a_ptr->~ring_buffer();
            free(a_ptr);
        }
        a_ptr = nullptr;
    }

    /// Check if the content is empty
    bool   empty()      const { return !load_size<Atomic>(); }
    /// Check if the entire ring buffer is filled
    bool   full()       const { return load_size<Atomic>() >= m_capacity; }
    /// Max number of elements that can be stored in the ring-buffer
    size_t capacity()   const { return m_capacity;   }
    /// Number of allocated slots in the ring-buffer
    size_t size()       const {
        size_t n = load_size<Atomic>();
        return n < m_capacity ? n : m_capacity;
    }

    /// Returns true if the instance was constructed from externally
    /// allocated memory
    bool is_externally_allocated() const {
        return m_version & 0x1; // Last bit set - indicates external allocation
    }

    /// "Clear":
    void clear() { store_size<Atomic>(0); }

    /// \brief Insert a new entry into the buffer
    /// \return a direct access ptr to the entry stored
    template<class ...Args>
    const T* add(Args&&... a_ctor_args) {
        // Check if the item associated with the new entry is already there:
        // The curr front of the Buffer (where next insertion would occur):
        // NB: Since capacity is a power of 2, modular arithmetic can be done as
        // a logical AND:
        //
        size_t sz = load_size<Atomic>(std::memory_order_acquire);
        size_t pos;

        if constexpr(SizeIsPow2)
            pos = sz & m_mask;
        else
            pos = sz - ((sz / m_capacity) * m_capacity);

        // Store the value at "front" and atomically increment "m_size", so the
        // Reader Clients will always have a consistent view;  "m_size"  itself
        // is monotonically-increasing (NOT modular).
        // The new entry is constructed in-place (a partial case is applying the
        // Copy Ctor on "T"):
        //
        T* at = m_entries + pos;
        construct<T>(at, a_ctor_args...);

        store_size<Atomic>(sz + 1);

        return at;
    }

    /// Index of the most recent entry in range [0 ... capacity()-1]
    size_t last() const {
        size_t sz = load_size<Atomic>();
        if (UNLIKELY(sz == 0))
            // CANNOT produce a valid ptr because there are no, as yet,
            // entries to point to:
            throw std::runtime_error("ring_buffer:last(): no entries");

        // GENERIC CASE: The most-recently-inserted pos. Again, using the
        // logical AND for power-of-2 modular arithmetic:
        if constexpr(SizeIsPow2)
            return (sz - 1) & m_mask;
        else {
            auto   n = sz-1;
            return n - ((n / m_capacity) * m_capacity);
        }
    }

    /// Ptr to the most recent entry:
    const T* back()        const { return m_entries + last();  }

    /// Total number of entries added so far (including over-written ones)
    size_t   total_count() const { return load_size<Atomic>(); }

    /// Accessing the ring buffer by index
    T const& operator[](size_t a_idx) const {
        check_index(a_idx);
        return m_entries[a_idx];
    }

    /// Accessing the ring buffer by index
    T& operator[](size_t a_idx) {
        check_index(a_idx);
        return m_entries[a_idx];
    }

    /// Total memory footprint needed to allocate a ring-buffer of a_capacity
    static size_t memory_size(size_t a_capacity) {
        size_t sz = SizeIsPow2 ? math::upper_power(a_capacity, 2) : a_capacity;
        return sizeof(ring_buffer<T>) + sizeof(T) * sz;
    }

private:
    //--------------------------------------------------------------------------
    // Internals:
    //--------------------------------------------------------------------------
    static const size_t s_static_capacity =
        upper_power<StaticCapacity, 2>::value;

    // Version of the ring_buffer used to validate it in case we will construct
    // externally allocated buffer (last bit signifies external allocation
    static const size_t s_version = 0xFF123450;
    size_t m_version;

    // Total number of Entries inserted (including those which over-wrote the
    // previous ones). It is "atomic" if Atomic param is set:
    //
    typename
        std::conditional<Atomic, std::atomic<size_t>, size_t>::type
        m_end;

    // Array holding the data, of "m_capacity" total length. XXX: If the Stat-
    // icCapacity is 0, we use a a C-style variable-size class / struct here
    // (ie the header and body are aloocated at once, for efficiency reasons).
    // Otherwise, the Capacity is rounded up to the nearest power of 2;
    // (NB: the result is 0 if Capacity is 0, ie the corresp exponent of 2 is
    // -oo):
    size_t m_capacity;
    size_t m_mask;
    T*     m_data;
    T      m_entries[s_static_capacity];

    // NB: Copy Ctor, Assignemnt and Equality Testing are not deleted but gen-
    // erally discouraged...

    //--------------------------------------------------------------------------
    // Non-Default Ctor:
    //--------------------------------------------------------------------------
    // NB:
    // (*) it does not allocate the space for "m_entries" -- the Caller is res-
    //     ponsible for allocating the whole "ring_buffer" object of the cor-
    //     resp size;
    // (*) it is applicable only if the StaticCapacity is 0;
    // (*) the actualy capacity is rounded UP to a power of 2 from the value
    //     provided (based on the memory available);
    // The Ctor is private (but not deleted!) because it is supposed to  be in-
    // voked from within "CreateInShM" only:
    //
    ring_buffer(size_t a_capacity, bool a_external_memory)
        : m_version   (s_version | (a_external_memory ? 1 : 0))
        , m_end       (0u)
        , m_capacity  (StaticCapacity
            ? s_static_capacity : math::upper_power(a_capacity, 2))
        , m_mask      (m_capacity - 1)
    {
        if (!(StaticCapacity ^ a_capacity))
            throw badarg_error
                ("ring_buffer: invalid / inconsistent capacity (static=",
                 StaticCapacity, ", dynamic=", a_capacity);

        assert(m_capacity >= 1);
        if constexpr(SizeIsPow2)
            assert((m_capacity & m_mask) == 0);   // Power of 2 indeed!
    }

    /// User needs to use destroy() to destruct the object
    ~ring_buffer() { clear(); }

    template <bool IsAtomic>
    typename std::enable_if<IsAtomic, size_t>::type
    load_size(std::memory_order a_ord = std::memory_order_relaxed) const
        { return m_end.load(a_ord); }

    template <bool IsAtomic>
    typename std::enable_if<!IsAtomic, size_t>::type
    load_size(std::memory_order a_ord = std::memory_order_relaxed) const
        { return m_end; }

    template <bool IsAtomic>
    typename std::enable_if<IsAtomic, void>::type
    store_size(size_t a) { m_end.store(a, std::memory_order_release); }

    template <bool IsAtomic>
    typename std::enable_if<!IsAtomic, void>::type
    store_size(size_t a) { m_end = a; }

    void check_index(size_t a_idx) {
        #ifndef NDEBUG
        // If the buffer is full, then any "idx" (< capacity) is OK; otherwise,
        // it must be < size:
        auto sz    = load_size<Atomic>();
        auto limit = std::min<size_t>(sz, m_capacity);
        if (UNLIKELY(a_idx >= limit))
            throw badarg_error
                ("ring_buffer[]", "invalid idx=", a_idx, ", capacity=",
                 m_capacity, ", total_size=", sz);
        assert(a_idx < m_capacity);
        #endif
    }

    template<class E>
    typename std::enable_if<std::is_trivial<E>::value, void>::type
    construct(E* a_ele, E a_item) {
        *a_ele = a_item;
    }

    template<class E, class ...Args>
    typename std::enable_if<!std::is_trivial<E>::value, void>::type
    construct(E* a_ele, Args&&... a_ctor_args) {
        new (a_ele) E(a_ctor_args...);
    }
};

} // namespace utxx
