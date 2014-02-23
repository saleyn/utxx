/* ex: ts=4 sw=4 ft=cpp et indentexpr=
*/

/**
 * \file
 * \brief object instance id
 * \author Dmitriy Kargapolov \<dmitriy dot kargapolov at gmail dot com\>
 * \version 1.0
 * \since 08 August 2012
 */

/*
 * Copyright (C) 2011 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef _UTXX_OBJECT_ID_HPP_
#define _UTXX_OBJECT_ID_HPP_

#include <utxx/atomic_value.hpp>

namespace utxx {

template<typename T, typename I = unsigned>
class object_id {
    static atomic::atomic_value<I> m_cnt;
    I m_id;

protected:
    object_id() : m_id(++m_cnt) {}

public:
    const I& oid() const { return m_id; }
};

template<typename T, typename I>
atomic::atomic_value<I> object_id<T, I>::m_cnt;

} // namespace utxx

#endif // _UTXX_OBJECT_ID_HPP_
