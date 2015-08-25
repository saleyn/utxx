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

// Note that readFull requires holding a lock and iterating through all of the
// thread local objects with the same Tag, so if you have a lot of
// thread_cached_int's you should considering breaking up the Tag space even
// further.
template <class IntT, class Tag=IntT>
class thread_cached_int : boost::noncopyable {
    struct IntCache;

    public:
    explicit thread_cached_int(IntT initialVal = 0, uint32_t cacheSize = 1000)
        : target_(initialVal), cacheSize_(cacheSize) {
    }

    void increment(IntT inc) {
        auto cache = cache_.get();
        if (unlikely(cache == nullptr || cache->parent_ == nullptr)) {
        cache = new IntCache(*this);
        cache_.reset(cache);
        }
        cache->increment(inc);
    }

    // Quickly grabs the current value which may not include some cached
    // increments.
    IntT readFast() const { return target_.load(std::memory_order_relaxed); }

    // Reads the current value plus all the cached increments.  Requires grabbing
    // a lock, so this is significantly slower than readFast().
    IntT readFull() const {
        IntT ret = readFast();
        for (const auto& cache : cache_.access_all_threads())
            if (!cache.reset_.load(std::memory_order_acquire)) {
                ret += cache.val_.load(std::memory_order_relaxed);
            }
        return ret;
    }

    // Quickly reads and resets current value (doesn't reset cached increments).
    IntT readFastAndReset() {
        return target_.exchange(0, std::memory_order_release);
    }

    // This function is designed for accumulating into another counter, where you
    // only want to count each increment once.  It can still get the count a
    // little off, however, but it should be much better than calling readFull()
    // and set(0) sequentially.
    IntT readFullAndReset() {
        IntT ret = readFastAndReset();
        for (auto& cache : cache_.access_all_threads())
            if (!cache.reset_.load(std::memory_order_acquire)) {
                ret += cache.val_.load(std::memory_order_relaxed);
                cache.reset_.store(true, std::memory_order_release);
            }
        return ret;
    }

    void setCacheSize(uint32_t newSize) {
        cacheSize_.store(newSize, std::memory_order_release);
    }

    uint32_t getCacheSize() const { return cacheSize_.load(); }

    thread_cached_int& operator+=(IntT inc) { increment(inc);  return *this; }
    thread_cached_int& operator-=(IntT inc) { increment(-inc); return *this; }
    // pre-increment (we don't support post-increment)
    thread_cached_int& operator++()         { increment(1);    return *this; }
    thread_cached_int& operator--()         { increment(-1);   return *this; }

    // Thread-safe set function.
    // This is a best effort implementation. In some edge cases, there could be
    // data loss (missing counts)
    void set(IntT newVal) {
        for (auto& cache : cache_.access_all_threads())
            cache.reset_.store(true, std::memory_order_release);
        target_.store(newVal, std::memory_order_release);
    }

    // This is a little tricky - it's possible that our IntCaches are still alive
    // in another thread and will get destroyed after this destructor runs, so we
    // need to make sure we signal that this parent is dead.
    ~thread_cached_int() {
        for (auto& cache : cache_.access_all_threads())
            cache.parent_ = nullptr;
    }

private:
    std::atomic<IntT>            target_;
    std::atomic<uint32_t>        cacheSize_;
    thr_local_ptr<IntCache,Tag> cache_; // Must be last for dtor ordering

    // This should only ever be modified by one thread
    struct IntCache {
        thread_cached_int*            parent_;
        mutable std::atomic<IntT>   val_;
        mutable uint32_t            numUpdates_;
        std::atomic<bool>           reset_;

        explicit IntCache(thread_cached_int& parent)
            : parent_(&parent), val_(0), numUpdates_(0), reset_(false)
        {}

        void increment(IntT inc) {
            if (likely(!reset_.load(std::memory_order_acquire))) {
                // This thread is the only writer to val_, so it's fine do do
                // a relaxed load and do the addition non-atomically.
                val_.store(
                    val_.load(std::memory_order_relaxed) + inc,
                    std::memory_order_release
                );
            } else {
                val_.store(inc, std::memory_order_relaxed);
                reset_.store(false, std::memory_order_release);
            }
            ++numUpdates_;
            if (unlikely(numUpdates_ >
                         parent_->cacheSize_.load(std::memory_order_acquire)))
                flush();
        }

        void flush() const {
            parent_->target_.fetch_add(val_, std::memory_order_release);
            val_.store(0, std::memory_order_release);
            numUpdates_ = 0;
        }

        ~IntCache() {
            if (parent_)
                flush();
        }
    };
};

} // namespace utxx
