//----------------------------------------------------------------------------
/// \file   atomic_value.hpp
/// \author Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
//----------------------------------------------------------------------------
/// \brief Atomic value operations.
//----------------------------------------------------------------------------
// Copyright (c) 2012 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
// Created: 2012-12-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (c) 2012 Omnibius, LLC
Author: Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>

Use, modification and distribution are subject to the Boost Software
License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
at http://www.boost.org/LICENSE_1_0.txt)

***** END LICENSE BLOCK *****
*/

#ifndef _UTXX_ATOMIC_ATOMIC_VALUE_HPP_
#define _UTXX_ATOMIC_ATOMIC_VALUE_HPP_

namespace utxx {
namespace atomic {

template<typename T>
class atomic_value {
    volatile T val;

public:
    atomic_value(T a_val = 0) : val(a_val) {} 
    T get() const { return val; }

    bool cas(T a_old, T a_new) {
        return __sync_bool_compare_and_swap(&val, a_old, a_new);
    }
    // prefix ++: ++val, returning new val
    T operator++() {
        return __sync_add_and_fetch(&val, 1);
    }
    // postfix ++: val++, returning old val
    T operator++(int) {
        return __sync_fetch_and_add(&val, 1);
    }
    void operator +=(T a_add) {
        __sync_fetch_and_add(&val, a_add);
    }
    void operator -=(T a_sub) {
        __sync_fetch_and_sub(&val, a_sub);
    }
    T operator&(T a_bits) {
        return val & a_bits;
    }
    void bclear(T a_bits) {
        __sync_fetch_and_and(&val, ~a_bits);
    }
    void bset(T a_bits) {
        __sync_fetch_and_or(&val, a_bits);
    }
};

} // namespace atomic
} // namespace utxx

#endif // _UTXX_ATOMIC_ATOMIC_VALUE_HPP_
