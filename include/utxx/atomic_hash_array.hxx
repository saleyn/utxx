//----------------------------------------------------------------------------
/// \file   atomic_hash_array.hxx
/// \author Spencer Ahrens
//----------------------------------------------------------------------------
/// \brief A building block for atomic_hash_map.
///
/// This file is taken from open-source folly library with some enhancements
/// done by Serge Aleynikov to support hosting hash map/array in shared memory.
//----------------------------------------------------------------------------
// Copyright (C) 2014 Facebook
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

#ifndef _UTXX_ATOMICHASHARRAY_HPP_
#error "This should only be included by atomic_hash_array.hpp"
#endif

#include <assert.h>
#include <utxx/math.hpp>
#include <utxx/compiler_hints.hpp>

namespace utxx {

// atomic_hash_array private constructor --
template <class KeyT, class ValueT,
          class HashFcn, class EqualFcn>
atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>::
atomic_hash_array(size_t capacity, const config& c) noexcept
    : m_capacity    (capacity)
    , m_max_entries (size_t(c.m_max_load_factor * m_capacity + 0.5))
    , m_anchor_mask (math::upper_power(m_capacity, 2) - 1)
    , m_empty_key   (c.m_empty_key)
    , m_locked_key  (c.m_locked_key)
    , m_erased_key  (c.m_erased_key)
    , m_hash_fun    (c.m_hash_fun)
    , m_eq_fun      (c.m_eq_fun)
    , m_num_entries (0, c.m_entry_cnt_thr_cache_sz)
    , m_pend_entries(0, c.m_entry_cnt_thr_cache_sz)
    , m_is_full     (0)
    , m_num_erases  (0)
{}

/*
 * internal_find --
 *
 *   Sets ret.second to value found and ret.index to index
 *   of key and returns true, or if key does not exist returns false and
 *   ret.index is set to m_capacity.
 */
template <class KeyT, class ValueT, class HashFcn, class EqualFcn>
typename atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>::simple_ret_t
atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>::
internal_find(const KeyT& key_in) const {
    assert(!is_empty_eq(key_in));
    assert(!is_locked_eq(key_in));
    assert(!is_erased_eq(key_in));
    for (size_t idx = key_to_anchor_idx(key_in), probes = 0;
         ;      idx = probe_next(idx, probes))
    {
        const KeyT key = load_key_acquire(m_cells[idx]);
        if (likely(is_key_eq(key, key_in)))
            return simple_ret_t(idx, true);

        if (unlikely(is_empty_eq(key)))
            // if we hit an empty element, this key does not exist
            return simple_ret_t(m_capacity, false);

        ++probes;

        if (unlikely(probes >= m_capacity))
            // probed every cell...fail
            return simple_ret_t(m_capacity, false);
    }
}

/*
 * internal_insert --
 *
 *   Returns false on failure due to key collision or full.
 *   Also sets ret.index to the index of the key.  If the map is full, sets
 *   ret.index = m_capacity.  Also sets ret.second to cell value, thus if insert
 *   successful this will be what we just inserted, if there is a key collision
 *   this will be the previously inserted value, and if the map is full it is
 *   default.
 */
template <class KeyT, class ValueT, class HashFcn, class EqualFcn>
template <class T>
typename atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>::simple_ret_t
atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>::
internal_insert(const KeyT& key_in, T&& value) {
    const short NO_NEW_INSERTS = 1;
    const short NO_PENDING_INSERTS = 2;
    assert(!is_empty_eq(key_in));
    assert(!is_locked_eq(key_in));
    assert(!is_erased_eq(key_in));

    size_t idx = key_to_anchor_idx(key_in);
    size_t numProbes = 0;
    for (;;) {
        assert(idx < m_capacity);
        value_type* cell = &m_cells[idx];
        if (is_empty_eq(load_key_relaxed(*cell))) {
        // NOTE: m_is_full is set based on m_num_entries.readFast(), so it's
        // possible to insert more than m_max_entries entries. However, it's not
        // possible to insert past m_capacity.
        ++m_pend_entries;
        if (m_is_full.load(std::memory_order_acquire)) {
            --m_pend_entries;

            // Before deciding whether this insert succeeded, this thread needs to
            // wait until no other thread can add a new entry.

            // Correctness assumes m_is_full is true at this point. If
            // another thread now does ++m_pend_entries, we expect it
            // to pass the m_is_full.load() test above. (It shouldn't insert
            // a new entry.)
            for
            (
                int count=0;
                m_is_full.load(std::memory_order_acquire) != NO_PENDING_INSERTS
                && m_pend_entries.readFull() != 0
                && count < 10000;
                count++
            )
                pthread_yield();

            m_is_full.store(NO_PENDING_INSERTS, std::memory_order_release);

            if (is_empty_eq(load_key_relaxed(*cell)))
                // Don't insert past max load factor
                return simple_ret_t(m_capacity, false);
        } else {
            // An unallocated cell. Try once to lock it. If we succeed, insert here.
            // If we fail, fall through to comparison below; maybe the insert that
            // just beat us was for this very key....
            if (try_lock_cell(cell)) {
                // Write the value - done before unlocking
                try {
                    assert(is_locked_eq(load_key_relaxed(*cell)));
                    /*
                    * This happens using the copy constructor because we won't have
                    * constructed a lhs to use an assignment operator on when
                    * values are being set.
                    */
                    new (&cell->second) ValueT(std::forward<T>(value));
                    unlock_cell(cell, key_in); // Sets the new key
                } catch (...) {
                    // Transition back to empty key---requires handling
                    // locked->empty below.
                    unlock_cell(cell, m_empty_key);
                    --m_pend_entries;
                    throw;
                }
                // Direct comparison rather than EqualFcn ok here
                // (we just inserted it)
                assert(is_key_eq(load_key_relaxed(*cell), key_in));
                --m_pend_entries;
                ++m_num_entries;  // This is a thread cached atomic increment :)
                if (m_num_entries.readFast() >= int(m_max_entries))
                    m_is_full.store(NO_NEW_INSERTS, std::memory_order_relaxed);

                return simple_ret_t(idx, true);
            }
            --m_pend_entries;
        }
        }
        assert(!is_empty_eq(load_key_relaxed(*cell)));
        if (is_locked_eq(load_key_acquire(*cell)))
            for(int n=0; is_locked_eq(load_key_acquire(*cell)) && n < 10000; n++)
                pthread_yield();

        const KeyT thisKey = load_key_acquire(*cell);
        if (is_key_eq(thisKey, key_in))
            // Found an existing entry for our key, but we don't overwrite the
            // previous value.
            return simple_ret_t(idx, false);
        else if (is_empty_eq(thisKey) || is_locked_eq(thisKey))
            // We need to try again (i.e., don't increment numProbes or
            // advance idx): this case can happen if the constructor for
            // ValueT threw for this very cell (the rethrow block above).
            continue;

        ++numProbes;
        if (unlikely(numProbes >= m_capacity))
            // probed every cell...fail
            return simple_ret_t(m_capacity, false);

        idx = probe_next(idx, numProbes);
    }
}


/*
 * erase --
 *
 *   This will attempt to erase the given key key_in if the key is found. It
 *   returns 1 iff the key was located and marked as erased, and 0 otherwise.
 *
 *   Memory is not freed or reclaimed by erase, i.e. the cell containing the
 *   erased key will never be reused. If there's an associated value, we won't
 *   touch it either.
 */
template <class KeyT, class ValueT, class HashFcn, class EqualFcn>
size_t atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>::
erase(const KeyT& key_in) {
    assert(!is_empty_eq(key_in));
    assert(!is_locked_eq(key_in));
    assert(!is_erased_eq(key_in));
    for (size_t idx = key_to_anchor_idx(key_in), numProbes = 0;
        ;
        idx = probe_next(idx, numProbes))
    {
        assert(idx < m_capacity);
        value_type* cell = &m_cells[idx];
        KeyT  curr_key = load_key_acquire(*cell);
        if (is_empty_eq(curr_key) || is_locked_eq(curr_key))
            // If we hit an empty (or locked) element, this key does not exist. This
            // is similar to how it's handled in find().
            return 0;

        if (is_key_eq(curr_key, key_in)) {
            // Found an existing entry for our key, attempt to mark it erased.
            // Some other thread may have erased our key, but this is ok.
            KeyT expect = curr_key;
            if (cell_pkey(*cell)->compare_exchange_strong(expect, m_erased_key)) {
                m_num_erases.fetch_add(1, std::memory_order_relaxed);

                // Even if there's a value in the cell, we won't delete (or even
                // default construct) it because some other thread may be accessing it.
                // Locking it meanwhile won't work either since another thread may be
                // holding a pointer to it.

                // We found the key and successfully erased it.
                return 1;
            }
            // If another thread succeeds in erasing our key, we'll stop our search.
            return 0;
        }

        ++numProbes;

        if (unlikely(numProbes >= m_capacity))
            // probed every cell...fail
            return 0;
    }
}

template <class KeyT, class ValueT, class HashFcn, class EqualFcn>
const typename atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>::config
atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>::s_def_config;

template <class KeyT, class ValueT, class HashFcn, class EqualFcn>
template <class Allocator>
typename atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>
::template SmartPtr<Allocator>
atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>::
create(size_t maxSize, Allocator& alloc, const config& c) {
    assert(c.m_max_load_factor <= 1.0);
    assert(c.m_max_load_factor  > 0.0);
    assert(c.m_empty_key != c.m_locked_key);
    size_t capacity = maxSize;
    size_t sz = config::memory_size(capacity, c.m_max_load_factor);

    auto const mem = alloc.allocate(sz);
    // mem could be an offset ptr if using shared memory, therefore we
    // dereference/reference it to let the compiler know we want the real pointer
    atomic_hash_array* p = reinterpret_cast<atomic_hash_array*>(&*mem);

    try {
        new (p) atomic_hash_array(capacity, c);
    } catch (...) {
        alloc.deallocate(mem, sz);
        throw;
    }

    SmartPtr<Allocator> map(p, alloc);

    /*
    * Mark all cells as empty.
    *
    * Note: we're bending the rules a little here accessing the key
    * element in our cells even though the cell object has not been
    * constructed, and casting them to atomic objects (see cell_pkey).
    * (Also, in fact we never actually invoke the value_type
    * constructor.)  This is in order to avoid needing to default
    * construct a bunch of value_type when we first start up: if you
    * have an expensive default constructor for the value type this can
    * noticeably speed construction time for an AHA.
    */
    for (size_t i=0; i < map->m_capacity; i++) {
        std::atomic<KeyT>* p = cell_pkey(map->m_cells[i]);
        p->store(map->m_empty_key, std::memory_order_relaxed);
    }
    return map;
}

template <class KeyT, class ValueT, class HashFcn, class EqualFcn>
template <class Allocator>
void atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>::
destroy(atomic_hash_array* p, Allocator& alloc) {
    assert(p);

    for(size_t i=0; i < p->m_capacity; i++)
        if (!p->is_empty_eq(p->m_cells[i].first))
            p->m_cells[i].~value_type();

    p->~atomic_hash_array();

    size_t sz = config::memory_size(p->m_capacity);

    typename Allocator::pointer q(reinterpret_cast<char*>(p));
    alloc.deallocate(q, sz);
}

// clear -- clears all keys and values in the map and resets all counters
template <class KeyT, class ValueT, class HashFcn, class EqualFcn>
void atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>::
clear() {
    for(size_t i=0; i < m_capacity; i++) {
        if (!is_empty_eq(m_cells[i].first)) {
            m_cells[i].~value_type();
            *const_cast<KeyT*>(&m_cells[i].first) = m_empty_key;
        }
        assert(is_empty_eq(m_cells[i].first));
    }
    m_num_entries.set(0);
    m_pend_entries.set(0);
    m_is_full.store(0, std::memory_order_relaxed);
    m_num_erases.store(0, std::memory_order_relaxed);
}


// iterator implementation

template <class KeyT, class ValueT, class HashFcn, class EqualFcn>
template <class ContT, class IterVal>
struct atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>::aha_iterator
    : boost::iterator_facade<aha_iterator<ContT,IterVal>,
                             IterVal,
                             boost::forward_traversal_tag>
{
    explicit aha_iterator() : m_aha(0) {}

    // Conversion ctor for interoperability between const_iterator and
    // iterator.  The enable_if<> magic keeps us well-behaved for
    // is_convertible<> (v. the iterator_facade documentation).
    template<class OtherContT, class OtherVal>
    aha_iterator(const aha_iterator<OtherContT,OtherVal>& o,
                 typename std::enable_if<
                     std::is_convertible<OtherVal*,IterVal*>::value
                 >::type* = 0)
        : m_aha(o.m_aha)
        , m_offset(o.m_offset)
    {}

    explicit aha_iterator(ContT* array, size_t offset)
        : m_aha(array)
        , m_offset(offset)
    {
        advancePastEmpty();
    }

    // Returns unique index that can be used with find_at().
    // WARNING: The following function will fail silently for hashtable
    // with capacity > 2^32
    uint32_t index() const { return m_offset; }

private:
    friend class atomic_hash_array;
    friend class boost::iterator_core_access;

    void increment() {
        ++m_offset;
        advancePastEmpty();
    }

    bool equal(const aha_iterator& o) const {
        return m_aha == o.m_aha && m_offset == o.m_offset;
    }

    IterVal& dereference() const {
        return m_aha->m_cells[m_offset];
    }

    void advancePastEmpty() {
        while (m_offset < m_aha->m_capacity && !isValid()) {
        ++m_offset;
        }
    }

    bool isValid() const {
        KeyT key = load_key_acquire(m_aha->m_cells[m_offset]);
        return key != m_aha->m_empty_key  &&
        key != m_aha->m_locked_key &&
        key != m_aha->m_erased_key;
    }

private:
    ContT* m_aha;
    size_t m_offset;
}; // aha_iterator

} // namespace utxx
