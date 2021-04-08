//----------------------------------------------------------------------------
/// \file  alloc_fixed_pool.hpp
//----------------------------------------------------------------------------
/// \brief This module implements a concurrent lock-free fixed size pool
/// manager for objects allocated in the heap or shared memory.
/// Modeled after IBM free-list algorithm.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-21
//----------------------------------------------------------------------------
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

#include <boost/type_traits.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/interprocess/offset_ptr.hpp>

#include <utxx/math.hpp>
#include <utxx/meta.hpp>
#include <utxx/atomic.hpp>
#include <utxx/error.hpp>
#ifdef DEBUG
#include <iomanip>
#endif

namespace utxx   {
namespace memory {
namespace detail {

using namespace boost::interprocess;

/// Template arguments:
///    PointerType - must be either offset_ptr<char> or char*
template <class PointerType>
class fixed_size_object_pool {
    typedef PointerType pointer_type;
    typedef PointerType value_type;

    typedef struct {
        #ifdef USE_PID_RECOVERY
        unsigned short freed; // set to true when free is called
        unsigned short owner;
        #endif
        pointer_type   next;
    } object_t;

    static const unsigned int   s_magic        = 0xFFEE8899;
    static const unsigned int   s_index_mask   = 0x0000FFFF;
    static const unsigned int   s_version_mask = 0xFFFF0000;
    static const unsigned int   s_version_inc  = 0x00010000;

    const unsigned int          m_magic;
    const size_t                m_object_size;
    const pointer_type          m_begin;       // offset from this till begin of last obj
    const pointer_type          m_end;         // offset from this till end of last object
    const pointer_type          m_pool_end;    // end of addressable range managed by pool
    const size_t                m_object_count;
    pointer_type       volatile m_next;
    size_t             volatile m_free_list;
    long               volatile m_available;   // only valid when compiled with DEBUG

    fixed_size_object_pool(size_t bytes, size_t object_size);

    size_t object_idx(const pointer_type& p) const {
        return (1 + (p - m_begin) / m_object_size) & s_index_mask;
    }

    object_t* head_to_object(size_t head) const {
        return reinterpret_cast<object_t*>(&*m_begin + ((head & s_index_mask)-1)*m_object_size);
    }

    size_t object_to_head(size_t head, const pointer_type& p) const {
        return new_head_version(head) | object_idx(p);
    }

    static size_t new_head_version(size_t old_head) {
        return (old_head & s_version_mask) + s_version_inc;
    }

    static pid_t get_pid() {
        static pid_t pid = ::getpid();
        return pid;
    }
public:
    /// Initialize the pool of fixed size objects.
    static fixed_size_object_pool& create(void* storage, size_t bytes, size_t object_size) {
        return *new (static_cast<fixed_size_object_pool*>(storage))
            fixed_size_object_pool(bytes, object_size);
    }

    /// Attach a client to the pool pointed by storage
    static fixed_size_object_pool& attach(void* storage, size_t bytes, size_t object_size) {
        fixed_size_object_pool* pool = static_cast<fixed_size_object_pool*>(storage);
        if (storage == NULL)
            throw badarg_error("Empty storage provided!");
        if (pool->end() != static_cast<char*>(storage) + bytes)
            throw badarg_error("Wrong pool size (requested:", bytes,
                               ", found: ", (pool->end() - static_cast<char*>(storage)), ')');
        if (pool->object_size() != object_size)
            throw badarg_error("Invalid object size (requested:",
                    object_size, ", found: ", pool->object_size(), ')');
        return *pool;
    }

    /// @return storage size needed to hold <count> objects of size <object_size>.
    template <size_t object_size, size_t count>
    struct storage_size {
        enum { 
            obj_size = object_size + sizeof(object_t),
            value    = obj_size * (sizeof(fixed_size_object_pool)/obj_size + 1)
                     + count * obj_size
        };
    };

    /// Allocate a object of size object_size(). This operation is thread-safe.
    void* allocate();

    /// Free a object by returning it to the pool. This operation is thread-safe.
    void free(void* object);

    /// @return object size managed by this pool.
    size_t object_size() const { return m_object_size - sizeof(object_t); }

    /// Only valid when compiled with DEBUG
    size_t available()   const { return m_available; }

    /// End of addressable range managed by this pool
    const pointer_type end() const { return m_pool_end; }

    /// Return maximum pool capacity
    size_t capacity() const { return m_object_count; }

    #ifdef DEBUG
    void dump(std::ostream& out) const;
    void info(void* p, size_t& obj_idx, size_t& next_idx) const;
    #endif

    /// Reclaim allocated objects owned by <died_pid> by moving them to free list.
    void reclaim_objects(pid_t died_pid);
};

} // namespace detail

/// Use the <heap_fixed_size_object_pool> for pooling objects in heap memory
typedef detail::fixed_size_object_pool<char*> heap_fixed_size_object_pool;

/// Use the <heap_fixed_size_object_pool> for pooling objects in shared memory
typedef detail::fixed_size_object_pool< 
    boost::interprocess::offset_ptr<char> >   shmem_fixed_size_object_pool;

//-----------------------------------------------------------------------------
// IMPLEMENTATION
//-----------------------------------------------------------------------------

namespace detail {

template <class PointerType>
fixed_size_object_pool<PointerType>
::fixed_size_object_pool(size_t bytes, size_t object_size)
    : m_magic(s_magic)
    , m_object_size(object_size + sizeof(object_t))
    , m_begin((char*)this + m_object_size * (sizeof(fixed_size_object_pool)/m_object_size + 1))
    , m_end(m_begin + ((bytes - (&*m_begin - (char*)this)) / m_object_size) * m_object_size)
    , m_pool_end((char*)this + bytes)
    , m_object_count((m_end-m_begin) / m_object_size)
    , m_next(NULL)
    , m_free_list(0)
    , m_available(0)
{
    BOOST_STATIC_ASSERT((
           boost::is_same<PointerType, char*>::value
        || boost::is_same<PointerType, offset_ptr<char> >::value));

    BOOST_ASSERT(bytes > m_object_size + sizeof(fixed_size_object_pool)); // Too little storage
    BOOST_ASSERT(m_begin + m_object_size*m_object_count <= m_end); // Wrong object size!
    BOOST_ASSERT(m_begin + m_object_size*m_object_count >  m_end - m_object_size); // Wrong object count

    if (m_begin+m_object_size >= m_end)
        throw badarg_error("Pool size too small!");
    if (m_object_count > s_index_mask)
        throw badarg_error("Pool size too large!");

    pointer_type p, last = m_end - m_object_size;
    // Initialize the free list
    for (p = m_begin; p < m_end; p += m_object_size) {
        object_t& pg = reinterpret_cast<object_t&>(*p);
        pg.next  = p == last ? NULL : p + m_object_size;
        #ifdef USE_PID_RECOVERY
        pg.owner = 0;
        #endif
        #ifdef DEBUG
        atomic::inc(&m_available);
        #endif
    }
    m_free_list = 1;
    atomic::memory_barrier();
}

template <class PointerType>
void* fixed_size_object_pool<PointerType>
::allocate() 
{
    BOOST_ASSERT(m_magic == s_magic);

    while(1) {
        size_t old_head = m_free_list;

        if ((old_head & s_index_mask) == 0)
            return NULL;

        // Free list is not empty
        object_t* p = head_to_object(old_head);

        #ifndef NDEBUG
        pointer_type pt = reinterpret_cast<char*>(p);
        #endif
        BOOST_ASSERT(m_begin <= pt && pt < m_end);
            //   "Invalid free list head (old_head=" << 
            //   (old_head & s_index_mask) << ", ptr_idx=" << 
            //   object_idx(pt) << ")!");

        size_t new_head = p->next ? object_to_head(old_head, p->next)
                                  : new_head_version(old_head);

        BOOST_ASSERT((new_head & s_index_mask) >= 0 &&
               (new_head & s_index_mask) <= m_object_count);
            // "Invalid new list head index " << (new_head & s_index_mask) <<
            // " (old_head=" << (old_head & s_index_mask) <<
            // ", avail=" << m_available << ')');

        if (atomic::cas(&m_free_list, old_head, new_head)) {
            #ifdef USE_PID_RECOVERY
            p->freed = 0;
            p->owner = get_pid();
            #endif
            #ifdef DEBUG
            atomic::add(&m_available, -1);
            #endif
            return (void *)(++p);
        }
    }
}

template <class PointerType>
void fixed_size_object_pool<PointerType>
::free(void* object)
{
    if (object == NULL) return;

    object_t* obj = static_cast<object_t*>(object) - 1;
    const pointer_type p = reinterpret_cast<char*>(obj);

    BOOST_ASSERT(m_begin <= p && p < m_end); // "Invalid object pointer " << obj
    BOOST_ASSERT(((p - m_begin) % m_object_size) == 0); // "Invalid object alignment!"

    #ifdef USE_PID_RECOVERY
    obj->freed = 1;
    #endif

    size_t old_head, new_head;

    do {
        old_head  = m_free_list;
        obj->next = (old_head & s_index_mask) == 0 
                  ? NULL
                  : reinterpret_cast<char*>(head_to_object(old_head));
        new_head = object_to_head(old_head, p);

        BOOST_ASSERT((new_head & s_index_mask) >= 0 && 
               (new_head & s_index_mask) <= m_object_count);
            // "Invalid old head of free list: " << (old_head & s_index_mask) <<
            // " (new_head=" << (new_head & s_index_mask) << 
            // ", avail=" << m_available << ')');
    } while (!atomic::cas(&m_free_list, old_head, new_head));

    #ifdef DEBUG
    atomic::inc(&m_available);
    #endif
}

template <class PointerType>
void fixed_size_object_pool<PointerType>
::reclaim_objects(pid_t died_pid) 
{
    #ifdef USE_PID_RECOVERY
    // Walk through all objects and move objects owned by died_pid
    // to free list.
    for (pointer_type p = m_begin; offset < m_end; offset += m_object_size) {
        object_t& pg = reinterpret_cast<object_t&>(*p);
        if (pg.owner == died_pid && pg.freed == 1) {
            pg.owner = 0;
            free(&pg + 1);
        }
    }
    #endif
}

#ifdef DEBUG
template <class PointerType>
void fixed_size_object_pool<PointerType>
::dump(std::ostream& out) const
{
    out
        << "Header size..: " << sizeof(fixed_size_object_pool) << std::endl
        << "Begin offset.: " << ((char*)&*m_begin - (char*)this) << std::endl
        << "Total bytes..: " << ((char*)&*m_pool_end - (char*)this) << std::endl
        << "Pool size....: " << m_object_count << " objects" << std::endl
        << "Object size..: " << m_object_size << std::endl
        << "Usable space.: " << (m_end - m_begin) << std::endl
        << "Available....: " << m_available << std::endl
        << "Free list....: ";
    if ((m_free_list & s_index_mask) == 0)
        out << "NULL" << std::endl;
    else
        out << (m_free_list & s_index_mask) << " (version: " 
            << (m_free_list >> 16) << ")" << std::endl;

    for(pointer_type p=m_begin; p < m_end; p += m_object_size) {
        object_t& o = reinterpret_cast<object_t&>(*p);
        size_t i  = object_idx(p);
        if (o.next == NULL) 
            out << "  [" << std::setw(6) << i << "] -> NULL" << std::endl; 
        else {
            size_t in = object_idx(o.next);
            out << "  [" << std::setw(6) << i << "] -> [" << std::setw(6) << in << "]" << std::endl; 
        }
    }
}

template <class PointerType>
void fixed_size_object_pool<PointerType>
::info(void* object, size_t& obj_idx, size_t& next_idx) const
{
    object_t* obj = static_cast<object_t*>(object) - 1;
    const pointer_type p = reinterpret_cast<char*>(obj);

    if (p < m_begin || p >= m_end || ((p - m_begin) % m_object_size != 0)) {
        obj_idx = next_idx = 0;
        return;
    }
    obj_idx  = object_idx(p);
    next_idx = obj->next == NULL ? 0 : object_idx(p->next);
}

#endif

} // namespace detail
} // namespace memory
} // namespace utxx
