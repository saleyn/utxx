// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief flat memory read-only data storage facility
 *
 * \author Dmitriy Kargapolov
 * \since 07 May 2013
 *
 */

/*
 * Copyright (C) 2013 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_CONTAINER_DETAIL_FLAT_DATA_STORE_HPP_
#define _UTXX_CONTAINER_DETAIL_FLAT_DATA_STORE_HPP_

#include <stdexcept>

namespace utxx {
namespace container {
namespace detail {

// flat memory region abstract strie node and data store
//
template <typename Node = void, typename OffsetType = unsigned>
class flat_data_store {
public:
    // rebind to other node type
    template<typename U>
    struct rebind { typedef flat_data_store<U, OffsetType> other; };

    // abstract data pointer
    typedef OffsetType pointer_t;

    // this store does not provide allocate/deallocate methods
    static const bool dynamic = false;

    // null pointer constant
    static const pointer_t null;

    // construct from memory region
    flat_data_store(const void *a_start, pointer_t a_size)
            : m_start(a_start), m_size(a_size) {
    }

    // convert abstract pointer to native pointer
    template<typename T> T *native_pointer(pointer_t a_ptr) const {
        if (a_ptr > m_size - sizeof(T))
            throw std::invalid_argument("flat_data_store: bad offset");
        return (T*)((char *)m_start + a_ptr);
    }

private:
    const void *m_start;
    pointer_t m_size;
};

template<typename T, typename A>
const typename flat_data_store<T, A>::pointer_t
               flat_data_store<T, A>::null = 0;

} // namespace detail
} // namespace container
} // namespace utxx

#endif // _UTXX_CONTAINER_DETAIL_FLAT_DATA_STORE_HPP_
