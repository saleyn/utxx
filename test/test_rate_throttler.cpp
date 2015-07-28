//----------------------------------------------------------------------------
/// \file   test_rate_throttler.hpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the rate_throttler.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-01-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects

Copyright (C) 2011 Serge Aleynikov <saleyn@gmail.com>

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

// #define BOOST_TEST_MODULE time_val_test

#include <utxx/test_helper.hpp>
#include <utxx/rate_throttler.hpp>
#include <utxx/timestamp.hpp>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_rate_throttler_time_spacing )
{
    auto now = time_val(2015, 6, 1, 12, 0, 0, 0);
    time_spacing_throttle thr(10, 1000, now);    // Throttle 10 samples / second

    BOOST_CHECK_EQUAL(100000, thr.step());
    BOOST_CHECK_EQUAL(10u,    thr.available(now));

    int n   = thr.add(1,  now);
    BOOST_CHECK_EQUAL(1,  n);
    BOOST_CHECK_EQUAL(9u, thr.available(now));

    n       = thr.add(1,  now);
    BOOST_CHECK_EQUAL(1,  n);
    BOOST_CHECK_EQUAL(8u, thr.available(now));
    auto next_time = time_val(2015,6,1, 12,0,0, 200000);
    BOOST_CHECK(next_time == thr.next_time());

    now.add_msec(100);
    BOOST_CHECK_EQUAL(9u, thr.available(now));

    n       = thr.add(5,  now);
    BOOST_CHECK_EQUAL(5,  n);
    BOOST_CHECK_EQUAL(4u, thr.available(now));
    next_time = time_val(2015,6,1, 12,0,0, 700000);
//     BOOST_MESSAGE("Next = " << timestamp::to_string(thr.next_time(), utxx::TIME_WITH_USEC, true));
//     BOOST_MESSAGE("Now  = " << timestamp::to_string(now, utxx::TIME_WITH_USEC, true));
    BOOST_CHECK(next_time == thr.next_time());

    n       = thr.add(5,  now);
    BOOST_CHECK_EQUAL(4,  n);
    BOOST_CHECK_EQUAL(0u, thr.available(now));
    next_time = time_val(2015,6,1, 12,0,1, 100000);
    BOOST_CHECK(next_time == thr.next_time());
}

BOOST_AUTO_TEST_CASE( test_rate_throttler_basic )
{
    basic_rate_throttler<16> l_throttler;
    time_val tv;

    l_throttler.init(3);
    int i;
    for (i=0, tv.usec(500000); i < 8; i++, tv.add_usec(500000)) {
        if (tv.usec() > 500000) {
            tv.usec(tv.usec() % 500000);
            tv.add_sec(1);
        }
        l_throttler.add(tv, i+1);
    }

    BOOST_REQUIRE_EQUAL(33, l_throttler.running_sum());

    tv.add_sec(2);
    l_throttler.add(tv, ++i); // i=9
    BOOST_REQUIRE_EQUAL(17, l_throttler.running_sum());

    tv.add_sec(3);
    l_throttler.add(tv, ++i); // i=10
    BOOST_REQUIRE_EQUAL(10, l_throttler.running_sum());

    tv.add_sec(9);
    l_throttler.add(tv, ++i); // i=11
    BOOST_REQUIRE_EQUAL(11, l_throttler.running_sum());

    tv.add_sec(2);
    l_throttler.add(tv, ++i); // i=12
    BOOST_REQUIRE_EQUAL(23, l_throttler.running_sum());

    tv.add_sec(2);
    l_throttler.add(tv, ++i); // i=13
    BOOST_REQUIRE_EQUAL(25, l_throttler.running_sum());

    tv.add_sec(1);
    l_throttler.add(tv, ++i); // i=14
    BOOST_REQUIRE_EQUAL(27, l_throttler.running_sum());

    tv.add_sec(2);
    l_throttler.add(tv, ++i); // i=15
    BOOST_REQUIRE_EQUAL(29, l_throttler.running_sum());

    BOOST_REQUIRE_EQUAL(29.0 / 3, l_throttler.running_avg());
}