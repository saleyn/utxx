// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief simple strie node storage facility
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

#ifndef _UTXX_SIMPLE_NODE_STORE_HPP_
#define _UTXX_SIMPLE_NODE_STORE_HPP_

#include <memory>
#include <cstddef>

namespace utxx {

// simple strie node store implementation
// it works like allocator adapter, does not offer any benefit
// over underlying allocator, used for testing or simple applications
//
template <typename T = void, typename Allocator = std::allocator<char> >
class simple_node_store {
public:
    template<typename U>
    struct rebind { typedef simple_node_store<U, Allocator> other; };

    typedef typename Allocator::template rebind<T>::other alloc_t;
    typedef T* pointer_t;

    static const pointer_t null;

    simple_node_store() : cnt(0) {}

    pointer_t allocate() {
        ++cnt;
        pointer_t l_ptr = m_allocator.allocate(1);
        return new (l_ptr) T;
    }

    void deallocate(pointer_t a_ptr) {
        a_ptr->~T();
        m_allocator.deallocate(a_ptr, 1);
        --cnt;
    }

    T *native_pointer(pointer_t a_ptr) const {
        return a_ptr;
    }

    alloc_t m_allocator;
    size_t cnt;
};

template<typename T, typename A>
const typename simple_node_store<T, A>::pointer_t
               simple_node_store<T, A>::null = 0;

} // namespace utxx

#endif // _UTXX_SIMPLE_NODE_STORE_HPP_
