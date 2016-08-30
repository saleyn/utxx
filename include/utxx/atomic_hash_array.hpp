//----------------------------------------------------------------------------
/// \file   atomic_hash_array.hpp
/// \author Spencer Ahrens
//----------------------------------------------------------------------------
/// \brief A building block for atomic_hash_map.
///
/// This file is taken from open-source folly library with some enhancements
/// done by Serge Aleynikov to support hosting hash map/array in shared memory.
///
/// It provides the core lock-free functionality,
/// but is limitted by the fact that it cannot
/// grow past it's initialization size and is a little more awkward (no public
/// constructor, for example).  If you're confident that you won't run out of
/// space, don't mind the awkardness, and really need bare-metal performance,
/// feel free to use AHA directly.
///
/// Check out atomic_hash_map.h for more thorough documentation on perf and
/// general pros and cons relative to other hash maps.
///
/// @author Spencer Ahrens <sahrens@fb.com>
/// @author Jordan DeLong  <delong.j@fb.com>
///
/// Contributor: Serge Aleynikov <saleyn@gmail.com>
///   (added support for offset pointers and shared memory allocation)
///
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
#define _UTXX_ATOMICHASHARRAY_HPP_

#include <atomic>
#include <type_traits>
#include <utxx/compiler_hints.hpp>
#include <utxx/thread_cached_int.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/noncopyable.hpp>


namespace utxx {

template <class KeyT, class ValueT,
          class HashFcn,
          class EqualFcn,
          class Allocator,
          class SubMap,
          class SubMapPtr>
class atomic_hash_map;

template <class KeyT, class ValueT,
          class HashFcn  = std::hash<KeyT>,
          class EqualFcn = std::equal_to<KeyT>>
class atomic_hash_array : boost::noncopyable {
    static_assert((std::is_convertible<KeyT, int32_t>::value ||
                   std::is_convertible<KeyT, int64_t>::value ||
                   std::is_convertible<KeyT, const void*>::value),
                 "You are trying to use atomic_hash_array with disallowed key "
                 "types.  You must use atomically compare-and-swappable integer "
                 "keys, or a different container class.");
public:
    using key_type        = KeyT;
    using mapped_type     = ValueT;
    using value_type      = std::pair<const KeyT, ValueT>;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;

    const size_t m_capacity;
    const size_t m_max_entries;

    template<class ContT, class IterVal>
    struct  aha_iterator;

    using const_iterator = aha_iterator<const atomic_hash_array,const value_type>;
    using iterator       = aha_iterator<atomic_hash_array,value_type>;

    // You really shouldn't need this if you use the SmartPtr provided by create,
    // but if you really want to do something crazy like stick the released
    // pointer into a DescriminatedPtr or something, you'll need this to clean up
    // after yourself.
    template <class Allocator>
    static void destroy(atomic_hash_array*, Allocator&);

    const EqualFcn& eq_fcn() const { return m_eq_fun; }
    const HashFcn&  hs_fcn() const { return m_hash_fun;  }
private:
    const size_t    m_anchor_mask;
    const KeyT      m_empty_key;
    const KeyT      m_locked_key;
    const KeyT      m_erased_key;
    const HashFcn   m_hash_fun;
    const EqualFcn  m_eq_fun;

    template <class Allocator>
    class deleter {
        Allocator& m_alloc;
    public:
        deleter(Allocator& a_alloc) : m_alloc(a_alloc) {}
        void operator=(const deleter& a_rhs) {}

        void operator()(atomic_hash_array* p) {
            atomic_hash_array::destroy(p, m_alloc);
        }
    };

    bool   is_key_eq   (const KeyT& a, const KeyT& b) const { return m_eq_fun(a,b); }
    size_t hash        (const KeyT& a)                const { return m_hash_fun(a); }

    bool   is_empty_eq (const KeyT& a) const { return m_eq_fun(m_empty_key, a); }
    bool   is_locked_eq(const KeyT& a) const { return m_eq_fun(m_locked_key,a); }
    bool   is_erased_eq(const KeyT& a) const { return m_eq_fun(m_erased_key,a); }

public:
    template <typename Allocator>
    using SmartPtr = std::unique_ptr<atomic_hash_array, deleter<Allocator>>;

    /*
    * create --
    *
    *   Creates atomic_hash_array objects.  Use instead of constructor/destructor.
    *
    *   We do things this way in order to avoid the perf penalty of a second
    *   pointer indirection when composing these into atomic_hash_map, which needs
    *   to store an array of pointers so that it can perform atomic operations on
    *   them when growing.
    *
    *   Instead of a mess of arguments, we take a max size and a Config struct to
    *   simulate named ctor parameters.  The Config struct has sensible defaults
    *   for everything, but is overloaded - if you specify a positive capacity,
    *   that will be used directly instead of computing it based on
    *   max_load_factor.
    *
    *   Create returns an AHA::SmartPtr which is a unique_ptr with a custom
    *   deleter to make sure everything is cleaned up properly.
    */
    struct config {
        KeyT      m_empty_key;
        KeyT      m_locked_key;
        KeyT      m_erased_key;
        HashFcn   m_hash_fun;
        EqualFcn  m_eq_fun;
        double    m_max_load_factor;
        double    m_growth_factor;
        int       m_entry_cnt_thr_cache_sz;
        size_t    m_capacity; // if positive, overrides max_load_factor

        static constexpr const double def_max_load_factor = 0.8;

        config
        (
            const KeyT&     a_empty_key       = (KeyT)-1,
            const KeyT&     a_locked_key      = (KeyT)-2,
            const KeyT&     a_erased_key      = (KeyT)-3,
            const HashFcn&  a_hash            = HashFcn(),
            const EqualFcn& a_equal           = EqualFcn(),
            double          a_max_load_factor = def_max_load_factor,
            double          a_growth_factor   = -1,
            int             a_cnt_cache_sz    = 1000,
            size_t          a_capacity        = 0
        ) : m_empty_key             (a_empty_key),
            m_locked_key            (a_locked_key),
            m_erased_key            (a_erased_key),
            m_hash_fun              (a_hash),
            m_eq_fun                (a_equal),
            m_max_load_factor       (a_max_load_factor),
            m_growth_factor         (a_growth_factor),
            m_entry_cnt_thr_cache_sz(a_cnt_cache_sz),
            m_capacity              (a_capacity)
        {}

        // Returns memory size needed for allocating given number of elements.
        // Capacity argument is adjusted by the max_load_factor
        static size_t memory_size(size_t& a_capacity, double a_max_load_factor) {
            a_capacity /= a_max_load_factor;
            return memory_size(a_capacity);
        }
        static size_t memory_size(size_t a_capacity) {
            return sizeof(atomic_hash_array<KeyT, ValueT, HashFcn, EqualFcn>)
                 + sizeof(value_type) * a_capacity;
        }
    };

    static const config s_def_config;

    template <class Allocator>
    static SmartPtr<Allocator> create(
        size_t a_max_sz, Allocator& a_alloc, const config& = s_def_config);

    iterator       find(KeyT k) { return iterator(this, internal_find(k).idx); }
    const_iterator find(KeyT k) const {
        return const_cast<atomic_hash_array*>(this)->find(k);
    }

    /*
    * insert --
    *
    *   Returns a pair with iterator to the element at r.first and bool success.
    *   Retrieve the index with ret.first.index().
    *
    *   Fails on key collision (does not overwrite) or if map becomes
    *   full, at which point no element is inserted, iterator is set to end(),
    *   and success is set false.  On collisions, success is set false, but the
    *   iterator is set to the existing entry.
    */
    std::pair<iterator,bool> insert(const value_type& r) {
        simple_ret_t ret = internal_insert(r.first, r.second);
        return std::make_pair(iterator(this, ret.idx), ret.success);
    }
    std::pair<iterator,bool> insert(value_type&& r) {
        simple_ret_t ret = internal_insert(r.first, std::move(r.second));
        return std::make_pair(iterator(this, ret.idx), ret.success);
    }

    // returns the number of elements erased - should never exceed 1
    size_t erase(const KeyT& k);

    // clears all keys and values in the map and resets all counters.  Not thread
    // safe.
    void clear();

    // Exact number of elements in the map - note that read_full() acquires a
    // mutex.  See folly/thread_cached_int.h for more details.
    size_t size() const {
        return m_num_entries.read_full() -
               m_num_erases.load(std::memory_order_relaxed);
    }

    bool            empty() const { return size() == 0;                      }

    iterator        begin()       { return iterator(this, 0);                }
    iterator        end()         { return iterator(this, m_capacity);       }
    const_iterator  begin() const { return const_iterator(this, 0);          }
    const_iterator  end()   const { return const_iterator(this, m_capacity); }

    // See atomic_hash_map::find_at - access elements directly
    // WARNING: The following 2 functions will fail silently for hashtable
    // with capacity > 2^32
    iterator find_at(uint32_t idx) {
        assert(idx < m_capacity);
        return iterator(this, idx);
    }
    const_iterator find_at(uint32_t idx) const {
        return const_cast<atomic_hash_array*>(this)->find_at(idx);
    }

    iterator       make_iter(size_t idx)       { return iterator(this, idx); }
    const_iterator make_iter(size_t idx) const { return const_iterator(this, idx); }

    // The max load factor allowed for this map
    double max_load_factor() const { return ((double) m_max_entries) / m_capacity; }

    void entry_count_thr_cache_size(uint32_t newSize) {
        m_num_entries.cache_size(newSize);
        m_pend_entries.cache_size(newSize);
    }

    int entry_count_thr_cache_size() const { return m_num_entries.cache_size(); }

private:
    template <class K, class V, class H, class E, class A, class S, class SM>
    friend class atomic_hash_map;

    struct simple_ret_t {
        size_t  idx;
        bool    success;
        simple_ret_t(size_t i, bool s) : idx(i), success(s) {}
        simple_ret_t() {}
    };

    template <class T>
    simple_ret_t internal_insert(const KeyT& key, T&& value);
    simple_ret_t internal_find  (const KeyT& key) const;

    static std::atomic<KeyT>* cell_pkey(const value_type& r) {
        // We need some illegal casting here in order to actually store
        // our value_type as a std::pair<const,>.
        static_assert(sizeof(std::atomic<KeyT>) == sizeof(KeyT),
                    "std::atomic is implemented in an unexpected way");
        return const_cast<std::atomic<KeyT>*>(
                reinterpret_cast<std::atomic<KeyT> const*>(&r.first));
    }

    static KeyT load_key_relaxed(const value_type& r) {
        return cell_pkey(r)->load(std::memory_order_relaxed);
    }

    static KeyT load_key_acquire(const value_type& r) {
        return cell_pkey(r)->load(std::memory_order_acquire);
    }

    // Fun with thread local storage - atomic increment is expensive
    // (relatively), so we accumulate in the thread cache and periodically
    // flush to the actual variable, and walk through the unflushed counts when
    // reading the value, so be careful of calling size() too frequently.  This
    // increases insertion throughput several times over while keeping the count
    // accurate.
    thread_cached_int<int64_t> m_num_entries;  ///< Successful key inserts
    thread_cached_int<int64_t> m_pend_entries; ///< Used by internal_insert
    std::atomic<int64_t>       m_is_full;      ///< Used by internal_insert
    std::atomic<int64_t>       m_num_erases;   ///< Successful key erases

    //-------------------------------------------------------------------------
    // This must be the last field of this class
    //-------------------------------------------------------------------------
    value_type                 m_cells[0];

    // Force constructor/destructor private since create/destroy should be
    // used externally instead
    atomic_hash_array(size_t capacity, const config& config) noexcept;

    ~atomic_hash_array() {}

    inline void unlock_cell(value_type* const cell, const KeyT& newKey) {
        cell_pkey(*cell)->store(newKey, std::memory_order_release);
    }

    inline bool try_lock_cell(value_type* const cell) {
        KeyT expect = m_empty_key;
        return cell_pkey(*cell)->compare_exchange_strong
                (expect, m_locked_key, std::memory_order_acq_rel);
    }

    inline size_t key_to_anchor_idx(const KeyT& k) const {
        const  size_t hashVal = hash(k);
        const  size_t probe   = hashVal & m_anchor_mask;
        return likely(probe   < m_capacity) ? probe : hashVal % m_capacity;
    }

    inline size_t probe_next(size_t idx, size_t numProbes) const {
        //idx += numProbes; // quadratic probing
        idx += 1; // linear probing
        // Avoid modulus because it's slow
        return likely(idx < m_capacity) ? idx : (idx - m_capacity);
    }
}; // atomic_hash_array

} // namespace utxx

#include <utxx/atomic_hash_array.hxx>