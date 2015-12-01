// vim:ts=4:sw=4:et
//------------------------------------------------------------------------------
/// @file  generation_buffer.hpp
/// @brief Wait-free single-producer multi-consumer insertion-only ring buffer
//------------------------------------------------------------------------------
// Copyright (C) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-05
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2015 Serge Aleynikov <saleyn@gmail.com>

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

#include <cstddef>
#include <utxx/meta.hpp>
#include <utxx/math.hpp>
#include <utxx/error.hpp>
#include <utxx/compiler_hints.hpp>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <atomic>
#include <algorithm>
#include <type_traits>
#include <utility>
#include <cassert>

#ifdef UTXX_GENERATION_BUFFER_SHMEM
#include <boost/interprocess/managed_shared_memory.hpp>
namespace BIPC = boost::interprocess;
#endif

namespace utxx
{
    namespace {
        template <class Self, bool Atomic> struct size_impl {
            using type = std::atomic<uint64_t>;

            static uint64_t get(const Self* p) {
                return const_cast<Self*>(p)->m_size.load
                                             (std::memory_order_relaxed);
            }
            static void inc(Self* p) {
                p->m_size.fetch_add(1, std::memory_order_relaxed);
            }
            static void store(Self* p, size_t n) {
                p->m_size.store(n, std::memory_order_relaxed);
            }
        };

        template <class Self> struct size_impl<Self, false> {
            using  type =   std::size_t;
            static uint64_t get(const Self* p)     { return p->m_size; }
            static void     inc(Self* p)           { ++p->m_size; }
            static void   store(Self* p, size_t n) { p->m_size = n; }
        };
    }

    //--------------------------------------------------------------------------
    /// Generational wait-free single-producer multiple-consumer buffer.
    /// @tparam T              generational item type.
    /// @tparam StaticCapacity optional static capacity of the buffer (not
    ///                        needed when the buffer is allocated in shared
    ///                        memory or using dynamic heap allocation).
    /// @tparam AtomicSize     indicates whether or not the buffer is used by
    ///                        multiple threads.
    //--------------------------------------------------------------------------
    template<typename T, uint32_t StaticCapacity=0, bool AtomicSize=true>
    class generation_buffer {
        template <class S, bool A> friend struct size_impl;
    private:
        using Self  = generation_buffer<T, StaticCapacity, AtomicSize>;
        using Size  = size_impl<Self, AtomicSize>;
        using SizeT = typename Size::type;

        /// Total number of entries inserted since last clear (with overwrites)
        SizeT    m_size;
        /// Max capacity of the buffer in number of slots rounded up to pow of 2
        uint32_t m_capacity;
        uint32_t m_mask;     ///< Capacity minus one.

        /// Array holding the data, of "m_capacity" total length.
        // If the StaticCapacity is 0, we use a a C-style variable-size
        // class / struct here (i.e. the header and body are allocated at once,
        // for efficiency reasons).
        // Otherwise, the Capacity is rounded up to the nearest power of 2;
        T        m_entries[upper_power<StaticCapacity, 2>::value];

        // NB: Copy Ctor, Assignemnt and Equality Testing are not deleted but
        // generally discouraged...

        //----------------------------------------------------------------------
        // Non-Default Ctor:
        //----------------------------------------------------------------------
        // (*) it does not allocate the space for "m_entries" -- the caller is
        //     responsible for allocating the whole "generation_buffer" object
        //     of the corresp size;
        // (*) it is applicable only if the StaticCapacity is 0;
        // (*) the actualy capacity is rounded DOWN to a power of 2 from the
        //     value provided (based on the memory available);
        // The ctor is private (but not deleted!) because it is supposed to be
        // invoked from within "CreateInShM" only:
        //
        generation_buffer(uint32_t a_capacity)
            : m_size    (0)
            , m_capacity(0)
            , m_mask    (0)
        {
            if (StaticCapacity != 0 || a_capacity == 0)
                throw std::invalid_argument
                      ("generation_buffer: InValid / InConsistent Capacity");

            uint32_t cap2 = utxx::math::upper_power(a_capacity, 2);
            assert(a_capacity >= 1 && cap2 >= 1);

            if (cap2 > a_capacity)
                cap2 /= 2;
            assert(1 <= cap2 && cap2 <= a_capacity);

            m_capacity = cap2;
            m_mask     = cap2 - 1;
            assert((m_capacity & m_mask) == 0);   // Power of 2!
        }

    public:
        //----------------------------------------------------------------------
        /// Default ctor only makes sense when StaticCapacity > 0.
        //----------------------------------------------------------------------
        /// The buffer is allocated on the heap using placement new with the
        /// memory allocated by the caller with help of memory_size() function.
        /// Alternatively it can be placed in shared memory using create().
        generation_buffer()
            : m_size      (0)
            , m_capacity  (sizeof(m_entries) / sizeof(T)) // Capacity in bytes
            , m_mask      (m_capacity - 1)
        {
            assert((m_capacity & m_mask) == 0); // Capacity must be pow of 2
        }

        ~generation_buffer() = default;

        //----------------------------------------------------------------------
        // "IsEmpty":
        //----------------------------------------------------------------------
        bool empty() const { return Size::get(this) == 0; }

        //----------------------------------------------------------------------
        // Clear the data in the buffer (but not the buffer itself)
        //----------------------------------------------------------------------
        void clear() { m_size = 0; }

        //----------------------------------------------------------------------
        /// Emplace a new entry into the buffer
        /// @return a direct access ref to the entry stored
        //----------------------------------------------------------------------
        template<class...Args>
        T const& emplace(Args&&... a_ctor_args) {
            uint32_t front = Size::get(this) & m_mask;
            assert(front < m_capacity);

            T*   at  = m_entries + front;
            new (at) T(a_ctor_args...);
            Size::inc(this);
            return *at;
        }

        //----------------------------------------------------------------------
        /// Insert a copy of an entry into the buffer
        /// @return a direct access ref to the entry stored
        //----------------------------------------------------------------------
        const T& move(T&& a_item) {
            uint32_t front = Size::get(this) & m_mask;
            assert(front < m_capacity);

            T*   at  = m_entries + front;
            new (at) T(std::move(a_item));
            Size::inc(this);
            return *at;
        }

        //----------------------------------------------------------------------
        /// Add a copy of an entry into the buffer
        /// @return a direct access ref to the entry stored
        //----------------------------------------------------------------------
        const T& add(const T& a_item) {
            uint32_t front = Size::get(this) & m_mask;
            assert(front < m_capacity);

            T*   at  = m_entries + front;
            new (at) T(a_item);
            Size::inc(this);
            return *at;
        }

        //----------------------------------------------------------------------
        /// Call \a a_fun and insert the resulting item at the next index.
        /// Atomically evaluate the lambda \a a_fun by passing it a reference
        /// of the next available item in the buffer and incrementing generation
        /// index on success.
        /// This method is EXCLUSIVELY to be used by the producer of data.
        /// @return -1 if \a a_fun evaluates to false, or index of the newly
        ///         inserted item.
        //----------------------------------------------------------------------
        template <typename Lambda>
        int add(const Lambda& a_fun) {
            uint32_t front = Size::get(this) & m_mask;
            assert(front < m_capacity);

            T*  at = m_entries + front;
            if (unlikely(!a_fun(*at, front)))
                return -1;
            Size::inc(this);
            return front;
        }

        //----------------------------------------------------------------------
        /// Get reference of the most recent entry
        //----------------------------------------------------------------------
        const T& last() const {
            auto sz = Size::get(this);
            if (sz == 0)
                throw std::runtime_error("generation_buffer:last(): empty!");

            /// Return the most-recently-inserted item:
            auto last = (sz - 1) & m_mask;
            assert(last < m_capacity);
            return m_entries[last];
        }

        //----------------------------------------------------------------------
        /// Get reference of the most recent entry (maybe NULL)
        //----------------------------------------------------------------------
        const T* last_ptr() const {
            uint32_t sz = Size::get(AtomicSize);
            if (unlikely(!sz))
                return nullptr;

            // Return the most-recently-inserted item:
            auto last = (sz - 1) & m_mask;
            assert(last < m_capacity);
            return m_entries + last;
        }

        //----------------------------------------------------------------------
        /// Reserve the pointer to the next available generation.
        /// This method is EXCLUSIVELY to be used by the producer of data.
        /// @return a pair of the next-past-last element's pointer and its index
        //----------------------------------------------------------------------
        std::pair<T*, uint32_t> reserve() {
            /// Return the next available item slot:
            auto i = Size::get(this) & m_mask;
            assert(i < m_capacity);
            return std::make_pair(m_entries+i, i);
        }

        //----------------------------------------------------------------------
        /// Write the index value obtained by the call to next() after the
        /// modification of the returned data element is complete.
        /// This method is EXCLUSIVELY to be used by the producer of data in
        /// conjunction with the call to reserve().
        //----------------------------------------------------------------------
        void commit_index(uint32_t a_idx) { Size::store(this, a_idx+1); }

        //----------------------------------------------------------------------
        /// Index of the most recent entry.
        //----------------------------------------------------------------------
        uint32_t index() const
        {
            uint32_t sz =  m_size;
            if (sz == 0)
                // No entries - cannot produce a valid ptr yet
                throw std::runtime_error("generation_buffer::index: No Entries");

            uint32_t last = (sz - 1) & m_mask;
            assert(last < m_capacity);
            return last;
        }

        //----------------------------------------------------------------------
        /// Access the buffer by index
        //----------------------------------------------------------------------
        const T& operator[](uint32_t a_idx) const {
#ifndef NDEBUG
            // If the buffer is full, then any "idx" (< capacity) is OK;
            // otherwise it must be < size:
            uint32_t limit = std::min<uint32_t>(Size::get(this), m_capacity);
            if (utxx::unlikely(a_idx >= limit))
                throw badarg_error(
                    "generation_buffer: Invalid idx=", a_idx,
                    ", capacity=",    m_capacity, ", total_size=", Size::get(this));
            assert(a_idx < m_capacity);
#endif
            return m_entries[a_idx];
        }

        //----------------------------------------------------------------------
        /// Non-const buffer accessor by index
        //----------------------------------------------------------------------
        T& operator[](uint32_t a_idx) {
#ifndef NDEBUG
            uint32_t limit = std::min<uint32_t>
                (Size::get(this), m_capacity);
            if (utxx::unlikely(a_idx >= limit))
                throw badarg_error
                      ("generation_buffer: Invalid Idx=", a_idx, ": Capacity=",
                       m_capacity, ", TotSize=", Size::get(this));
            assert(a_idx < m_capacity);
#endif
            return m_entries[a_idx];
        }

        //----------------------------------------------------------------------
        /// Total number of entries saved in the buffer since it was cleared
        //----------------------------------------------------------------------
        uint32_t total_count() const { return Size::get(this); }

        //----------------------------------------------------------------------
        /// Total memory footprint of this buffer
        //----------------------------------------------------------------------
        uint32_t memory_size() const { return m_capacity; }

        //----------------------------------------------------------------------
        /// Get the memory size needed to allocate a buffer of given capacity
        //----------------------------------------------------------------------
        static size_t memory_size(size_t a_capacity) {
            return sizeof(Self) + a_capacity * sizeof(T);
        }

        //----------------------------------------------------------------------
        /// Factory function for allocating the buffer in the heap memory
        /// @param a_buffer externally allocated memory buffer.
        /// @param a_size   buffer sized obtained by the call to memory_size().
        //----------------------------------------------------------------------
        static generation_buffer<T, StaticCapacity, AtomicSize>*
        create(void* a_buffer, size_t a_size) {
            // Verify the full object size:
            auto capacity = (a_size - sizeof(Self)) / sizeof(T);

            if (memory_size(capacity) != a_size)
                throw badarg_error(
                    "generation_buffer::create: use memory_size() to determine "
                    "the size of required memory buffer!");

            return new (a_buffer) Self(capacity);
        }

        //----------------------------------------------------------------------
        /// Factory function for allocating the buffer using given allocator
        /// @param a_capacity is the number of elements in the buffer
        /// @param a_alloc    is the allocator to use
        //----------------------------------------------------------------------
        template <typename Alloc = std::allocator<char>>
        static generation_buffer<T, StaticCapacity, AtomicSize>*
        create(size_t a_capacity, const Alloc& a_alloc = Alloc()) {
            // Verify the full object size:
            auto sz = memory_size(a_capacity);

            auto p = Alloc(a_alloc).allocate(sz);

            return new ((void*)p) Self(a_capacity);
        }

        //----------------------------------------------------------------------
        /// Iterate backwards and apply \a Visitor
        ///
        /// The lambda \a Visitor returns "true" to continue, "false" to stop.
        /// <tt>bool Visitor(const E&);</tt>
        //----------------------------------------------------------------------
        template<typename Visitor>
        void reverse_visit(Visitor&& a_visitor) const
        {
            auto sz = Size::get(this);
            if  (sz == 0)
                return;

            /// Return the most-recently-inserted item:

            // Generic Case:
            // First, run down to 0:
            auto last = (sz - 1) & m_mask;
            for (long i = last; i >= 0; --i) {
                assert(0 <= i && i < long(m_capacity));
                auto curr = m_entries + i;
                // Apply the "action" and check exit status:
                if (unlikely(!(a_visitor(*curr))))
                    return;
            }

            // Then possibly wrap around the buffer end:
            if (unlikely(last <= m_capacity))
                return;

            for (long i = long(m_mask); i > last; --i) {
                assert(0 <= i && i < long(m_capacity));
                auto curr = m_entries + i;
                // Apply the "action" and check exit status:
                if (unlikely(!(a_visitor(*curr))))
                    return;
            }
        }

#ifdef UTXX_GENERATION_BUFFER_SHMEM
        //----------------------------------------------------------------------
        /// Factory function for allocating the buffer in shared memory
        //----------------------------------------------------------------------
        /// Allocate a new "generation_buffer"  of a given capacity in a given
        /// shared memory segment, and runs a Ctor to initialise it.
        /// Because the object has a dynamically-determined size, it is
        /// allocated as an array of "long double"s (this guarantees proper
        /// alignment), and that array ptr is converted into void* using
        /// "placement new".
        static generation_buffer<T, StaticCapacity, AtomicSize>*
        create
        (
            int                                a_capacity,
            BIPC::fixed_managed_shared_memory* a_segment,
            char const*                        a_obj_name,
            bool                               a_force_new
        )
        {
            // Does it already exist?
            assert(a_obj_name != NULL && *a_obj_name != '\0');
            std::pair<long double*, size_t> fres =
                a_segment->find<long double>(a_obj_name);
            bool exists = (fres.first != NULL);

            if (exists && !a_force_new) {
                assert(fres.second != 0);
                return (Self*)(fres.first);
            }

            // Otherwise: Create a new CircBuff:
            assert(a_segment != NULL);
            if (a_capacity <= 0)
                throw badarg_error
                      ("generation_buffer::create: Invalid Capacity)");

            // Calculate the full object size:
            auto sz = memory_size(a_capacity);

            try {
                // Remove the existing one first:
                if (exists)
                    (void) a_segment->destroy<long double>(a_obj_name);

                // Allocate it as a named array of bytes, zeroing them out. XXX: how-
                // ever, we need a right alignment here -- so do 16-byte alignment:
                // XXX:  or do we need an explicit alignment? Maybe it's 16 bytes by
                // default on 64-bit systems?
                //
                static_assert(sizeof(long double) == 16, "sizeof(long double)!=16 ?");
                int sz16 = (sz % 16 == 0) ? (sz / 16) : (sz / 16 + 1);

                long double* mem =
                    a_segment->construct<long double>(a_obj_name)[sz16](0.0L);
                assert(mem != NULL);

                // Run the "placement new" and the CircBuff Ctor on the bytes allo-
                // cated:
                auto* res = new ((void*)(mem)) Self(a_capacity);
                assert(res != NULL);
                return res;
            } catch (std::exception const& e) {
                // Delete the CircBuff if it has already been constructed:
                (void) a_segment->destroy<long double>(a_obj_name);

                throw runtime_error("generation_buffer: creation of ", a_obj_name,
                                    " of size ", sz, " failed: ", e.what());
            }
        }

        //----------------------------------------------------------------------
        /// Deallocate a named generation_buffer object from shared memory:
        //----------------------------------------------------------------------
        static void destroy
        (
            generation_buffer*                 a_obj,
            BIPC::fixed_managed_shared_memory* a_segment
        )
        {
            if (!a_obj)
                return;

            // Run a Dtor, although it is trivial in this case:
            a_obj->~Self();

            // De-allocate the CircBuff by ptr. It was allocated as an array of
            // "long double"s with 16-byte alignment. Do same for de-allocation:
            assert(a_segment);
            a_segment->destroy_ptr((long double*)(a_obj));
        }

        //----------------------------------------------------------------------
        /// Find named generation_buffer in a shared memory segment
        /// @return a non-NULL ptr if the buffer is found, or else throws exc.
        //----------------------------------------------------------------------
        static generation_buffer<E, StaticCapacity, WithSync> const&
        locate(BIPC::fixed_managed_shared_memory* a_seg, char const* a_obj_name)
        {
            assert(obj_name != NULL && *obj_name != '\0');
            std::pair<char*, size_t> fres = a_seg->find<char>(a_obj_name);

            auto res = (generation_buffer<E, StaticCapacity, WithSync> const*)
                       fres.first;

            if (unlikely(!res))
                throw runtime_error
                    ("Generation buffer ", a_obj_name, " not found!");
            return *res;
        }
#endif // UTXX_GENERATION_BUFFER_SHMEM
    };

} // namespace utxx
