//----------------------------------------------------------------------------
/// \file  test_throttler.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the throttler.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
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

#include <boost/test/unit_test.hpp>
#include <util/rate_throttler.hpp>

using namespace util;

BOOST_AUTO_TEST_CASE( test_basic_rate_throttler )
{
    basic_rate_throttler<16> l_throttler;
    struct timeval tv = {0,0};

    l_throttler.init(3);
    int i;
    for (i=0, tv.tv_usec=500000; i < 8; i++, tv.tv_usec += 500000) {
        if (tv.tv_usec > 500000) {
            tv.tv_usec = tv.tv_usec % 500000;
            tv.tv_sec++;
        }
        l_throttler.add(tv, i+1);
    }

    BOOST_REQUIRE_EQUAL(33, l_throttler.running_sum());

    tv.tv_sec += 2;
    l_throttler.add(tv, ++i); // i=9
    BOOST_REQUIRE_EQUAL(17, l_throttler.running_sum());

    tv.tv_sec += 3;
    l_throttler.add(tv, ++i); // i=10
    BOOST_REQUIRE_EQUAL(10, l_throttler.running_sum());

    tv.tv_sec += 9;
    l_throttler.add(tv, ++i); // i=11
    BOOST_REQUIRE_EQUAL(11, l_throttler.running_sum());

    tv.tv_sec += 2;
    l_throttler.add(tv, ++i); // i=12
    BOOST_REQUIRE_EQUAL(23, l_throttler.running_sum());

    tv.tv_sec += 2;
    l_throttler.add(tv, ++i); // i=13
    BOOST_REQUIRE_EQUAL(25, l_throttler.running_sum());

    tv.tv_sec += 1;
    l_throttler.add(tv, ++i); // i=14
    BOOST_REQUIRE_EQUAL(27, l_throttler.running_sum());

    tv.tv_sec += 2;
    l_throttler.add(tv, ++i); // i=15
    BOOST_REQUIRE_EQUAL(29, l_throttler.running_sum());

    BOOST_REQUIRE_EQUAL(29.0 / 3, l_throttler.running_avg());
}
