//----------------------------------------------------------------------------
/// \file   thread_local.hxx
//----------------------------------------------------------------------------
/// \brief Improved thread local storage for non-trivial types.
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

#include <limits.h>
#include <pthread.h>
#include <assert.h>

#include <mutex>
#include <string>
#include <vector>
#include <cstdlib>
#include <utxx/compiler_hints.hpp>
#include <utxx/error.hpp>

namespace utxx {
namespace thr_local_detail {

/// Base class for deleters
class deleter_base {
public:
    virtual ~deleter_base() {}
    virtual void dispose(void* ptr, tlp_destruct_mode mode) const = 0;
};

/// Simple deleter class that calls delete on the passed-in pointer
template <class Ptr>
class simple_deleter : public deleter_base {
public:
    virtual void dispose(void* p, tlp_destruct_mode mode) const {
        delete static_cast<Ptr>(p);
    }
};

/// Custom deleter that calls a given callable
template <class Ptr, class Deleter>
class custom_deleter : public deleter_base {
public:
    explicit custom_deleter(Deleter d) : m_deleter(d) { }
    virtual void dispose(void* p, tlp_destruct_mode mode) const {
        m_deleter(static_cast<Ptr>(p), mode);
    }
private:
    Deleter m_deleter;
};


/// POD wrapper around an element (a void*) and an associated deleter.
/// This must be POD, as we memset() it to 0 and memcpy() it around.
struct element_wrapper {
    void dispose(tlp_destruct_mode mode) {
        if (ptr != nullptr) {
            assert(deleter != nullptr);
            deleter->dispose(ptr, mode);

            cleanup();
        }
    }

    void* release() {
        auto retPtr = ptr;

        if (ptr != nullptr)
            cleanup();

        return retPtr;
    }

    template <class Ptr>
    void set(Ptr p) {
        assert(ptr == nullptr);
        assert(deleter == nullptr);

        if (p) {
            // We leak a single object here but that is ok.  If we used an
            // object directly, there is a chance that the destructor will be
            // called on that static object before any of the element_wrappers
            // are disposed and that isn't so nice.
            static auto d = new simple_deleter<Ptr>();
            ptr           = p;
            deleter       = d;
            ownsDeleter   = false;
        }
    }

    template <class Ptr, class Deleter>
    void set(Ptr p, Deleter d) {
        assert(ptr     == nullptr);
        assert(deleter == nullptr);
        if (p) {
            ptr         = p;
            deleter     = new custom_deleter<Ptr,Deleter>(d);
            ownsDeleter = true;
        }
    }

    void cleanup() {
        if (ownsDeleter)
            delete deleter;
        ptr         = nullptr;
        deleter     = nullptr;
        ownsDeleter = false;
    }

    void*             ptr;
    deleter_base*     deleter;
    bool              ownsDeleter;
};

/**
 * Per-thread entry.  Each thread using a static_meta object has one.
 * This is written from the owning thread only (under the lock), read
 * from the owning thread (no lock necessary), and read from other threads
 * (under the lock).
 */
struct thread_entry {
    element_wrapper*  elements;
    size_t            capacity;
    thread_entry*     next;
    thread_entry*     prev;
};

// Held in a singleton to track our global instances.
// We have one of these per "Tag", by default one for the whole system
// (Tag=void).
//
// Creating and destroying thr_local_ptr objects, as well as thread exit
// for threads that use thr_local_ptr objects collide on a lock inside
// static_meta; you can specify multiple Tag types to break that lock.
template <class Tag>
struct static_meta {
    static static_meta<Tag>& instance() {
        // Leak it on exit, there's only one per process and we don't have to
        // worry about synchronization with exiting threads.
        static bool constructed = (m_inst = new static_meta<Tag>());
        (void)constructed; // suppress unused warning
        return *m_inst;
    }

    int              m_next_id;
    std::vector<int> m_free_ids;
    std::mutex       m_lock;
    pthread_key_t    m_pthr_key;
    thread_entry     m_head;

    void push_back(thread_entry* t) {
        t->next =   &m_head;
        t->prev =    m_head.prev;
        m_head.prev->next = t;
        m_head.prev = t;
    }

    void erase(thread_entry* t) {
        t->next->prev = t->prev;
        t->prev->next = t->next;
        t->next = t->prev = t;
    }

    #if !__APPLE__
    static thread_local thread_entry m_thread_entry;
    #endif
    static static_meta<Tag>* m_inst;

    static_meta() : m_next_id(1) {
        m_head.next = m_head.prev = &m_head;
        int ret = pthread_key_create(&m_pthr_key, &on_thread_exit);
        if (ret < 0)
            UTXX_THROW_IO_ERROR(errno, "pthread_key_create failed");

        // pthread_atfork is not part of the Android NDK at least as of n9d. If
        // something is trying to call native fork() directly at all with Android's
        // process management model, this is probably the least of the problems.
    #if !__ANDROID__
        ret = pthread_atfork(/*prepare*/ &static_meta::pre_fork,
                             /*parent*/  &static_meta::on_fork_parent,
                             /*child*/   &static_meta::on_fork_child);
        if (ret < 0)
            UTXX_THROW_IO_ERROR(errno, "pthread_atfork failed");
    #endif
    }

    ~static_meta() {
        std::cerr << "static_meta is supposed to live forever!" << std::endl;
        assert(false);
    }

    static thread_entry* getThreadEntry() {
    #if !__APPLE__
        return &m_thread_entry;
    #else
        thread_entry* thr_entry =
            static_cast<thread_entry*>(pthread_getspecific(m_inst->m_pthr_key));
        if (!thr_entry) {
            thr_entry = new thread_entry();
            int ret = pthread_setspecific(m_inst->m_pthr_key, thr_entry);
            if (ret < 0)
                UTXX_THROW_IO_ERROR(errno, "pthread_setspecific failed");
        }
        return thr_entry;
    #endif
    }

    static void pre_fork()       { instance().m_lock.lock(); }

    static void on_fork_parent() { m_inst->m_lock.unlock();  }

    static void on_fork_child()  {
        // only the current thread survives
        m_inst->m_head.next      = m_inst->m_head.prev = &m_inst->m_head;
        thread_entry* thr_entry  = getThreadEntry();
        // If this thread was in the list before the fork, add it back.
        if (thr_entry->capacity != 0)
            m_inst->push_back(thr_entry);
        m_inst->m_lock.unlock();
    }

    static void on_thread_exit(void* ptr) {
        auto & meta = instance();
    #if !__APPLE__
        thread_entry* thr_entry = getThreadEntry();

        assert(ptr == &meta);
        assert(thr_entry->capacity > 0);
    #else
        thread_entry* thr_entry = static_cast<thread_entry*>(ptr);
    #endif
        {
            std::lock_guard<std::mutex> g(meta.m_lock);
            meta.erase(thr_entry);
            // No need to hold the lock any longer; the thread_entry is private
            // to this thread now that it's been removed from meta.
        }
        for (size_t i=0; i < thr_entry->capacity; i++)
            thr_entry->elements[i].dispose(tlp_destruct_mode::THIS_THREAD);
        free(thr_entry->elements);
        thr_entry->elements = nullptr;
        pthread_setspecific(meta.m_pthr_key, nullptr);

    #if __APPLE__
        // Allocated in getThreadEntry(); free it
        delete thr_entry;
    #endif
    }

    static int create() {
        int id;
        auto& meta = instance();
        std::lock_guard<std::mutex> g(meta.m_lock);
        if (meta.m_free_ids.empty())
            id = meta.m_next_id++;
        else {
            id = meta.m_free_ids.back();
            meta.m_free_ids.pop_back();
        }
        return id;
    }

    static void destroy(size_t id) {
        try {
            auto& meta = instance();
            // Elements in other threads that use this id.
            std::vector<element_wrapper> elements;
            {
                std::lock_guard<std::mutex> g(meta.m_lock);
                for (thread_entry* e = meta.m_head.next; e != &meta.m_head; e = e->next)
                    if (id < e->capacity && e->elements[id].ptr) {
                        elements.push_back(e->elements[id]);

                        /*
                        * Writing another thread's thread_entry from here is fine;
                        * the only other potential reader is the owning thread --
                        * from on_thread_exit (which grabs the lock, so is properly
                        * synchronized with us) or from get(), which also grabs
                        * the lock if it needs to resize the elements vector.
                        *
                        * We can't conflict with reads for a get(id), because
                        * it's illegal to call get on a thread local that's
                        * destructing.
                        */
                        e->elements[id].ptr = nullptr;
                        e->elements[id].deleter = nullptr;
                        e->elements[id].ownsDeleter = false;
                    }
                meta.m_free_ids.push_back(id);
            }
            // Delete elements outside the lock
            for (auto& it : elements)
                it.dispose(tlp_destruct_mode::ALL_THREADS);
        }
        catch (...) {} // Just in case we get a lock error or something anyway...
    }

    /**
    * Reserve enough space in the thread_entry::elements for the item
    * @id to fit in.
    */
    static void reserve(int id) {
        auto& meta = instance();
        thread_entry* thr_entry = getThreadEntry();
        size_t prev_cap = thr_entry->capacity;
        // Growth factor < 2, see folly/docs/FBVector.md; + 5 to prevent
        // very slow start.
        size_t new_cap = static_cast<size_t>((id + 5) * 1.7);
        assert(new_cap > prev_cap);
        element_wrapper* reallocated = nullptr;

        // Need to grow. Note that we can't call realloc, as elements are
        // still linked in meta, so another thread might access invalid memory
        // after realloc succeeds. We'll copy by hand and update our thread_entry
        // under the lock.
#ifdef UTXX_USING_JEMALLOC
        if (usingJEMalloc()) {
            bool success = false;
            size_t new_sz = nallocx(new_cap * sizeof(element_wrapper), 0);

            // Try to grow in place.
            //
            // Note that xallocx(MALLOCX_ZERO) will only zero newly allocated memory,
            // even if a previous allocation allocated more than we requested.
            // This is fine; we always use MALLOCX_ZERO with jemalloc and we
            // always expand our allocation to the real size.
            if (prev_cap * sizeof(element_wrapper) >= jemallocMinInPlaceExpandable) {
                success = (xallocx(thr_entry->elements, new_sz, 0, MALLOCX_ZERO)
                        == new_sz);
            }

            // In-place growth failed.
            if (!success) {
                success = ((reallocated = static_cast<element_wrapper*>(
                            mallocx(new_sz, MALLOCX_ZERO))) != nullptr);
            }

            if (!success)
                throw std::bad_alloc();

            // Expand to real size
            assert(new_sz / sizeof(element_wrapper) >= new_cap);
            new_cap = new_sz / sizeof(element_wrapper);
        } else {  // no jemalloc
#else
        if (1) {
#endif
            // calloc() is simpler than malloc() followed by memset(), and
            // potentially faster when dealing with a lot of memory, as it can get
            // already-zeroed pages from the kernel.
            reallocated = static_cast<element_wrapper*>(
                calloc(new_cap, sizeof(element_wrapper)));
            if (!reallocated)
                throw std::bad_alloc();
        }

        // Success, update the entry
        {
            std::lock_guard<std::mutex> g(meta.m_lock);

            if (prev_cap == 0)
                meta.push_back(thr_entry);

            if (reallocated) {
                /*
                * Note: we need to hold the meta lock when copying data out of
                * the old vector, because some other thread might be
                * destructing a thr_local and writing to the elements vector
                * of this thread.
                */
                memcpy(reallocated, thr_entry->elements,
                    sizeof(element_wrapper) * prev_cap);
                using std::swap;
                swap(reallocated, thr_entry->elements);
            }
            thr_entry->capacity = new_cap;
        }

        free(reallocated);

    #if !__APPLE__
        if (prev_cap == 0)
            pthread_setspecific(meta.m_pthr_key, &meta);
    #endif
    }

    static element_wrapper& get(size_t id) {
        thread_entry* thr_entry = getThreadEntry();
        if (unlikely(thr_entry->capacity <= id)) {
            reserve(id);
            assert(thr_entry->capacity > id);
        }
        return thr_entry->elements[id];
    }
};

#if !__APPLE__
    template <class Tag>
    thread_local thread_entry static_meta<Tag>::m_thread_entry{nullptr, 0,
                                                        nullptr, nullptr};
#endif
    template <class Tag> static_meta<Tag>* static_meta<Tag>::m_inst = nullptr;

}  // namespace thr_local_detail
}  // namespace utxx