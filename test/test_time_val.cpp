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
#include <utxx/verbosity.hpp>
#include <utxx/time_val.hpp>
#include <utxx/time.hpp>
#include <utxx/print.hpp>
//#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <time.h>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_time_val )
{
    time_val now  = now_utc();
    time_val now1 = time_val::universal_time();

    while(now == time_val::universal_time());

    BOOST_REQUIRE((now1 - now).milliseconds() <= 1);

    #if __cplusplus >= 201103L
    time_val now2(std::move(now1));
    BOOST_REQUIRE_EQUAL(now1.microseconds(), now2.microseconds());
    #endif

    time_val rel(1, 1004003);
    BOOST_REQUIRE_EQUAL(2,          rel.sec());
    BOOST_REQUIRE_EQUAL(4003,       rel.usec());
    BOOST_REQUIRE_EQUAL(4,          rel.msec());
    BOOST_REQUIRE_EQUAL(4003000,    rel.nanosec());
    BOOST_REQUIRE_EQUAL(2004,       rel.milliseconds());
    BOOST_REQUIRE_EQUAL(2004003u,   rel.microseconds());
    BOOST_REQUIRE_EQUAL(2.004003,   rel.seconds());

    struct timespec ts = rel.timespec();
    BOOST_REQUIRE_EQUAL(2,          ts.tv_sec);
    BOOST_REQUIRE_EQUAL(4003000,    ts.tv_nsec);

    time_val add = now + rel;

    BOOST_REQUIRE_EQUAL(now.microseconds() + 2004003, add.microseconds());
    BOOST_REQUIRE_EQUAL(4003, rel.usec());

    {
        time_t t;     time(&t);
        struct tm tm; localtime_r(&t, &tm);
        time_val gmt = time_val::universal_time(2014, 7, 10, 0,0,0, 0);
        time_val loc = time_val::local_time    (2014, 7, 10, 0,0,0, 0);
        BOOST_MESSAGE("TZ  offset: " << tm.tm_gmtoff);
        BOOST_MESSAGE("GMT   time: " << gmt.sec());
        BOOST_MESSAGE("Local time: " << loc.sec());
        BOOST_REQUIRE_EQUAL(gmt.sec() - loc.sec(), tm.tm_gmtoff);
    }

    {
        time_val t(10.123);
        BOOST_CHECK_EQUAL(10,     t.sec());
        BOOST_CHECK_EQUAL(123000, t.usec());

        t += 1.1;
        BOOST_CHECK_EQUAL(11,     t.sec());
        BOOST_CHECK_EQUAL(223000, t.usec());

        t.add(1.9);

        BOOST_CHECK_EQUAL(13,     t.sec());
        BOOST_CHECK_EQUAL(123000, t.usec());
    }
    {
        time_val t(0.999999);
        BOOST_CHECK_EQUAL(0,      t.sec());
        BOOST_CHECK_EQUAL(999999, t.usec());
    }
    {
        unsigned Y,M,D, h,m,s;
        BOOST_CHECK_EQUAL(0, to_gregorian_days(1970, 1, 1));
        std::tie(Y,M,D) = from_gregorian_days(0);
        BOOST_CHECK_EQUAL(1970u, Y);
        BOOST_CHECK_EQUAL(1u,    M);
        BOOST_CHECK_EQUAL(1u,    D);

        BOOST_CHECK_EQUAL(86400, mktime_utc(1970, 1, 2));

        time_val t = time_val::universal_time(2014, 7, 10, 1,2,3, 0);
        std::tie(Y,M,D) = t.to_ymd();
        BOOST_CHECK_EQUAL(2014u, Y);
        BOOST_CHECK_EQUAL(7u,    M);
        BOOST_CHECK_EQUAL(10u,   D);

        t = time_val(2014, 7, 10);
        BOOST_CHECK_EQUAL(1404950400, t.sec());

        t = time_val(2014, 7, 10, 1,2,3, 0);
        BOOST_CHECK_EQUAL(1404954123, t.sec());
        BOOST_CHECK_EQUAL(0,          t.usec());

        std::tie(Y,M,D, h,m,s) = t.to_ymdhms();
        BOOST_CHECK_EQUAL(2014u, Y);
        BOOST_CHECK_EQUAL(7u,    M);
        BOOST_CHECK_EQUAL(10u,   D);
        BOOST_CHECK_EQUAL(1u,    h);
        BOOST_CHECK_EQUAL(2u,    m);
        BOOST_CHECK_EQUAL(3u,    s);
    }
}


BOOST_AUTO_TEST_CASE( test_time_val_ptime )
{
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    // Note that pt below is in local time!
    ptime pt = to_ptime(utxx::time_val::universal_time(2000, 1, 2, 3, 4, 5, 1000));
    BOOST_REQUIRE_EQUAL("2000-Jan-02 03:04:05.001000", to_simple_string(pt));
    ptime tt = ptime(date(2000,1,2))
                + hours(3) + minutes(4) + seconds(5) + microseconds(1000);
    BOOST_REQUIRE_EQUAL(pt, tt);
}

