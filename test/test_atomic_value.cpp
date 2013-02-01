//----------------------------------------------------------------------------
/// \file   test_atomic_value.cpp
/// \author Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
//----------------------------------------------------------------------------
/// \brief Atomic value operations tests.
//----------------------------------------------------------------------------
// Copyright (c) 2012 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
// Created: 2013-02-01
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

#include <boost/test/unit_test.hpp>
#include <utxx/atomic_value.hpp>

using namespace utxx::atomic;

BOOST_AUTO_TEST_SUITE( test_atomic_value )

BOOST_AUTO_TEST_CASE( single_thread_ops )
{
    atomic_value<int> a = 10;
    BOOST_REQUIRE_EQUAL(a.get(), 10);
    BOOST_REQUIRE_EQUAL(++a, 11);
    BOOST_REQUIRE_EQUAL(a++, 11);
    BOOST_REQUIRE_EQUAL(a.get(), 12);
    a += 10;
    BOOST_REQUIRE_EQUAL(a.get(), 22);
    a -= 15;
    BOOST_REQUIRE_EQUAL(a.get(), 7);
    a.cas(6, 12);
    BOOST_REQUIRE_EQUAL(a.get(), 7);
    a.cas(7, 12);
    BOOST_REQUIRE_EQUAL(a.get(), 12);
    a.bclear(7);
    BOOST_REQUIRE_EQUAL(a.get(), 8);
    a.bset(5);
    BOOST_REQUIRE_EQUAL(a.get(), 13);
    BOOST_REQUIRE_EQUAL(a & 7, 5);
}

BOOST_AUTO_TEST_SUITE_END()
