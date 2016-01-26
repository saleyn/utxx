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
#include <utxx/verbosity.hpp>
#include <utxx/time_val.hpp>
#include <utxx/timestamp.hpp>
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
    time_val now0;

    BOOST_REQUIRE(!now0);

    time_val now  = now_utc();
    time_val now1 = time_val::universal_time();

    BOOST_REQUIRE(now);

    while (now.microseconds() == time_val::universal_time().microseconds());

    BOOST_CHECK((now1 - now).milliseconds() <= 1);

    time_val now12 = now;
    time_val now13(now);

    BOOST_CHECK_EQUAL(now.microseconds(), now12.microseconds());
    BOOST_CHECK_EQUAL(now.microseconds(), now13.microseconds());

    #if __cplusplus >= 201103L
    time_val now2(std::move(now1));
    BOOST_REQUIRE_EQUAL(now1.microseconds(), now2.microseconds());
    #endif

    time_val rel(1, 1004003);
    BOOST_REQUIRE_EQUAL(2,          rel.sec());
    BOOST_REQUIRE_EQUAL(4003,       rel.usec());
    BOOST_REQUIRE_EQUAL(4,          rel.msec());
    BOOST_REQUIRE_EQUAL(4003000,    rel.nsec());
    BOOST_REQUIRE_EQUAL(2004,       rel.milliseconds());
    BOOST_REQUIRE_EQUAL(2004003u,   rel.microseconds());
    BOOST_REQUIRE_EQUAL(2.004003,   rel.seconds());
    BOOST_REQUIRE_EQUAL(2004003000L,rel.nanoseconds());

    struct timespec ts = rel.timespec();
    BOOST_REQUIRE_EQUAL(2,          ts.tv_sec);
    BOOST_REQUIRE_EQUAL(4003000,    ts.tv_nsec);

    time_val add = now + rel;

    BOOST_REQUIRE_EQUAL(now.microseconds() + 2004003, add.microseconds());
    BOOST_REQUIRE_EQUAL(4003, rel.usec());

    {
        time_val gmt = time_val::universal_time(2014, 7, 10, 0,0,0, 0);
        time_t     t = gmt.sec();
        struct tm tm;  localtime_r(&t, &tm);
        struct tm tm0; gmtime_r(&t, &tm0);
        time_val loc = time_val::local_time    (2014, 7, 10, 0,0,0, 0);
        BOOST_TEST_MESSAGE("TZ  offset: " << tm.tm_gmtoff);
        BOOST_TEST_MESSAGE("GMT   time: " << gmt.sec());
        BOOST_TEST_MESSAGE("Local time: " << loc.sec());
        BOOST_REQUIRE_EQUAL(gmt.sec() - loc.sec(), tm.tm_gmtoff);
    }

    {
        time_val ts(secs(10));
        BOOST_CHECK_EQUAL(10,     ts.sec());
        BOOST_CHECK_EQUAL(0,      ts.usec());

        ts.set(15);
        BOOST_CHECK_EQUAL(15,     ts.sec());
        BOOST_CHECK_EQUAL(0,      ts.usec());

        time_val t(secs(10.123));
        BOOST_CHECK_EQUAL(10,     t.sec());
        BOOST_CHECK_EQUAL(123000, t.usec());

        t += 1.1;
        BOOST_CHECK_EQUAL(11,     t.sec());
        BOOST_CHECK_EQUAL(223000, t.usec());

        t.add(1.9);

        BOOST_CHECK_EQUAL(13,     t.sec());
        BOOST_CHECK_EQUAL(123000, t.usec());

        t += usecs(100);
        BOOST_CHECK_EQUAL(13123100, t.microseconds());
        t += secs(1);
        BOOST_CHECK_EQUAL(14123100,t.microseconds());
        t -= secs(1);
        BOOST_CHECK_EQUAL(13123100, t.microseconds());
        t -= usecs(100);
        BOOST_CHECK_EQUAL(13123000, t.microseconds());
        t += msecs(15);
        BOOST_CHECK_EQUAL(13138000, t.microseconds());
        t -= msecs(15);
        BOOST_CHECK_EQUAL(13123000, t.microseconds());

        t = usecs(1123000);
        BOOST_CHECK_EQUAL(1123100, (t + usecs(100)).microseconds());
        BOOST_CHECK_EQUAL(2123000, (t + secs(1)).microseconds());
        BOOST_CHECK_EQUAL(1122500, (t - usecs(500)).microseconds());
        BOOST_CHECK_EQUAL(123000,  (t - secs(1)).microseconds());
    }
    {
        time_val t(secs(0.999999));
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
    {
        time_val tv(10, 5);
        tv.add_sec(50);
        BOOST_CHECK_EQUAL(time_val(60,5), tv);
        tv.add_sec(-30);
        BOOST_CHECK_EQUAL(time_val(30,5), tv);

        tv.add_msec(1);
        BOOST_CHECK_EQUAL(time_val(30,1005), tv);

        BOOST_CHECK_EQUAL(time_val(0,1005).add_sec(30), tv);
        BOOST_CHECK_EQUAL(time_val(30,5).add_msec(1),   tv);

        bool ok = tv.add_sec(10) == time_val(40, 1005);
        BOOST_CHECK(ok);
        BOOST_CHECK_EQUAL(tv.sec(), 40);

        const time_val& now = tv;
        BOOST_CHECK_EQUAL(time_val(45, 1005), now.add_sec(5));
        BOOST_CHECK_EQUAL(tv.sec(), 40);
        BOOST_CHECK_EQUAL(time_val(45, 3005), now.add(5, 2000));

        auto tv1 = time_val(abs_time(1, 100000));
        BOOST_CHECK_EQUAL(1,      tv1.sec());
        BOOST_CHECK_EQUAL(100000, tv1.usec());
        time_val tv0, tv2;
        int count = 0;
        do {
            tv0 = now_utc();
            tv2 = time_val(rel_time(1, 100000));
        } while (tv2.milliseconds() != tv0.milliseconds() && count++ < 10);

        auto tv3 = tv0 + tv1;
        BOOST_CHECK_EQUAL(tv2.sec(),  tv3.sec());
        BOOST_CHECK_EQUAL(tv2.msec(), tv3.msec());
    }

    {
        BOOST_REQUIRE_EQUAL("00123", detail::itoar(123, 5));
        BOOST_REQUIRE_EQUAL("3",     detail::itoar(123, 1));
        BOOST_REQUIRE_EQUAL("",      detail::itoar(123, 0));

        auto t = utxx::time_val::universal_time(2000, 1, 2, 3, 4, 5, 1000);
        BOOST_REQUIRE_EQUAL("", t.to_string(utxx::NO_TIMESTAMP));
        BOOST_REQUIRE_EQUAL("20000102-03:04:05.001000", t.to_string(utxx::DATE_TIME_WITH_USEC));
        BOOST_REQUIRE_EQUAL("20000102-03:04:05",        t.to_string(utxx::DATE_TIME));
        BOOST_REQUIRE_EQUAL("2000-01-02-03:04:05",      t.to_string(utxx::DATE_TIME, '-'));
        BOOST_REQUIRE_EQUAL("20000102-030405",          t.to_string(utxx::DATE_TIME, '\0', '\0'));
        BOOST_REQUIRE_EQUAL("20000102-03:04:05.001",    t.to_string(utxx::DATE_TIME_WITH_MSEC));
        BOOST_REQUIRE_EQUAL("03:04:05",                 t.to_string(utxx::TIME));
        BOOST_REQUIRE_EQUAL("03:04:05.001",             t.to_string(utxx::TIME_WITH_MSEC));
        BOOST_REQUIRE_EQUAL("03:04:05.001000",          t.to_string(utxx::TIME_WITH_USEC));
    }

    {
        time_val tv1; tv1.nanoseconds(1453768119042798821);
        time_val tv2; tv2.nanoseconds(1453768061796270822);
        BOOST_CHECK(tv1 >= tv2);
        BOOST_CHECK(tv1.nanoseconds() >= tv2.nanoseconds());
        BOOST_CHECK(tv1 > tv2);
        BOOST_CHECK(tv1.nanoseconds() >  tv2.nanoseconds());
    }
}


BOOST_AUTO_TEST_CASE( test_time_val_perf )
{
    static const int ITERATIONS = env("ITERATIONS", 10000000);
    double elapsed1, elapsed2;
    time_val now = now_utc();
    long sum = 0;
    int y; unsigned m,d;

    struct tm tm;
    time_t now_sec = now.sec();
    localtime_r(&now_sec, &tm);
    int offset = tm.tm_gmtoff;
    BOOST_TEST_MESSAGE("TZ offset = " << offset);
    std::tie(y,m,d) = now.to_ymd(true);
    time_t tt = mktime_utc(y,m,d);
    BOOST_TEST_MESSAGE("mktime_utc(" << y << '-' << m << '-' << d << ") = " << tt);
    std::tie(y,m,d) = now.to_ymd(true);
    BOOST_TEST_MESSAGE("now.to_ymd(true)  = " << y << '-' << m << '-' << d << " | " << now.sec());
    std::tie(y,m,d) = now.to_ymd(false);
    BOOST_TEST_MESSAGE("now.to_ymd(false) = " << y << '-' << m << '-' << d << " | " << now.sec()+offset);
    std::tie(y,m,d) = from_gregorian_time(now.sec() + offset);
    BOOST_TEST_MESSAGE("from_greg_time(" << y << '-' << m << '-' << d << ") = " << now.sec()+offset);

    std::tie(y,m,d) = now.to_ymd(false);
    BOOST_TEST_MESSAGE("local to_ymd              = " << y << '-' << m << '-' << d);
    std::tie(y,m,d) = from_gregorian_time(now.sec() + offset);
    BOOST_TEST_MESSAGE("local from_gregorian_days = " << y << '-' << m << '-' << d);
    std::tie(y,m,d) = now.to_ymd(true);
    BOOST_TEST_MESSAGE("utc   to_ymd              = " << y << '-' << m << '-' << d);
    std::tie(y,m,d) = from_gregorian_time(now.sec());
    BOOST_TEST_MESSAGE("utc   from_gregorian_time = " << y << '-' << m << '-' << d);

    BOOST_CHECK(now.to_ymd(false) == from_gregorian_time(now.sec() + offset));
    BOOST_CHECK(now.to_ymd(true)  == from_gregorian_time(now.sec()));
    {
        timer t;
        for (int i=0; i < ITERATIONS; i++, sum += y+m+d)
            std::tie(y,m,d) = now.to_ymd(false);
        elapsed1 = t.elapsed();
    }
    {
        timer t;
        for (int i=0; i < ITERATIONS; i++, sum += y+m+d)
            std::tie(y,m,d) = from_gregorian_time(now.sec() + offset);
        elapsed2 = t.elapsed();
    }
    BOOST_TEST_MESSAGE("local time_val::to_ymd / from_gregorian_days = "
                  << std::setprecision(2) << std::fixed
                  << ((100.0 * elapsed1) / elapsed2) << '%');
    {
        timer t;
        for (int i=0; i < ITERATIONS; i++, sum += y+m+d)
            std::tie(y,m,d) = now.to_ymd(true);
        elapsed1 = t.elapsed();
    }
    {
        timer t;
        for (int i=0; i < ITERATIONS; i++, sum += y+m+d)
            std::tie(y,m,d) = from_gregorian_time(now.sec());
        elapsed2 = t.elapsed();
    }
    BOOST_TEST_MESSAGE("utc   time_val::to_ymd / from_gregorian_days = "
                  << std::setprecision(2) << std::fixed
                  << ((100.0 * elapsed1) / elapsed2) << '%');

    BOOST_CHECK(sum); // suppress the run-time warning
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

