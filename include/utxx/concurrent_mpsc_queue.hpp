//----------------------------------------------------------------------------
/// \file   concurrent_spsc_queue.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Producer/consumer queue.
///
/// Based on code from:
/// https://github.com/facebook/folly/blob/master/folly/concurrent_spsc_queue.h
/// Original public domain version authored by Facebook
/// Modifications by Serge Aleynikov
//----------------------------------------------------------------------------
// Created: 2014-06-10
//----------------------------------------------------------------------------
/*
 ***** BEGIN LICENSE BLOCK *****

 This file is part of the utxx open-source project.

 Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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
#ifndef _UTXX_CONCURRENT_MPSC_QUEUE_HPP_
#define _UTXX_CONCURRENT_MPSC_QUEUE_HPP_

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <boost/noncopyable.hpp>
#include <boost/type_traits.hpp>
#include <utxx/math.hpp>

namespace utxx {

#if __cplusplus >= 201103L

/**
 * A lock-free implementation of the multi-producer-single-consumer queue.
 */
template <class T, class Allocator = std::allocator<char> >
struct concurrent_mpsc_queue {
    struct node {
        T       data;
        node*   next;
        node(T&&      d) : data(std::forward<T>(d)) {}
        node(const T& d) : data(d) {}
    };

    typedef typename Allocator::template rebind<node>::other Alloc;

    explicit concurrent_mpsc_queue(const Alloc& a_alloc = Alloc())
        : m_head     (nullptr)
        , m_allocator(a_alloc)
    {}

    bool empty() const {
        return m_head.load(std::memory_order_relaxed) == nullptr;
    }

    /// Insert an element's copy into the queue. 
    void push(const T& data) {
        node* h, *n = m_allocator.allocate(1);
        new   (n) node(data);
        do    { n->next = h = m_head.load(std::memory_order_relaxed); }
        while (!m_head.compare_exchange_weak(h, n, std::memory_order_release));
    }

    /// Pop all queued elements in the order of insertion
    ///
    /// Use concurrent_mpsc_queue::free() to deallocate each node
    node* pop_all(void) {
        node* first = nullptr;
        for (node* tmp, *last = pop_all_reverse(); last; first = tmp) {
            tmp       = last;
            last      = last->next;
            tmp->next = first;
        }
        return first;
    }

    /// Pop all queued elements in the reverse order
    ///
    /// Use concurrent_mpsc_queue::free() to deallocate each node
    node* pop_all_reverse() {
        return m_head.exchange(nullptr, std::memory_order_consume);
    }

    /// Deallocate a node created by a call to pop_all() or pop_all_reverse()
    void free(node* a_node) {
        a_node->~node();
        m_allocator.deallocate(a_node, sizeof(T));
    }

public:
    std::atomic<node*> m_head;
    Alloc              m_allocator;
};

#endif // __cplusplus > 201103L

} // namespace utxx

#endif //_UTXX_CONCURRENT_MPSC_QUEUE_HPP_
