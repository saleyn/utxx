//----------------------------------------------------------------------------
/// \file   atomic_hash_array.hpp
/// \author Spencer Ahrens
//----------------------------------------------------------------------------
/// \brief A high performance concurrent hash map with int32 or int64 keys.
///
/// This file is taken from open-source folly library with some enhancements
/// done by Serge Aleynikov to support hosting hash map/array in shared memory.
///
/// Supports insert, find(key), find_at(index), erase(key), size, and more.
/// Memory cannot be freed or reclaimed by erase.
/// Can grow to a maximum of about 18 times the
/// initial capacity, but performance degrades linearly with growth. Can also be
/// used as an object store with unique 32-bit references directly into the
/// internal storage (retrieved with iterator::index()).
///
/// Advantages:
///    - High performance (~2-4x tbb::concurrent_hash_map in heavily
///      multi-threaded environments).
///    - Efficient memory usage if initial capacity is not over estimated
///      (especially for small keys and values).
///    - Good fragmentation properties (only allocates in large slabs which can
///      be reused with clear() and never move).
///    - Can generate unique, long-lived 32-bit references for efficient lookup
///      (see find_at()).
///
/// Disadvantages:
///    - Keys must be native int32 or int64, or explicitly converted.
///    - Must be able to specify unique empty, locked, and erased keys
///    - Performance degrades linearly as size grows beyond initialization
///      capacity.
///    - Max size limit of ~18x initial size (dependent on max load factor).
///    - Memory is not freed or reclaimed by erase.
///
/// Usage and Operation Details:
///   Simple performance/memory tradeoff with max_load_factor.  Higher load factors
///   give better memory utilization but probe lengths increase, reducing
///   performance.
///
/// Implementation and Performance Details:
///   AHArray is a fixed size contiguous block of value_type cells.  When
///   writing a cell, the key is locked while the rest of the record is
///   written.  Once done, the cell is unlocked by setting the key.  find()
///   is completely wait-free and doesn't require any non-relaxed atomic
///   operations.  AHA cannot grow beyond initialization capacity, but is
///   faster because of reduced data indirection.
///
///   AHMap is a wrapper around AHArray sub-maps that allows growth and provides
///   an interface closer to the stl UnorderedAssociativeContainer concept. These
///   sub-maps are allocated on the fly and are processed in series, so the more
///   there are (from growing past initial capacity), the worse the performance.
///
///   Insert returns false if there is a key collision and throws if the max size
///   of the map is exceeded.
///
///   Benchmark performance with 8 simultaneous threads processing 1 million
///   unique <int64, int64> entries on a 4-core, 2.5 GHz machine:
///
///     Load Factor   Mem Efficiency   usec/Insert   usec/Find
///         50%             50%           0.19         0.05
///         85%             85%           0.20         0.06
///         90%             90%           0.23         0.08
///         95%             95%           0.27         0.10
///
///   See folly/tests/atomic_hash_mapTest.cpp for more benchmarks.
///
/// @author Spencer Ahrens <sahrens@fb.com>
/// @author Jordan DeLong <delong.j@fb.com>
///
/// Contributor: Serge Aleynikov <saleyn@gmail.com>
///   (added support for offset pointers and shared memory allocation)
///
//----------------------------------------------------------------------------
/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#define _UTXX_ATOMICHASHMAP_HPP_

#include <boost/iterator/iterator_facade.hpp>
#include <boost/noncopyable.hpp>
#include <boost/type_traits/is_convertible.hpp>

#include <stdexcept>
#include <functional>
#include <atomic>

#include <utxx/atomic_hash_array.hpp>

namespace utxx {

/*
 * atomic_hash_map provides an interface somewhat similar to the
 * UnorderedAssociativeContainer concept in C++.  This does not
 * exactly match this concept (or even the basic Container concept),
 * because of some restrictions imposed by our datastructure.
 *
 * Specific differences (there are quite a few):
 *
 * - Efficiently thread safe for inserts (main point of this stuff),
 *   wait-free for lookups.
 *
 * - You can erase from this container, but the cell containing the key will
 *   not be free or reclaimed.
 *
 * - You can erase everything by calling clear() (and you must guarantee only
 *   one thread can be using the container to do that).
 *
 * - We aren't DefaultConstructible, CopyConstructible, Assignable, or
 *   EqualityComparable.  (Most of these are probably not something
 *   you actually want to do with this anyway.)
 *
 * - We don't support the various bucket functions, rehash(),
 *   reserve(), or equal_range().  Also no constructors taking
 *   iterators, although this could change.
 *
 * - Several insertion functions, notably operator[], are not
 *   implemented.  It is a little too easy to misuse these functions
 *   with this container, where part of the point is that when an
 *   insertion happens for a new key, it will atomically have the
 *   desired value.
 *
 * - The map has no templated insert() taking an iterator range, but
 *   we do provide an insert(key, value).  The latter seems more
 *   frequently useful for this container (to avoid sprinkling
 *   make_pair everywhere), and providing both can lead to some gross
 *   template error messages.
 *
 * - The Allocator must not be stateful (a new instance will be spun up for
 *   each allocation), and its allocate() method must take a raw number of
 *   bytes.
 *
 * - KeyT must be a 32 bit or 64 bit atomic integer type, and you must
 *   define special 'locked' and 'empty' key values in the ctor
 *
 * - We don't take the Hash function object as an instance in the
 *   constructor.
 *
 */

// Thrown when insertion fails due to running out of space for
// submaps.
struct atomic_hash_map_full_error : std::runtime_error {
    explicit atomic_hash_map_full_error()
        : std::runtime_error("atomic_hash_map is full")
    {}
};

template<class KeyT, class ValueT,
         class HashFcn   = std::hash<KeyT>,
         class EqualFcn  = std::equal_to<KeyT>,
         class Allocator = std::allocator<char>,
         class SubMap    = atomic_hash_array<KeyT,ValueT,HashFcn,EqualFcn>,
         class PSubMap   = SubMap*>
class atomic_hash_map : boost::noncopyable {
public:
    using char_alloc      = typename Allocator::template rebind<char>::other;
    using key_type        = KeyT;
    using mapped_type     = ValueT;
    using value_type      = std::pair<const KeyT, ValueT>;
    using hasher          = HashFcn;
    using key_equal       = EqualFcn;
    using pointer         = value_type*;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using difference_type = std::ptrdiff_t;
    using size_type       = std::size_t;
    using config          = typename SubMap::config;

    template<class ContT, class IterVal, class SubIt>
    struct ahm_iterator;

    using const_iterator =
        ahm_iterator<const atomic_hash_map,
                     const value_type,
                     typename SubMap::const_iterator>;
    using iterator =
        ahm_iterator<atomic_hash_map,
                     value_type,
                     typename SubMap::iterator>;

public:
    const float  m_growth_frac;  // How much to grow when we run out of capacity

    static const config default_config;

    // The constructor takes a max_sz_hint which is the optimal
    // number of elements to maximize space utilization and performance,
    // and a config object to specify more advanced options.
    explicit atomic_hash_map(size_t max_sz_hint, const config& = default_config,
                             const char_alloc& alloc = char_alloc());

    ~atomic_hash_map() {
        const int num_maps = m_alloc_num_maps.load(std::memory_order_relaxed);
        for (int i=0; i < num_maps; i++) {
            PSubMap map = m_submaps[i].load(std::memory_order_relaxed);
            assert(map);
            SubMap::destroy(&*map, m_allocator);
        }
    }

    const key_equal& key_eq()        const { return m_config.m_eq_fun;   }
    const hasher&    hash_function() const { return m_config.m_hash_fun; }

    // TODO: emplace() support would be nice.

    /// Insert a value pair into the map
    ///
    /// Returns a pair with iterator to the element at r.first and
    /// success.  Retrieve the index with ret.first.index().
    ///
    /// Does not overwrite on key collision, but returns an iterator to
    /// the existing element (since this could due to a race with
    /// another thread, it is often important to check this return
    /// value).
    ///
    /// Allocates new sub maps as the existing ones become full.  If
    /// all sub maps are full, no element is inserted, and
    /// atomic_hash_map_full_error is thrown.
    std::pair<iterator,bool> insert(const value_type& r) {
        return insert(r.first, r.second);
    }
    std::pair<iterator,bool> insert(const key_type& k, const mapped_type& v);
    std::pair<iterator,bool> insert(value_type&& r) {
        return insert(r.first, std::move(r.second));
    }
    std::pair<iterator,bool> insert(const key_type& k, mapped_type&& v);

    /// Find a value associated with the key in the map
    ///
    /// @return an iterator into the map or end() if key is not found
    iterator       find(const key_type& k);
    const_iterator find(const key_type& k) const;

    /// Erase a value associated with key from the map
    ///
    /// @return 1 if the key is found and erased, and 0 otherwise.
    size_type erase(const key_type& k);

    /// Clear the map
    ///
    /// Wipes all keys and values from primary map and destroys all secondary
    /// maps.  Primary map remains allocated and thus the memory can be reused
    /// in place.  Not thread safe.
    void clear();

    /// Returns the exact size of the map
    ///
    /// Note this is not as cheap as typical size() implementations because
    /// for each atomic_hash_array in this AHM, we need to grab a lock and
    /// accumulate the values from all the thread local
    /// counters.  See utxx/thread_cached_int.hpp for more details.
    size_t size() const;

    bool empty() const { return size() == 0; }

    /// Check if there's a value associated with the key
    bool exists(const key_type& k) const { return find(k) != end(); }

    /// Returns an iterator into the map associated with given index
    ///
    /// Note: \a idx should only be an unmodified value returned by calling
    /// index() on a valid iterator returned by find() or insert(). If idx is
    /// invalid you have a bug and the process aborts.
    iterator find_at(uint32_t idx) {
        simple_ret_t ret = internal_find_at(idx);
        assert(int(ret.i) < num_submaps());
        return iterator(this, ret.i,
        m_submaps[ret.i].load(std::memory_order_relaxed)->make_iter(ret.j));
    }
    const_iterator find_at(uint32_t idx) const {
        return const_cast<atomic_hash_map*>(this)->find_at(idx);
    }

    /// Total capacity - summation of capacities of all submaps
    size_t capacity() const;

    /// Number of new insertions until current submaps are all at max load factor
    size_t remaining_space() const;

    void entry_count_thr_cache_size(int32_t newSize) {
        const int numMaps = m_alloc_num_maps.load(std::memory_order_acquire);
        for (int i = 0; i < numMaps; ++i) {
        PSubMap map = m_submaps[i].load(std::memory_order_relaxed);
        map->entry_count_thr_cache_size(newSize);
        }
    }

    /// Number of sub maps allocated so far to implement this map
    ///
    /// The more there are, the worse the performance.
    int num_submaps() const {
        return m_alloc_num_maps.load(std::memory_order_acquire);
    }

    iterator begin() {
        return iterator(this, 0,
        m_submaps[0].load(std::memory_order_relaxed)->begin());
    }

    iterator end() { return iterator(); }

    const_iterator begin() const {
        return const_iterator(this, 0,
        m_submaps[0].load(std::memory_order_relaxed)->begin());
    }

    const_iterator end()   const { return const_iterator(); }

    /* Advanced functions for direct access: */

    inline uint32_t rec_to_idx(const value_type& r, bool mayInsert = true) {
        simple_ret_t ret = mayInsert ?
        internal_insert(r.first, r.second) : internal_find(r.first);
        return encode_idx(ret.i, ret.j);
    }

    inline uint32_t rec_to_idx(value_type&& r, bool mayInsert = true) {
        simple_ret_t ret = mayInsert ?
        internal_insert(r.first, std::move(r.second)) : internal_find(r.first);
        return encode_idx(ret.i, ret.j);
    }

    inline uint32_t rec_to_idx(const key_type& k, const mapped_type& v,
        bool mayInsert = true) {
        simple_ret_t ret = mayInsert ? internal_insert(k, v) : internal_find(k);
        return encode_idx(ret.i, ret.j);
    }

    inline uint32_t rec_to_idx(const key_type& k, mapped_type&& v, bool may_ins = true) {
        simple_ret_t ret = may_ins ? internal_insert(k, std::move(v))
                                   : internal_find(k);
        return encode_idx(ret.i, ret.j);
    }

    inline uint32_t key_to_idx(const KeyT& k, bool mayInsert = false) {
        return rec_to_idx(value_type(k), mayInsert);
    }

    inline const value_type& idx_to_rec(uint32_t idx) const {
        simple_ret_t ret = internal_find_at(idx);
        return m_submaps[ret.i].load(std::memory_order_relaxed)->idx_to_rec(ret.j);
    }

private:
    // This limits primary submap size to 2^31 ~= 2 billion, secondary submap
    // size to 2^(32 - s_num_submap_bits - 1) = 2^27 ~= 130 million, and num subMaps
    // to 2^s_num_submap_bits = 16.
    static const uint32_t  s_num_submap_bits   = 4;
    static const uint32_t  s_secondary_map_bit = 1u << 31; // Highest bit
    static const uint32_t  s_submap_idx_shift  = 32 - s_num_submap_bits - 1;
    static const uint32_t  s_submap_idx_mask   = (1 << s_submap_idx_shift) - 1;
    static const uint32_t  s_num_submaps       = 1  << s_num_submap_bits;
    static const PSubMap   s_locked_ptr;

    struct simple_ret_t {
        uint32_t i;
        size_t   j;
        bool     success;
        simple_ret_t(uint32_t ii, size_t jj, bool s) : i(ii), j(jj), success(s) {}
        simple_ret_t() {}
    };

    template <class T>
    simple_ret_t internal_insert (const KeyT& k, T&& value);
    simple_ret_t internal_find   (const KeyT& k) const;
    simple_ret_t internal_find_at(uint32_t  idx) const;

    char_alloc              m_allocator;
    std::atomic<PSubMap>    m_submaps[s_num_submaps];
    std::atomic<uint32_t>   m_alloc_num_maps;
    const config            m_config;

    inline bool try_lock_map(int idx) {
        PSubMap val = nullptr;
        return m_submaps[idx].compare_exchange_strong
                (val, s_locked_ptr, std::memory_order_acquire);
    }

    static inline uint32_t encode_idx(uint32_t a_submap, uint32_t a_submap_idx);

}; // atomic_hash_map

} // namespace utxx

#include <utxx/atomic_hash_map.hxx>