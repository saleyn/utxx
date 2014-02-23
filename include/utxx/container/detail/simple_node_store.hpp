// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief simple strie node storage facility
 *
 * Simple strie node store is wrapper around STL allocator,
 * intended for use with strie when adding nodes dynamically
 * is required.
 *
 * \author Dmitriy Kargapolov
 * \version 1.0
 * \since 07 May 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_CONTAINER_DETAIL_SIMPLE_NODE_STORE_HPP_
#define _UTXX_CONTAINER_DETAIL_SIMPLE_NODE_STORE_HPP_

#include <memory>
#include <cstddef>

namespace utxx {
namespace container {
namespace detail {

/**
 * \brief this class implements node store facility
 * \tparam Node node type
 * \tparam Allocator STL allocator to use
 *
 * The Store and SArray types are themself templates
 */
template <typename Node = void, typename Allocator = std::allocator<char> >
class simple_node_store {
public:
    template<typename U>
    struct rebind { typedef simple_node_store<U, Allocator> other; };

    // this store provides allocate/deallocate methods
    static const bool dynamic = true;

    // abstract node pointer
    typedef void* pointer_t;

    // null pointer constant
    static const pointer_t null;

    simple_node_store() : m_node_count(0) {}

    simple_node_store(const Allocator& a_alloc)
        : m_node_allocator(a_alloc), m_char_allocator(a_alloc), m_node_count(0)
    {}

    template<typename T> struct id {};

    template<typename T>
    pointer_t allocate() { return allocate(id<T>()); }

    template<typename T>
    void deallocate(pointer_t a_ptr) { deallocate(a_ptr, id<T>()); }

    // convert abstract pointer to native pointer
    template<typename T>
    T *native_pointer(pointer_t a_ptr) const { return static_cast<T *>(a_ptr); }

    size_t count() const { return m_node_count; }

private:
    // prevent copying
    simple_node_store(const simple_node_store&);
    simple_node_store& operator=(const simple_node_store&);

    // node allocation
    pointer_t allocate(id<Node>) {
        ++m_node_count;
        pointer_t l_ptr = m_node_allocator.allocate(1);
        return new (l_ptr) Node;
    }

    // generic data allocation
    template<typename Data>
    pointer_t allocate(id<Data>) {
        pointer_t l_ptr = m_char_allocator.allocate(sizeof(Data));
        return new (l_ptr) Data;
    }

    // node deallocation
    void deallocate(pointer_t a_ptr, id<Node>) {
        static_cast<Node *>(a_ptr)->~Node();
        m_node_allocator.deallocate(static_cast<Node *>(a_ptr), 1);
        --m_node_count;
    }

    // generic data deallocation
    template<typename Data>
    void deallocate(pointer_t a_ptr, id<Data>) {
        static_cast<Data *>(a_ptr)->~Data();
        m_char_allocator.deallocate((char *)a_ptr, sizeof(Data));
    }

    typedef typename Allocator::template rebind<Node>::other node_alloc_t;
    typedef typename Allocator::template rebind<char>::other char_alloc_t;

    node_alloc_t m_node_allocator;
    char_alloc_t m_char_allocator;

    size_t m_node_count;
};

template<typename Node, typename Allocator>
const typename simple_node_store<Node, Allocator>::pointer_t
               simple_node_store<Node, Allocator>::null = 0;

} // namespace detail
} // namespace container
} // namespace utxx

#endif // _UTXX_CONTAINER_DETAIL_SIMPLE_NODE_STORE_HPP_
