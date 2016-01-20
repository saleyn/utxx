//----------------------------------------------------------------------------
/// \file   concurrent_mpsc_queue.hpp
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
#pragma once

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
 * All elements are equally sized of type T.
 */
template <class T, class Allocator = std::allocator<char>>
struct concurrent_mpsc_queue {
    class node {
        node*     m_next;
        mutable T m_data;
    public:
        node(T&&      d)     : m_next(nullptr), m_data(std::forward<T>(d)) {}
        node(const T& d)     : m_next(nullptr), m_data(d) {}

        template<typename... Args>
        node(Args&&... args) : m_next(nullptr), m_data(std::forward<Args>(args)...) {}

        const T&            data() const { return m_data; }
        T&                  data()       { return m_data; }
        constexpr unsigned  size() const { return sizeof(T); }
        node*               next()       { return m_next; }
        void  next(node* a_node)         { m_next = a_node; }
    };

    typedef typename Allocator::template rebind<node>::other Alloc;

    explicit concurrent_mpsc_queue(const Alloc& a_alloc = Alloc())
        : m_head     (nullptr)
        , m_allocator(a_alloc)
    {}

    bool empty() const {
        return m_head.load(std::memory_order_relaxed) == nullptr;
    }

    template <typename... Args>
    node* allocate(Args&&... args) {
        try {
            node*  n = m_allocator.allocate(1);
            new   (n)  node(std::forward<Args>(args)...);
            return n;
        } catch (std::bad_alloc const&) {
            return nullptr;
        }
    }

    /// Insert an element's copy into the queue. 
    bool push(const T& data) {
        try {
            node* n = m_allocator.allocate(1);
            new  (n)  node(data);
            push (n);
            return true;
        } catch (std::bad_alloc const&) {
            return false;
        }
    }

    /// Insert an element into the queue. 
    /// The element must have been previously allocated using allocate() function.
    void push(node* a_node) {
        node*   h;
        do    { h = m_head.load(std::memory_order_relaxed); a_node->next(h); }
        while (!m_head.compare_exchange_weak(h, a_node, std::memory_order_release));
    }

    /// Emplace an element into the queue by constructing the data with given arguments. 
    template <typename... Args>
    bool emplace(Args&&... args) {
        try {
            node* n = allocate(std::forward<Args>(args)...);
            push(n);
            return true;
        } catch (std::bad_alloc const&) {
            return false;
        }
    }

    /// Pop all queued elements in the order of insertion
    ///
    /// Use concurrent_mpsc_queue::free() to deallocate each node
    node* pop_all() {
        node* first = nullptr;
        for (node* tmp, *last = pop_all_reverse(); last; first = tmp) {
            tmp     = last;
            last    = last->next();
            tmp->next(first);
        }
        return first;
    }

    /// Pop all queued elements in the reverse order
    ///
    /// Use concurrent_mpsc_queue::free() to deallocate each node
    node* pop_all_reverse() {
        return m_head.exchange(nullptr, std::memory_order_acquire);
    }

    /// Deallocate a node created by a call to pop_all() or pop_all_reverse()
    void free(node* a_node) {
        a_node->~node();
        m_allocator.deallocate(a_node, a_node->size());
    }

    /// Clear the queue
    void clear() {
        for (node* tmp, *last = pop_all_reverse(); last; last = tmp) {
            tmp  = last->next();
            free(last);
        }
    }
private:
    std::atomic<node*> m_head;
    Alloc              m_allocator;
};

template <class Allocator>
struct concurrent_mpsc_queue<char, Allocator> {

    static_assert(std::is_same<char, typename Allocator::value_type>::value,
                  "Allocator must have char as its value_type!");
    class node {
        node*          m_next;
        const unsigned m_size;
        char           m_data[0];

        friend struct concurrent_mpsc_queue<char, Allocator>;
    public:
        node(unsigned a_sz) : m_next(nullptr), m_size(a_sz) {}

        char*       data()       { return m_data; }
        unsigned    size() const { return m_size; }
        node*       next()       { return m_next; }
        void        next(node* a){ m_next = a;    }

        template <class T>
        T&       to()
        { assert(sizeof(T) == m_size); return *reinterpret_cast<T*>(m_data); }

        template <class T>
        const T& to() const
        { assert(sizeof(T) == m_size); return *reinterpret_cast<const T*>(m_data); }
    };

    explicit concurrent_mpsc_queue(const Allocator& a_alloc = Allocator())
        : m_head     (nullptr)
        , m_allocator(a_alloc)
    {}

    bool empty() const {
        return m_head.load(std::memory_order_relaxed) == nullptr;
    }

    node* allocate(size_t a_size) {
        try {
            node*  n = m_allocator.allocate(sizeof(node) + a_size);
            new   (n)  node(a_size);
            return n;
        } catch(std::bad_alloc const&) {
            return nullptr;
        }
    }

    /// Push an element of type T on the queue by dynamically allocating it and
    /// calling its constructor.
    template <typename T, typename... Args>
    bool emplace(Args&&... args) {
        try {
            node* n    = reinterpret_cast<node*>(m_allocator.allocate(sizeof(node) + sizeof(T)));
            new  (n)     node(sizeof(T));
            T*    data = reinterpret_cast<T*>(n->data());
            new  (data)  T(std::forward<Args>(args)...);
            push (n);
            return true;
        } catch(std::bad_alloc const&) {
            return false;
        }
    }

    template <typename InitLambda>
    bool push(size_t a_size, InitLambda a_fun) {
        try {
            node* n    = reinterpret_cast<node*>(m_allocator.allocate(sizeof(node) + a_size));
            new  (n)     node(a_size);
            a_fun(n->data(), a_size);
            push (n);
            return true;
        } catch(std::bad_alloc const&) {
            return false;
        }
    }

    template <int N>
    bool push(const char (&a_value)[N]) {
        try {
            node*  n  = reinterpret_cast<node*>(m_allocator.allocate(sizeof(node) + N));
            new   (n)   node(N);
            memcpy(n->data(), a_value, N);
            push  (n);
            return true;
        } catch(std::bad_alloc const&) {
            return false;
        }
    }

    template <typename Ch>
    bool push(const std::basic_string<Ch>& a_value) {
        try {
            size_t sz  = a_value.size()+1;
            node*  n   = reinterpret_cast<node*>(m_allocator.allocate(sizeof(node) + sz));
            new   (n)    node(sz);
            memcpy(n->data(), a_value.c_str(), sz);
            push  (n);
            return true;
        } catch(std::bad_alloc const&) {
            return false;
        }
    }

    /// Insert an element into the queue. 
    /// The element must have been previously allocated using allocate() function.
    void push(node* a_node) {
        node*   h;
        do    { h = m_head.load(std::memory_order_relaxed); a_node->next(h); }
        while (!m_head.compare_exchange_weak(h, a_node, std::memory_order_release));
    }

    /// Pop all queued elements in the order of insertion
    ///
    /// Use concurrent_mpsc_queue::free() to deallocate each node
    node* pop_all(void) {
        node* first = nullptr;
        for (node* tmp, *last = pop_all_reverse(); last; first = tmp) {
            tmp         = last;
            last        = last->m_next;
            tmp->m_next = first;
        }
        return first;
    }

    /// Pop all queued elements in the reverse order
    ///
    /// Use concurrent_mpsc_queue::free() to deallocate each node
    node* pop_all_reverse() {
        return m_head.exchange(nullptr, std::memory_order_acquire);
    }

    /// Deallocate a node created by a call to pop_all() or pop_all_reverse()
    void free(node* a_node) {
        a_node->~node();
        m_allocator.deallocate(reinterpret_cast<char*>(a_node), sizeof(node) + a_node->size());
    }

    /// Clear the queue
    void clear() {
        for (node* tmp, *last = pop_all_reverse(); last; last = tmp) {
            tmp  = last->next();
            free(last);
        }
    }
public:
    std::atomic<node*> m_head;
    Allocator          m_allocator;
};

#endif // __cplusplus > 201103L

} // namespace utxx
