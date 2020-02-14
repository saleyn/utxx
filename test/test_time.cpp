//----------------------------------------------------------------------------
/// \file  test_time_val.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the time_val.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-10-15
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included  in different open-source projects

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

// #define BOOST_TEST_MODULE time_val_test

#include <boost/test/unit_test.hpp>
#include <utxx/test_helper.hpp>
#include <utxx/string.hpp>
#include <utxx/time.hpp>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_parse_time )
{
    auto n = parse_time_to_seconds("");
    BOOST_CHECK_EQUAL(-1, n);
    n = parse_time_to_seconds("2:03");
    BOOST_CHECK_EQUAL(-1, n);
    n = parse_time_to_seconds("02:03");
    BOOST_CHECK_EQUAL(2*3600+3*60, n);
    n = parse_time_to_seconds("02:03:04");
    BOOST_CHECK_EQUAL(2*3600+3*60+4, n);
    n = parse_time_to_seconds("02:03am");
    BOOST_CHECK_EQUAL(2*3600+3*60, n);
    n = parse_time_to_seconds("02:03pm");
    BOOST_CHECK_EQUAL(14*3600+3*60, n);
    n = parse_time_to_seconds("02:03x");
    BOOST_CHECK_EQUAL(-1, n);
}

BOOST_AUTO_TEST_CASE( test_parse_dow )
{
    const char* dow[] = {"Sun", "MON", "tUE", "wed", "THu", "fri", "sat"};

    for (unsigned i=0; i < length(dow); ++i) {
        auto p = dow[i];
        int  n = parse_dow_ref(p, -1);
        BOOST_CHECK_EQUAL(i, n);
        BOOST_CHECK_EQUAL(3, p-dow[i]);
    }

    time_t now    = time(nullptr);
    struct tm* tm = localtime(&now);

    const char* dowt[] = {"tOD", "Today"};
    auto p = dowt[0];
    int  n = parse_dow_ref(p, tm->tm_wday, false);
    BOOST_CHECK_EQUAL(n, tm->tm_wday);
    BOOST_CHECK_EQUAL(3, p-dowt[0]);

    p = dowt[1];
    n = parse_dow_ref(p, tm->tm_wday, false);
    BOOST_CHECK_EQUAL(n, tm->tm_wday);
    BOOST_CHECK_EQUAL(5, p-dowt[1]);

    BOOST_CHECK_EQUAL(-1, parse_dow("ttt"));
}

