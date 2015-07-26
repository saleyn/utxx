//----------------------------------------------------------------------------
/// \file   rate_throttler.hpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the rate_throttler.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2012-07-26
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

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_rate_throttler )
{
    auto now = time_val(2015, 6, 1, 12, 0, 0, 0);
    time_spacing_throttle thr(10, 1000, now);    // Throttle 10 samples / second

    BOOST_CHECK_EQUAL(100000, thr.step());
    BOOST_CHECK_EQUAL(10,     thr.available());

    auto n  = thr.add(1, now);
    BOOST_CHECK_EQUAL(1, n);
    BOOST_CHECK_EQUAL(9, thr.available());

    n       = thr.add(1, now);
    BOOST_CHECK_EQUAL(1, n);
    BOOST_CHECK_EQUAL(8, thr.available());
    auto next_time = time_val(2015,6,1, 12,0,0, 200000);
    BOOST_CHECK_EQUAL(next_time, thr.next_time());

    now.add_msec(100);
    BOOST_CHECK_EQUAL(9, thr.available(now));

    n       = thr.add(5, now);
    BOOST_CHECK_EQUAL(5, n);
    BOOST_CHECK_EQUAL(4, thr.available(now));
    next_time = time_val(2015,6,1, 12,0,0, 600000);
    BOOST_CHECK_EQUAL(next_time, thr.next_time());

    n       = thr.add(5, now);
    BOOST_CHECK_EQUAL(4, n);
    BOOST_CHECK_EQUAL(0, thr.available(now));
    next_time = time_val(2015,6,1, 12,0,1, 100000);
    BOOST_CHECK_EQUAL(next_time, thr.next_time());
}
