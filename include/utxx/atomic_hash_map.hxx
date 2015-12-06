//----------------------------------------------------------------------------
/// \file   atomic_hash_array.hpp
/// \author Spencer Ahrens
/// This file is taken from open-source folly library with some enhancements
/// done by Serge Aleynikov to support hosting hash map/array in shared memory
//----------------------------------------------------------------------------
/// \brief A high performance concurrent hash map with int32 or int64 keys.
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
#ifndef _UTXX_ATOMICHASHMAP_HPP_
#error "This should only be included by atomic_hash_map.hpp"
#endif

#include <sched.h>
#include <assert.h>
#include <utxx/compiler_hints.hpp>

namespace utxx {

template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
const typename
atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::config
atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::default_config;

template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
const PSubMap atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
s_locked_ptr = PSubMap(reinterpret_cast<SubMap*>(0x88ul << 48)); // invalid pointer

// atomic_hash_map constructor -- Atomic wrapper that allows growth
// This class has a lot of overhead (184 Bytes) so only use for big maps
template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
atomic_hash_map(size_t size, const config& config, const char_alloc& alloc)
    : m_growth_frac(config.m_growth_factor < 0 ?
                    1.0 - config.m_max_load_factor : config.m_growth_factor)
    , m_allocator(alloc)
    , m_config(config)
{
    assert(config.m_max_load_factor > 0.0 && config.m_max_load_factor < 1.0);
    m_submaps[0].store(SubMap::create(size, m_allocator, m_config).release(),
                       std::memory_order_relaxed);
    auto num_submaps = s_num_submaps;
    for (size_t i=1; i < num_submaps; ++i)
        m_submaps[i].store(nullptr, std::memory_order_relaxed);

    m_alloc_num_maps.store(1, std::memory_order_relaxed);
}

// insert --
template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
std::pair<typename atomic_hash_map<KeyT, ValueT, HashFcn,
                                 EqualFcn, Alloc, SubMap, PSubMap>::iterator, bool>
atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
insert(const key_type& k, const mapped_type& v) {
    simple_ret_t ret = internal_insert(k,v);
    PSubMap      map = m_submaps[ret.i].load(std::memory_order_relaxed);
    return std::make_pair(iterator(this, ret.i, map->make_iter(ret.j)),
                          ret.success);
}

template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
std::pair<typename atomic_hash_map<KeyT, ValueT, HashFcn,
                                 EqualFcn, Alloc, SubMap, PSubMap>::iterator, bool>
atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
insert(const key_type& k, mapped_type&& v) {
    auto ret = internal_insert(k, std::move(v));
    auto map = m_submaps[ret.i].load(std::memory_order_relaxed);
    return std::make_pair(iterator(this, ret.i, map->make_iter(ret.j)),
                          ret.success);
}

// internal_insert -- Allocates new sub maps as existing ones fill up.
template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
template <class T>
typename atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
simple_ret_t
atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
internal_insert(const key_type& key, T&& value) {
  beginInsertInternal:
    // this maintains our state
    int next_map_idx = m_alloc_num_maps.load(std::memory_order_acquire);
    typename SubMap::simple_ret_t ret;
    for (int i=0; i < next_map_idx; ++i) {
        // insert in each map successively.  If one succeeds, we're done!
        auto map = m_submaps[i].load(std::memory_order_relaxed);
        ret = map->internal_insert(key, std::forward<T>(value));
        if (ret.idx == map->m_capacity)
            continue;  //map is full, so try the next one

        // Either collision or success - insert in either case
        return simple_ret_t(i, ret.idx, ret.success);
    }

    // If we made it this far, all maps are full and we need to try to allocate
    // the next one.

    auto prim_submap = m_submaps[0].load(std::memory_order_relaxed);
    if (next_map_idx >= int(s_num_submaps)
        || prim_submap->m_capacity * m_growth_frac < 1.0)
        // Can't allocate any more sub maps.
        throw atomic_hash_map_full_error();

    if (try_lock_map(next_map_idx)) {
        // Alloc a new map and shove it in.  We can change whatever
        // we want because other threads are waiting on us...
        size_t alloc_num_cells = (size_t)
            (prim_submap->m_capacity *
             std::pow(1.0+m_growth_frac, next_map_idx-1));
        size_t new_sz = (int)(alloc_num_cells * m_growth_frac);
#ifndef NDEBUG
        SubMap* p =
#endif
        m_submaps[next_map_idx].load(std::memory_order_relaxed);
        assert(p == s_locked_ptr);
        // create a new map using the settings stored in the first map

        m_submaps[next_map_idx].store
            (SubMap::create(new_sz, m_allocator, m_config).release(),
             std::memory_order_release);

        // Publish the map to other threads.
        m_alloc_num_maps.fetch_add(1, std::memory_order_release);
        assert(next_map_idx+1 == int(m_alloc_num_maps.load(std::memory_order_relaxed)));
    } else {
        // If we lost the race, we'll have to wait for the next map to get
        // allocated before doing any insertion here.
        for (int n=0
            ; next_map_idx >= int(m_alloc_num_maps.load(std::memory_order_acquire))
              && n < 50000
            ; n++)
            sched_yield();
    }

    // Relaxed is ok here because either we just created this map, or we
    // just did a spin wait with an acquire load on m_alloc_num_maps.
    PSubMap map = m_submaps[next_map_idx].load(std::memory_order_relaxed);
    assert(map);
    assert(map != s_locked_ptr);
    ret = map->internal_insert(key, std::forward<T>(value));
    if (ret.idx != map->m_capacity)
        return simple_ret_t(next_map_idx, ret.idx, ret.success);

    // We took way too long and the new map is already full...try again from
    // the top (this should pretty much never happen).
    goto beginInsertInternal;
}

// find --
template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
typename atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::iterator
atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
find(const KeyT& k) {
    simple_ret_t ret = internal_find(k);
    if (!ret.success)
        return end();
    auto subMap = m_submaps[ret.i].load(std::memory_order_relaxed);
    return iterator(this, ret.i, subMap->make_iter(ret.j));
}

template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
typename atomic_hash_map<KeyT, ValueT,
         HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::const_iterator
atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
find(const KeyT& k) const {
    return const_cast<atomic_hash_map*>(this)->find(k);
}

// internal_find --
template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
typename atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
simple_ret_t
atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
internal_find(const KeyT& k) const {
    PSubMap const primaryMap = m_submaps[0].load(std::memory_order_relaxed);
    typename SubMap::simple_ret_t ret = primaryMap->internal_find(k);
    if (likely(ret.idx != primaryMap->m_capacity))
        return simple_ret_t(0, ret.idx, ret.success);

    int const maps_count = m_alloc_num_maps.load(std::memory_order_acquire);
    for (int i=1; i < maps_count; ++i) {
        // Check each map successively.  If one succeeds, we're done!
        PSubMap const map = m_submaps[i].load(std::memory_order_relaxed);
        ret = map->internal_find(k);
        if (likely(ret.idx != map->m_capacity))
            return simple_ret_t(i, ret.idx, ret.success);
    }
    // Didn't find our key...
    return simple_ret_t(maps_count, 0, false);
}

// internal_find_at -- see encode_idx() for details.
template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
typename atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::simple_ret_t
atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
internal_find_at(uint32_t idx) const {
    uint32_t submap_idx, submap_offset;
    if (idx & s_secondary_map_bit) {
        // idx falls in a secondary map
        idx &= ~s_secondary_map_bit;  // unset secondary bit
        submap_idx    = idx >> s_submap_idx_shift;
        assert(submap_idx < m_alloc_num_maps.load(std::memory_order_relaxed));
        submap_offset = idx & s_submap_idx_mask;
    } else {
        // idx falls in primary map
        submap_idx    = 0;
        submap_offset = idx;
    }
    return simple_ret_t(submap_idx, submap_offset, true);
}

// erase --
template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
typename atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::size_type
atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
erase(const KeyT& k) {
    int const num_maps = m_alloc_num_maps.load(std::memory_order_acquire);
    for (int i=0; i < num_maps; ++i)
        // Check each map successively.  If one succeeds, we're done!
        if (m_submaps[i].load(std::memory_order_relaxed)->erase(k))
            return 1;
    // Didn't find our key...
    return 0;
}

// capacity -- summation of capacities of all submaps
template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
size_t atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
capacity() const {
    size_t    tot_cap  = 0;
    int const num_maps = m_alloc_num_maps.load(std::memory_order_acquire);
    for (int i=0; i < num_maps; ++i)
        tot_cap += m_submaps[i].load(std::memory_order_relaxed)->m_capacity;
    return tot_cap;
}

// remaining_space --
// number of new insertions until current submaps are all at max load
template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
size_t atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
remaining_space() const {
    size_t    rem_space = 0;
    int const num_maps  = m_alloc_num_maps.load(std::memory_order_acquire);
    for (int i=0; i < num_maps; ++i) {
        auto  map   = m_submaps[i].load(std::memory_order_relaxed);
        rem_space  += std::max
            (0,  map->m_max_entries - &map->m_num_entries.read_full());
    }
    return rem_space;
}

// clear -- Wipes all keys and values from primary map and destroys
// all secondary maps.  Not thread safe.
template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
void atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
clear() {
    m_submaps[0].load(std::memory_order_relaxed)->clear();
    int const num_maps = m_alloc_num_maps.load(std::memory_order_relaxed);
    for (int i=1; i < num_maps; ++i) {
        auto   map  = m_submaps[i].load(std::memory_order_relaxed);
        assert(map);
        SubMap::destroy(map, m_allocator);
        m_submaps[i].store(nullptr, std::memory_order_relaxed);
    }
    m_alloc_num_maps.store(1, std::memory_order_relaxed);
}

// size --
template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
size_t atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
size() const {
    size_t    tot_size = 0;
    int const num_maps = m_alloc_num_maps.load(std::memory_order_acquire);
    for (int i=0; i < num_maps; ++i)
        tot_size += m_submaps[i].load(std::memory_order_relaxed)->size();
    return tot_size;
}

// encode_idx -- Encode the submap index and offset into return.
// index_ret must be pre-populated with the submap offset.
//
// We leave index_ret untouched when referring to the primary map
// so it can be as large as possible (31 data bits).  Max size of
// secondary maps is limited by what can fit in the low 27 bits.
//
// Returns the following bit-encoded data in index_ret:
//   if subMap == 0 (primary map) =>
//     bit(s)          value
//         31              0
//       0-30  submap offset (index_ret input)
//
//   if subMap > 0 (secondary maps) =>
//     bit(s)          value
//         31              1
//      27-30   which subMap
//       0-26  subMap offset (index_ret input)
template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
inline uint32_t atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::
encode_idx(uint32_t a_submap, uint32_t a_offset) {
    assert((a_offset & s_secondary_map_bit) == 0);  // offset can't be too big
    if (a_submap == 0) return a_offset;
    // Make sure subMap isn't too big
    assert((a_submap >> s_num_submap_bits) == 0);
    // Make sure subMap bits of offset are clear
    assert((a_offset & (~s_submap_idx_mask | s_secondary_map_bit)) == 0);

    // Set high-order bits to encode which submap this index belongs to
    return a_offset | (a_submap << s_submap_idx_shift) | s_secondary_map_bit;
}


// iterator implementation

template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn, class Alloc, class SubMap, class PSubMap>
template<class ContT, class IterVal, class SubIt>
struct atomic_hash_map<KeyT, ValueT, HashFcn, EqualFcn, Alloc, SubMap, PSubMap>::ahm_iterator
    : boost::iterator_facade<ahm_iterator<ContT,IterVal,SubIt>,
                             IterVal,
                             boost::forward_traversal_tag>
{
    explicit ahm_iterator() : m_ahm(0) {}

    // Conversion ctor for interoperability between const_iterator and
    // iterator.  The enable_if<> magic keeps us well-behaved for
    // is_convertible<> (v. the iterator_facade documentation).
    template<class OtherContT, class OtherVal, class OtherSubIt>
    ahm_iterator(const ahm_iterator<OtherContT,OtherVal,OtherSubIt>& o,
                typename std::enable_if<
                std::is_convertible<OtherSubIt,SubIt>::value >::type* = 0)
        : m_ahm   (o.m_ahm)
        , m_submap(o.m_submap)
        , m_subit (o.m_subit)
    {}

    /*
    * Returns the unique index that can be used for access directly
    * into the data storage.
    */
    uint32_t index() const {
        assert(!is_end());
        return m_ahm->encode_idx(m_submap, m_subit.index());
    }

private:
    friend class atomic_hash_map;
    explicit ahm_iterator(ContT* ahm, uint32_t subMap, const SubIt& subIt)
        : m_ahm   (ahm)
        , m_submap(subMap)
        , m_subit (subIt)
    {
        check_advance_to_next_submap();
    }

    friend class boost::iterator_core_access;

    void increment() {
        assert(!is_end());
        ++m_subit;
        check_advance_to_next_submap();
    }

    bool equal(const ahm_iterator& other) const {
        if (m_ahm != other.m_ahm)
            return false;

        if (is_end() || other.is_end())
            return is_end() == other.is_end();

        return m_submap == other.m_submap && m_subit == other.m_subit;
    }

    IterVal& dereference() const { return *m_subit; }

    bool is_end() const { return m_ahm == nullptr; }

    void check_advance_to_next_submap() {
        if (is_end())
            return;

        auto map = m_ahm->m_submaps[m_submap].load(std::memory_order_relaxed);
        if (m_subit == map->end()) {
            // This sub iterator is done, advance to next one
            if (m_submap+1 >= m_ahm->m_alloc_num_maps.load(std::memory_order_acquire))
                m_ahm = nullptr;
            else {
                ++m_submap;
                map = m_ahm->m_submaps[m_submap].load(std::memory_order_relaxed);
                m_subit = map->begin();
            }
        }
    }

private:
    ContT*   m_ahm;
    uint32_t m_submap;
    SubIt    m_subit;
}; // ahm_iterator

} // namespace utxx

