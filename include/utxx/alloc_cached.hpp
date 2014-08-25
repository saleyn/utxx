//----------------------------------------------------------------------------
/// \file   alloc_cached.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Concurrent lock-free cached allocator
///
/// This module implements a concurrent lock-free cached allocator that
/// uses pools of size class memory chunks and a user-specific allocator
/// for cases when a pool of the requested size class is empty. A size class
/// is a power of 2.  This allocator is not suitable for shared memory
/// interprocess allocations.
//----------------------------------------------------------------------------
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

#ifndef _ALLOC_CACHED_HPP_
#define _ALLOC_CACHED_HPP_

#include <boost/type_traits.hpp>

#include <stdexcept>
#include <utxx/math.hpp>
#include <utxx/meta.hpp>
#include <utxx/atomic.hpp>
#include <utxx/container/concurrent_stack.hpp>
#include <utxx/compiler_hints.hpp>
#ifdef DEBUG
#include <iomanip>
#endif

namespace utxx   {
namespace memory {

using namespace container;

//-----------------------------------------------------------------------------
// CACHED_ALLOCATOR
//-----------------------------------------------------------------------------

/// Concurrent cached allocator that manages memory by partitioning it in size
/// classes of power of 2.  
/// Template arguments:
/// @tparam   T - this is a helper argument, leave the default untouched
/// @tparam   AllocT      - User allocator used when a free list is empty.
/// @tparam   MinSize     - Minimum allocated memory chunk.
/// @tparam   SizeClasses - Max number of size class managed by the allocator.
///                         Objects of size >= 2^SizeClasses are allocated/freed
///                         directly using the AllocT bypassing caching.
template <
    class T, 
    class AllocT        = std::allocator<T>,
    int   MinSize       = 3 * sizeof(long), 
    int   SizeClasses   = 21>
class cached_allocator {
    typedef typename AllocT::template rebind<T>::other UserAllocT;
    typedef versioned_stack::node_t node_t;

    versioned_stack  m_freelist[SizeClasses];
    UserAllocT&      m_alloc;
    volatile long    m_large_objects;

    static UserAllocT& default_allocator() {
        static std::allocator<T> allocator;
        return allocator;
    }

    void* alloc_size_class(size_t size_class);
public:
    typedef ::std::size_t    size_type;
    typedef ::std::ptrdiff_t difference_type;
    typedef T*               pointer;
    typedef const T*         const_pointer;
    typedef T&               reference;
    typedef const T&         const_reference;
    typedef T                value_type;

#if __cplusplus >= 201103L
    // _GLIBCXX_RESOLVE_LIB_DEFECTS
    // 2103. propagate_on_container_move_assignment
    typedef std::true_type propagate_on_container_move_assignment;
#endif

    static const unsigned int max_size_class = SizeClasses-1;
    static const unsigned int min_size       = MinSize;

    template <typename U>
    struct rebind {
        typedef typename AllocT::template rebind<U>::other ArenaAlloc;
        typedef cached_allocator<U, ArenaAlloc, MinSize, SizeClasses> other;
    };

    cached_allocator() : m_alloc(default_allocator()), m_large_objects(0) {}
    cached_allocator(AllocT& alloc) : m_alloc(alloc),  m_large_objects(0) {}

    /// Allocate a count number of objects T. This operation is thread-safe.
    T* allocate(size_t count);

    /// Free an object by returning it to the pool. This operation is thread-safe.
    void free(void* object);

    void deallocate(void* obj, size_t) { free(obj); }

    /// For internal use.
    void free_node(node_t* nd);
    static node_t* to_node(void* p) { return node_t::to_node(p); }

    /// Reallocate an object to new <size>. This operation is thread-safe.
    void* reallocate(void* object, size_t size);

    /// @return number of allocated large objects (whose size is greater than 
    ///         1^SizeClasses.
    size_t large_objects() const { return m_large_objects; }

    /// Use this method for debugging only since determining list length
    /// in lock-free manner is not thread safe.
    int cache_size(size_t size_class) const {
        return size_class > max_size_class ? -1 : m_freelist[size_class].unsafe_size();
    }

    static size_t size_class(void* p) {
        return node_t::to_node(p)->size_class();
    }

    #ifdef DEBUG
    void dump() const;
    #endif
};

//-----------------------------------------------------------------------------
// IMPLEMENTATION
//-----------------------------------------------------------------------------

template <class T, class AllocT, int MinSize, int SizeClasses>
inline T* cached_allocator<T, AllocT, MinSize, SizeClasses>
::allocate(size_t count) 
{
    using namespace container;

    size_t alloc_sz = sizeof(T)*count + versioned_stack::header_size();
    char size_class = (alloc_sz < min_size)
              ? upper_power<min_size, 2>::value 
              : math::upper_log2(alloc_sz);
    return static_cast<T*>(alloc_size_class(size_class));
}

template <class T, class AllocT, int MinSize, int SizeClasses>
inline void cached_allocator<T, AllocT, MinSize, SizeClasses>
::free(void* p)
{
    using namespace container;

    if (p == NULL) return;

    node_t* nd = node_t::to_node(p);
    free_node(nd);
}

template <class T, class AllocT, int MinSize, int SizeClasses>
void* cached_allocator<T, AllocT, MinSize, SizeClasses>
::reallocate(void* p, size_t sz) 
{
    using namespace container;

    if (p == NULL) 
        return allocate(sz);

    node_t* nd = node_t::to_node(p);

    BOOST_ASSERT(nd->valid());

    char old_size_class = nd->size_class();
    size_t alloc_sz     = sz + versioned_stack::header_size();
    char new_size_class = (alloc_sz < min_size)
                        ? log<min_size, 2>::value
                        : math::upper_log2(alloc_sz);
    if (new_size_class <= old_size_class)
        return p;

    // Allocate new node
    void* pnew = alloc_size_class(new_size_class);
    if (!pnew)
        return NULL;
    // Copy old data
    node_t* nnd = node_t::to_node(pnew);
    void* data = nnd->data();
    memcpy(data, nd->data(), old_size_class - versioned_stack::header_size());
    // Free old node
    free(p);
    return data;
}

template <class T, class AllocT, int MinSize, int SizeClasses>
void* cached_allocator<T, AllocT, MinSize, SizeClasses>
::alloc_size_class(size_t size_class) 
{
    using namespace container;

    size_t size = 1 << size_class;

    node_t* nd = unlikely(size_class > max_size_class)
                                ? NULL : m_freelist[size_class].pop();
    if (nd == NULL) {
        nd = reinterpret_cast<node_t*>(m_alloc.allocate(size));
        BOOST_ASSERT((reinterpret_cast<unsigned long>(nd) &
                    versioned_stack::node_t::s_version_mask) == 0);
        new (nd) node_t(size_class);
        if (unlikely(size_class > max_size_class))
            atomic::inc(&m_large_objects);
    }
    return nd->data();
}

template <class T, class AllocT, int MinSize, int SizeClasses>
void cached_allocator<T, AllocT, MinSize, SizeClasses>
::free_node(node_t* nd)
{
    using namespace container;

    BOOST_ASSERT(nd->valid());

    char size_class = nd->size_class();

    if (unlikely((uint8_t)size_class > max_size_class)) {
        m_alloc.deallocate(reinterpret_cast<T*>(nd), 1 << size_class);
        atomic::add(&m_large_objects, -1);
        return;
    }

    m_freelist[(uint8_t)size_class].push(nd);
}

#ifdef DEBUG
template <class T, class AllocT, int MinSize, int SizeClasses>
void cached_allocator<T, AllocT, MinSize, SizeClasses>
::dump() const
{
    std::cout
        << "Large Objects: " << m_large_objects << std::endl
        << "Free lists...: ";
    for (size_t i = 0; i < SizeClasses; ++i) {
        if (i > 0) std::cout << "               ";
        std::cout << '[' << std::setw(2) << i << "]: " << cache_size(i) << std::endl;
    }
}
#endif

} // namespace memory
} // namespace utxx

#endif
