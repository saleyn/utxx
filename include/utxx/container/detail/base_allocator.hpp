//----------------------------------------------------------------------------
/// \file   base_fifo.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Base implementation of allocators for fifo queues.
//----------------------------------------------------------------------------
// Created: 2010-02-03
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

#ifndef _UTXX_FIFO_BASE_ALLOCATOR_HPP_
#define _UTXX_FIFO_BASE_ALLOCATOR_HPP_

#include <utxx/alloc_cached.hpp>
#include <utxx/alloc_fixed_pool.hpp>
#include <boost/noncopyable.hpp>
#include <utxx/atomic.hpp>
#include <utxx/container/concurrent_stack.hpp>

namespace utxx {
namespace container {
namespace detail {

//-----------------------------------------------------------------------------

template <typename T>
struct node_t {
    T data;
    volatile node_t* next;
    node_t() : next(NULL) {}
    node_t(const T& v) : data(v), next(NULL) {}
};

//-----------------------------------------------------------------------------
// ALLOCATORS
//-----------------------------------------------------------------------------

template <typename T>
struct unbound_allocator {
    node_t<T>* allocate()           const { return new node_t<T>(); }
    void       free(node_t<T>* nd)  const { delete nd; }
};

template <typename T>
class unbound_cached_allocator {
    typedef node_t<T> alloc_node_t;

    struct free_node_t : public versioned_stack::node_t {
        alloc_node_t node;

        static free_node_t* address(alloc_node_t* nd) {
            static free_node_t dummy;
            static int offset = (char*)&dummy.node - (char*)&dummy;
            return reinterpret_cast<free_node_t*>((char*)nd - offset);
        }
    };

    versioned_stack m_free_list;
public:
    alloc_node_t* allocate() {
        free_node_t* nd = reinterpret_cast<free_node_t*>(m_free_list.pop());
        if (nd == NULL)
            nd = new free_node_t();
        return unlikely(nd == NULL) ? NULL : &nd->node;
    }

    void free(node_t<T>* nd) {
        if (unlikely(nd == NULL)) return;
        free_node_t* node = free_node_t::address(nd);
        m_free_list.push(node);
    }
};

template <typename T, int Size>
class bound_allocator {
    char m_memory[memory::heap_fixed_size_object_pool::storage_size<sizeof(node_t<T>), Size>::value];
    memory::heap_fixed_size_object_pool& m_pool;
public:
    bound_allocator() 
        : m_pool(memory::heap_fixed_size_object_pool::create(
                    &m_memory, sizeof(m_memory), sizeof(node_t<T>)))
    {
        BOOST_ASSERT(m_pool.capacity() == Size);
            // "Wrong capacity of the pool " << m_pool.capacity() << " (expected " << Size << ')');
    }

    node_t<T>* allocate() {
        return reinterpret_cast<node_t<T>*>(m_pool.allocate());
    }

    void free(node_t<T>* nd) {
        m_pool.free(nd);
    }
};


} // namespace detail
} // namespace container
} // namespace utxx

#endif // _UTXX_FIFO_BASE_ALLOCATOR_HPP_

