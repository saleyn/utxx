// ex: ts=4 sw=4 ft=cpp et indentexpr=
/**
 * \file
 * \brief flat memory read-only data storage facility
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

#ifndef _UTXX_FLAT_DATA_STORE_HPP_
#define _UTXX_FLAT_DATA_STORE_HPP_

#include <stdexcept>

namespace utxx {

// flat memory region strie data store implementation
//
template <typename T = void, typename OffsetType = int>
class flat_data_store {
public:
    template<typename U>
    struct rebind { typedef flat_data_store<U, OffsetType> other; };

    typedef OffsetType pointer_t;

    static const pointer_t null;

    flat_data_store(const void *a_start, pointer_t a_size)
            : m_start(a_start), m_size(a_size) {
        m_ptr_max = a_size - sizeof(T);
    }

    T *native_pointer(pointer_t a_ptr) const {
        if (a_ptr > m_ptr_max)
            throw std::invalid_argument("flat_data_store: bad offset");
        return (T*)((char *)m_start + a_ptr);
    }

    const void *m_start;
    pointer_t m_size;
    pointer_t m_ptr_max;
};

template<typename T, typename A>
const typename flat_data_store<T, A>::pointer_t
               flat_data_store<T, A>::null = 0;

} // namespace utxx

#endif // _UTXX_FLAT_DATA_STORE_HPP_
