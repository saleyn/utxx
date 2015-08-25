//----------------------------------------------------------------------------
/// \file   thread_local.hpp
/// \author Spencer Ahrens
//----------------------------------------------------------------------------
/// \brief Improved thread local storage for non-trivial types.
///
/// This file is taken from open-source folly library.
///
/// Provides similar speed as pthread_getspecific but only consumes a single
/// pthread_key_t, and 4x faster than boost::thread_specific_ptr).
///
/// Also includes an accessor interface to walk all the thread local child
/// objects of a parent.  access_all_threads() initializes an accessor that holds
/// a global lock *that blocks all creation and destruction of thr_local
/// objects with the same Tag* and can be used as an iterable container.
///
/// Intended use is for frequent write, infrequent read data access patterns
/// such as counters.
///
/// There are two classes here - thr_local and thr_local_ptr. thr_local_ptr
/// has semantics similar to boost::thread_specific_ptr. thr_local is a thin
/// wrapper around thr_local_ptr that manages allocation automatically.
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

#include <boost/iterator/iterator_facade.hpp>
#include <utxx/compiler_hints.hpp>
#include <type_traits>


namespace utxx {

    enum class tlp_destruct_mode {
        THIS_THREAD,
        ALL_THREADS
    };

}  // namespace

#include <utxx/detail/thread_local.hxx>

namespace utxx {

template<class T, class Tag> class thr_local_ptr;

template<class T, class Tag=void>
class thr_local {
public:
    thr_local() { }

    T* get() const {
        T* ptr = m_tlp.get();
        if (likely(ptr != nullptr))
            return ptr;

        // separated new item creation out to speed up the fast path.
        return make_tlp();
    }

    T*   operator->()    const { return get();   }
    T&   operator*()     const { return *get();  }

    void reset(T* p = nullptr) { m_tlp.reset(p); }

    using accessor = typename thr_local_ptr<T,Tag>::accessor;

    accessor access_all_threads() const {
        return m_tlp.access_all_threads();
    }

    // movable
    thr_local(thr_local&&) = default;
    thr_local& operator=(thr_local&&) = default;

private:
    // non-copyable
    thr_local(const thr_local&) = delete;
    thr_local& operator=(const thr_local&) = delete;

    T* make_tlp() const {
        T* ptr = new T();
        m_tlp.reset(ptr);
        return ptr;
    }

    mutable thr_local_ptr<T,Tag> m_tlp;
};

/*
 * The idea here is that __thread is faster than pthread_getspecific, so we
 * keep a __thread array of pointers to objects (thread_entry::elements) where
 * each array has an index for each unique instance of the thr_local_ptr
 * object.  Each thr_local_ptr object has a unique id that is an index into
 * these arrays so we can fetch the correct object from thread local storage
 * very efficiently.
 *
 * In order to prevent unbounded growth of the id space and thus huge
 * thread_entry::elements, arrays, for example due to continuous creation and
 * destruction of thr_local_ptr objects, we keep a set of all active
 * instances.  When an instance is destroyed we remove it from the active
 * set and insert the id into m_free_ids for reuse.  These operations require a
 * global mutex, but only happen at construction and destruction time.
 *
 * We use a single global pthread_key_t per Tag to manage object destruction and
 * memory cleanup upon thread exit because there is a finite number of
 * pthread_key_t's available per machine.
 *
 * NOTE: Apple platforms don't support the same semantics for __thread that
 *       Linux does (and it's only supported at all on i386). For these, use
 *       pthread_setspecific()/pthread_getspecific() for the per-thread
 *       storage.  Windows (MSVC and GCC) does support the same semantics
 *       with __declspec(thread)
 */

template<class T, class Tag=void>
class thr_local_ptr {
public:
    thr_local_ptr() : m_id(thr_local_detail::static_meta<Tag>::create()) { }

    thr_local_ptr(thr_local_ptr&& other) noexcept : m_id(other.m_id) {
        other.m_id = 0;
    }

    thr_local_ptr& operator=(thr_local_ptr&& other) {
        assert(this != &other);
        destroy();
        m_id = other.m_id;
        other.m_id = 0;
        return *this;
    }

    ~thr_local_ptr() { destroy(); }

    T* get() const {
        return static_cast<T*>(thr_local_detail::static_meta<Tag>::get(m_id).ptr);
    }

    T* operator->() const { return get(); }
    T& operator*()  const { return *get(); }

    T* release() {
        thr_local_detail::element_wrapper& w =
            thr_local_detail::static_meta<Tag>::get(m_id);

        return static_cast<T*>(w.release());
    }

    void reset(T* p = nullptr) {
        thr_local_detail::element_wrapper& w =
            thr_local_detail::static_meta<Tag>::get(m_id);
        if (w.ptr != p) {
            w.dispose(tlp_destruct_mode::THIS_THREAD);
            w.set(p);
        }
    }

    explicit operator bool() const { return get() != nullptr; }

    /**
    * reset() with a custom deleter:
    * deleter(T* ptr, tlp_destruct_mode mode)
    * "mode" is ALL_THREADS if we're destructing this thr_local_ptr (and thus
    * deleting pointers for all threads), and THIS_THREAD if we're only deleting
    * the member for one thread (because of thread exit or reset())
    */
    template <class Deleter>
    void reset(T* p, Deleter deleter) {
        thr_local_detail::element_wrapper& w =
            thr_local_detail::static_meta<Tag>::get(m_id);
        if (w.ptr != p) {
            w.dispose(tlp_destruct_mode::THIS_THREAD);
            w.set(p, deleter);
        }
    }

    // Holds a global lock for iteration through all thread local child objects.
    // Can be used as an iterable container.
    // Use access_all_threads() to obtain one.
    class accessor {
        friend class thr_local_ptr<T,Tag>;

        thr_local_detail::static_meta<Tag>& m_meta;
        std::mutex*                        m_lock;
        int                                m_id;

    public:
        class        iterator;
        friend class iterator;

        // The iterators obtained from accessor are bidirectional iterators.
        class iterator
            : public boost::iterator_facade<
                iterator,                               // Derived
                T,                                      // value_type
                boost::bidirectional_traversal_tag
            >
        {   // traversal
            friend class accessor;
            friend class boost::iterator_core_access;
            const accessor* const m_accessor;
            thr_local_detail::thread_entry* m_e;

            void increment() { m_e = m_e->next; incrementToValid(); }

            void decrement() { m_e = m_e->prev; decrementToValid(); }

            T& dereference() const {
                return *static_cast<T*>(m_e->elements[m_accessor->m_id].ptr);
            }

            bool equal(const iterator& other) const {
                return (m_accessor->m_id == other.m_accessor->m_id &&
                        m_e == other.m_e);
            }

            explicit iterator(const accessor* accessor)
                : m_accessor(accessor),
                m_e(&m_accessor->m_meta.m_head) {
            }

            bool valid() const {
                return (m_e->elements &&
                        m_accessor->m_id < int(m_e->capacity) &&
                        m_e->elements[m_accessor->m_id].ptr);
            }

            void incrementToValid() {
                for (; m_e != &m_accessor->m_meta.m_head && !valid(); m_e = m_e->next) { }
            }

            void decrementToValid() {
                for (; m_e != &m_accessor->m_meta.m_head && !valid(); m_e = m_e->prev) { }
            }
        };

        ~accessor() { release(); }

        iterator begin() const { return ++iterator(this); }
        iterator end()   const { return iterator(this);   }

        accessor(const accessor&) = delete;
        accessor& operator=(const accessor&) = delete;

        accessor(accessor&& a_rhs) noexcept
            : m_meta(a_rhs.m_meta)
            , m_lock(a_rhs.m_lock)
            , m_id  (a_rhs.m_id)
        {
            a_rhs.m_id   = 0;
            a_rhs.m_lock = nullptr;
        }

        accessor& operator=(accessor&& other) noexcept {
            // Each Tag has its own unique meta, and accessors with different Tags
            // have different types.  So either *this is empty, or this and other
            // have the same tag.  But if they have the same tag, they have the same
            // meta (and lock), so they'd both hold the lock at the same time,
            // which is impossible, which leaves only one possible scenario --
            // *this is empty.  Assert it.
            assert(&m_meta == &other.meta_);
            assert(m_lock  == nullptr);
            std::swap(m_lock, other.m_lock);
            std::swap(m_id, other.m_id);
        }

        accessor()
            : m_meta(thr_local_detail::static_meta<Tag>::instance())
            , m_lock(nullptr)
            , m_id(0)
        {}

    private:
        explicit accessor(int id)
            : m_meta(thr_local_detail::static_meta<Tag>::instance())
            , m_lock(&m_meta.m_lock)
        {
            m_lock->lock();
            m_id = id;
        }

        void release() {
            if (m_lock) {
                m_lock->unlock();
                m_id = 0;
                m_lock = nullptr;
            }
        }
    };

    // accessor allows a client to iterate through all thread local child
    // elements of this thr_local instance.  Holds a global lock for each <Tag>
    accessor access_all_threads() const {
        static_assert(!std::is_same<Tag, void>::value,
                    "Must use a unique Tag to use the access_all_threads feature");
        return accessor(m_id);
    }

private:
    void destroy() {
        if (m_id)
            thr_local_detail::static_meta<Tag>::destroy(m_id);
    }

    // non-copyable
    thr_local_ptr(const thr_local_ptr&) = delete;
    thr_local_ptr& operator=(const thr_local_ptr&) = delete;

    int m_id;  // every instantiation has a unique id
};

}  // namespace utxx
