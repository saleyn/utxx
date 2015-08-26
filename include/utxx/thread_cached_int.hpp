//----------------------------------------------------------------------------
/// \file   thread_cached_int.hpp
/// \author Spencer Ahrens
//----------------------------------------------------------------------------
/// \brief Higher performance (up to 10x) atomic incr using thread caching
///
/// This file is taken from open-source folly library.
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

#include <atomic>

#include <boost/noncopyable.hpp>

#include <utxx/compiler_hints.hpp>
#include <utxx/thread_local.hpp>

namespace utxx {

// Note that read_full requires holding a lock and iterating through all of the
// thread local objects with the same Tag, so if you have a lot of
// thread_cached_int's you should considering breaking up the Tag space even
// further.
template <class IntT, class Tag=IntT>
class thread_cached_int : boost::noncopyable {
    struct int_cache;

public:
    explicit thread_cached_int(IntT a_init = 0, uint32_t a_cache_sz = 1000)
        : m_target(a_init), m_cache_size(a_cache_sz) {
    }

    void increment(IntT inc) {
        auto cache = m_cache.get();
        if (unlikely(cache == nullptr || cache->m_parent == nullptr)) {
        cache = new int_cache(*this);
        m_cache.reset(cache);
        }
        cache->increment(inc);
    }

    // Quickly grabs the current value which may not include some cached
    // increments.
    IntT read_fast() const { return m_target.load(std::memory_order_relaxed); }

    // Reads the current value plus all the cached increments.  Requires grabbing
    // a lock, so this is significantly slower than read_fast().
    IntT read_full() const {
        IntT ret = read_fast();
        for (const auto& cache : m_cache.access_all_threads())
            if (!cache.m_reset.load(std::memory_order_acquire)) {
                ret += cache.m_val.load(std::memory_order_relaxed);
            }
        return ret;
    }

    // Quickly reads and resets current value (doesn't reset cached increments).
    IntT read_fast_and_reset() {
        return m_target.exchange(0, std::memory_order_release);
    }

    // This function is designed for accumulating into another counter, where you
    // only want to count each increment once.  It can still get the count a
    // little off, however, but it should be much better than calling read_full()
    // and set(0) sequentially.
    IntT read_full_and_reset() {
        IntT ret = read_fast_and_reset();
        for (auto& cache : m_cache.access_all_threads())
            if (!cache.m_reset.load(std::memory_order_acquire)) {
                ret += cache.m_val.load(std::memory_order_relaxed);
                cache.m_reset.store(true, std::memory_order_release);
            }
        return ret;
    }

    void cache_size(uint32_t a_size) {
        m_cache_size.store(a_size, std::memory_order_release);
    }

    uint32_t cache_size() const { return m_cache_size.load(); }

    thread_cached_int& operator+=(IntT inc) { increment(inc);  return *this; }
    thread_cached_int& operator-=(IntT inc) { increment(-inc); return *this; }
    // pre-increment (we don't support post-increment)
    thread_cached_int& operator++()         { increment(1);    return *this; }
    thread_cached_int& operator--()         { increment(-1);   return *this; }

    // Thread-safe set function.
    // This is a best effort implementation. In some edge cases, there could be
    // data loss (missing counts)
    void set(IntT a_value) {
        for (auto& cache : m_cache.access_all_threads())
            cache.m_reset.store(true, std::memory_order_release);
        m_target.store(a_value, std::memory_order_release);
    }

    // This is a little tricky - it's possible that our int_caches are still alive
    // in another thread and will get destroyed after this destructor runs, so we
    // need to make sure we signal that this parent is dead.
    ~thread_cached_int() {
        for (auto& cache : m_cache.access_all_threads())
            cache.m_parent = nullptr;
    }

private:
    std::atomic<IntT>            m_target;
    std::atomic<uint32_t>        m_cache_size;
    thr_local_ptr<int_cache,Tag> m_cache; // Must be last for dtor ordering

    // This should only ever be modified by one thread
    struct int_cache {
        thread_cached_int*          m_parent;
        mutable std::atomic<IntT>   m_val;
        mutable uint32_t            m_updates;
        std::atomic<bool>           m_reset;

        explicit int_cache(thread_cached_int& parent)
            : m_parent(&parent), m_val(0), m_updates(0), m_reset(false)
        {}

        void increment(IntT inc) {
            if (likely(!m_reset.load(std::memory_order_acquire))) {
                // This thread is the only writer to m_val, so it's fine do do
                // a relaxed load and do the addition non-atomically.
                m_val.store(
                    m_val.load(std::memory_order_relaxed) + inc,
                    std::memory_order_release
                );
            } else {
                m_val.store(inc, std::memory_order_relaxed);
                m_reset.store(false, std::memory_order_release);
            }
            ++m_updates;
            if (unlikely(m_updates >
                         m_parent->m_cache_size.load(std::memory_order_acquire)))
                flush();
        }

        void flush() const {
            m_parent->m_target.fetch_add(m_val, std::memory_order_release);
            m_val.store(0, std::memory_order_release);
            m_updates = 0;
        }

        ~int_cache() {
            if (m_parent)
                flush();
        }
    };
};

} // namespace utxx
