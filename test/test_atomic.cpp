//----------------------------------------------------------------------------
/// \file  test_atomic.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating atomic.hpp functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-16
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>

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
//#define BOOST_TEST_MODULE atomic_test 

#include <boost/test/unit_test.hpp>
#include <util/atomic.hpp>
#include <stdio.h>

#define DO_REQUIRE_EQUAL(A, B, C) \
    { bool r = A == B; if (!r) { printf("Testing %s failed: %s != %s\n", C, #A, #B); \
      BOOST_REQUIRE(false); } }

using namespace util;

template <typename T>
void do_test_atomic_cas(const char* str_type) {
    volatile T x = 10, y = x, z = x;
    x = 20;
    DO_REQUIRE_EQUAL(true, atomic::cas(&z, y, x), str_type);
    DO_REQUIRE_EQUAL(20, x, str_type);
    DO_REQUIRE_EQUAL(10, y, str_type);
    DO_REQUIRE_EQUAL(20, z, str_type);
    y = 10;
    DO_REQUIRE_EQUAL(false, atomic::cas(&z, y, x), str_type);
    DO_REQUIRE_EQUAL(20, z, str_type);
    DO_REQUIRE_EQUAL(10, y, str_type);
    x = 30;
    y = 20;
    DO_REQUIRE_EQUAL(20, y, str_type);
    DO_REQUIRE_EQUAL(true, atomic::cas(&z, y, x), str_type);
    DO_REQUIRE_EQUAL(30, z, str_type);
}

namespace {
    template<typename T>
    struct TT {
        T v1;
        T v2;
        void set(T _v1, T _v2)                  { v1=_v1; v2=_v2; }
        void set(T _v1, T _v2) volatile         { v1=_v1; v2=_v2; }
        bool operator== (const TT& rhs) const   { return v1==rhs.v1 && v2==rhs.v2; }
        void operator=  (const TT& rhs)         { return v1=rhs.v1 && v2=rhs.v2; }
        TT(T x, T y) : v1(x), v2(y)             {}
        TT(const TT& x) : v1(x.v1), v2(x.v2)    {}
    };

    template<typename T>
    bool operator== (const TT<T>& lhs, const TT<T>& rhs) {
        return lhs.v1 == rhs.v1 && lhs.v2 == rhs.v2;
    }
}

template <typename T>
void do_test_atomic_dcas(const char* str_type) {
    typedef TT<T> TT;

    BOOST_STATIC_ASSERT(sizeof(TT) == 2*sizeof(T));

    TT x(10, 5), y(x);
    volatile TT z(x);
    x.set(20, 15);
    DO_REQUIRE_EQUAL(true, atomic::dcas(&z, y, x), str_type);
    DO_REQUIRE_EQUAL(TT(20,15), x, str_type);
    DO_REQUIRE_EQUAL(TT(10,5),  y, str_type);
    DO_REQUIRE_EQUAL(TT(20,15), const_cast<const TT&>(z), str_type);
    y.set(10, 2);
    DO_REQUIRE_EQUAL(false, atomic::dcas(&z, y, x), str_type);
    DO_REQUIRE_EQUAL(TT(20,15), const_cast<const TT&>(z), str_type);
    DO_REQUIRE_EQUAL(TT(10,2),  const_cast<const TT&>(y), str_type);
    x.set(30,16);
    y.set(20,15);
    DO_REQUIRE_EQUAL(true, atomic::dcas(&z, y, x), str_type);
    DO_REQUIRE_EQUAL(TT(30,16), const_cast<const TT&>(z), str_type);
}

template <typename T>
void test_atomic_cmpxchg(const char* str_type) {
    volatile T x = 10;
    T y = x, z = x;
    x = 20;
    DO_REQUIRE_EQUAL(10, atomic::cmpxchg(&z, y, x), str_type);
    DO_REQUIRE_EQUAL(20, z, str_type);
    x = 30;
    DO_REQUIRE_EQUAL(10, y, str_type);
    DO_REQUIRE_EQUAL(20, atomic::cmpxchg(&z, y, x), str_type);
    DO_REQUIRE_EQUAL(20, z, str_type);
}

BOOST_AUTO_TEST_CASE( test_atomic_cas )
{
    do_test_atomic_cas<int>("cas_int");
    do_test_atomic_cas<long>("cas_long");
    do_test_atomic_cas<long long>("cas_long_long");
}

BOOST_AUTO_TEST_CASE( test_atomic_dcas )
{
    do_test_atomic_dcas<int>("dcas_int");
    do_test_atomic_dcas<long>("dcas_long");
    do_test_atomic_dcas<long long>("dcas_long_long");
}

BOOST_AUTO_TEST_CASE( test_cmpxchg )
{
    test_atomic_cmpxchg<int>("cmpxchg int");
    test_atomic_cmpxchg<long>("cmpxchg long");
    test_atomic_cmpxchg<long long>("cmpxchg long long");
}

BOOST_AUTO_TEST_CASE( test_atomic_add )
{
    volatile long n = 1;
    BOOST_REQUIRE_EQUAL(1, atomic::add(&n,  1));
    BOOST_REQUIRE_EQUAL(2, n);
    BOOST_REQUIRE_EQUAL(2, atomic::add(&n, -1));
    BOOST_REQUIRE_EQUAL(1, n);
    volatile int m = 1;
    BOOST_REQUIRE_EQUAL(1, atomic::add(&m,  1));
    BOOST_REQUIRE_EQUAL(2, m);
    BOOST_REQUIRE_EQUAL(2, atomic::add(&m, -1));
    BOOST_REQUIRE_EQUAL(1, m);

    atomic::inc(&n);
    BOOST_REQUIRE_EQUAL(2, n);
    atomic::dec(&n);
    BOOST_REQUIRE_EQUAL(1, n);
}

BOOST_AUTO_TEST_CASE( test_atomic_set_bit )
{
    volatile unsigned long n = 0;
    atomic::set_bit(0, &n);
    BOOST_REQUIRE_EQUAL(1llu << 0,  n);
    n = 0;
    atomic::set_bit(5, &n);
    BOOST_REQUIRE_EQUAL(1llu << 5,  n);
    n = 0;
    atomic::set_bit(32, &n);
    BOOST_REQUIRE_EQUAL(1llu << 32, n);
    n = 0;
    atomic::set_bit(63, &n);
    BOOST_REQUIRE_EQUAL(1llu << 63, n);
}

BOOST_AUTO_TEST_CASE( test_atomic_clear_bit )
{
    volatile unsigned long n = 1llu << 0;
    atomic::clear_bit(0, &n);
    BOOST_REQUIRE_EQUAL(0u,  n);
    n = 1llu << 5;
    atomic::clear_bit(5, &n);
    BOOST_REQUIRE_EQUAL(0u,  n);
    n = 1llu << 32;
    atomic::clear_bit(32, &n);
    BOOST_REQUIRE_EQUAL(0u,  n);
    n = 1llu << 63;
    atomic::clear_bit(63, &n);
    BOOST_REQUIRE_EQUAL(0u,  n);
    n = 1llu << 63 | 1llu << 16;
    atomic::clear_bit(63u, &n);
    BOOST_REQUIRE_EQUAL(1llu << 16,  n);
}

BOOST_AUTO_TEST_CASE( test_atomic_change_bit )
{
    volatile unsigned long n = 1llu << 0;
    atomic::change_bit(0, &n);
    BOOST_REQUIRE_EQUAL(0u,  n);
    n = 1llu << 5;
    atomic::change_bit(5, &n);
    BOOST_REQUIRE_EQUAL(0u,  n);
    n = 1llu << 32;
    atomic::change_bit(32, &n);
    BOOST_REQUIRE_EQUAL(0u,  n);
    n = 1llu << 63;
    atomic::change_bit(63, &n);
    BOOST_REQUIRE_EQUAL(0u,  n);
    n = 1llu << 63 | 1llu << 16;
    atomic::change_bit(63, &n);
    BOOST_REQUIRE_EQUAL(1llu << 16,  n);
    atomic::change_bit(63, &n);
    atomic::change_bit(16, &n);
    BOOST_REQUIRE_EQUAL(1llu << 63,  n);
}

