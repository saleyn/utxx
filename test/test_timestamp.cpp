//----------------------------------------------------------------------------
/// \file  test_timestamp.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the timestamp.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-10-15
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects

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

// #define BOOST_TEST_MODULE high_res_timer_test

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <utxx/verbosity.hpp>
#include <utxx/high_res_timer.hpp>
#include <utxx/string.hpp>
#include <stdio.h>
#include <sstream>
#include <iostream>

#ifndef DEBUG_TIMESTAMP
#define DEBUG_TIMESTAMP
#endif
#include <utxx/timestamp.hpp>
#include <utxx/os.hpp>

using namespace utxx;

static long iterations   = ::getenv("ITERATIONS")
                         ? atoi(::getenv("ITERATIONS")) : 100000;
static long nthreads     = ::getenv("THREADS")
                         ? atoi(::getenv("THREADS")) : 1;

struct init {
    init() { high_res_timer::calibrate(200000, 1); }
};

static init s_init;

struct test1 {
    long id;
    long iterations;
    boost::barrier* barrier;
    high_res_timer hr;

    test1(long id, long it, boost::barrier* b)
        : id(id), iterations(it), barrier(b) {}

    void operator() () {
        timestamp::buf_type buf[2];

        if (barrier != NULL)
            barrier->wait();

        for (int i=0; i < iterations; i++) {
            hr.start_incr();
            int n = timestamp::update_and_write(
                DATE_TIME_WITH_USEC, buf[0], sizeof(buf[0]));
            hr.stop_incr();
            time_val t1 = now_utc();
            if (n != 24 || strlen(buf[0]) != 24) {
                std::cerr << "Wrong buffer length: " << buf[0] << std::endl;
                BOOST_REQUIRE_EQUAL(24, n);
            }
            hr.start_incr();
            timestamp::update_and_write(
                DATE_TIME_WITH_USEC, buf[1], sizeof(buf[1]));
            hr.stop_incr();
            time_val t2 = now_utc();
            if (strcmp(buf[0], buf[1]) > 0) {
                std::cerr << "Backward time jump detected: "
                    << buf[0] << ' ' << buf[1]
                    << '(' << t1.sec() << ' ' << t2.sec() << ')' << std::endl;

                BOOST_REQUIRE(strcmp(buf[1], buf[0]) >= 0);
            }
        }

        const time_val& tv = hr.elapsed_time();
        timestamp::format(TIME_WITH_USEC, tv, buf[0], sizeof(buf[0]));

        {
            std::stringstream s; s.precision(6);
            s << "Thread" << id << " timestamp: "
              #ifdef DEBUG_TIMESTAMP
              << "hrcalls=" << timestamp::hrcalls()
              << ", syscalls=" << timestamp::syscalls()
              #endif
              << ", speed=" << std::fixed << ((double)iterations / tv.seconds())
              << ", latency=" << (1000000000.0*tv.seconds()/iterations) << " ns";
            BOOST_TEST_MESSAGE(s.str());
        }
    }
};

struct test2 {
    long id;
    long iterations;
    boost::barrier* barrier;
    high_res_timer hr;

    struct caller {
        int id;
        std::string name;
        int n;

        caller(int id, const char* name, int n)
            : id(id), name(name), n(n) {}

        template <typename F>
        void operator() (F f) {
            time_val start; start.now();
            int j = 0;
            for (int i=0; i < n; i++) {
                f();
                j++;
            }
            time_val end; end.now();
            end -= start;

            assert(!end.empty());

            std::stringstream s; s.precision(1);
            s << "Thread" << id << ' '
              << std::left << std::setw(40) << name << std::right
              << "    speed=" << std::fixed
              << std::setw(6) << ((double)n / end.seconds() / 1000000)
              << " Mcalls/s,";
            s.precision(3);
            s << " latency=" << (1000000.0*end.seconds()/n) << " us";
            BOOST_TEST_MESSAGE(s.str());
        }
    };

    test2(long id, long it, boost::barrier* b)
        : id(id), iterations(it), barrier(b) {}

    void operator() () {
        if (barrier != NULL)
            barrier->wait();

        timer tmr;
        time_val last = now_utc();
        for (int i=0; i < iterations; i++) {
            if (last > timestamp::now()) {
                std::cerr << "Backward time jump detected in test2: "
                    << timestamp::to_string(last) << ' '
                    << timestamp::to_string() << std::endl;
                BOOST_REQUIRE(false);
                break;
            }
        }
        const time_val& tv = tmr.elapsed_time();
        timestamp::buf_type buf;
        timestamp::format(TIME_WITH_USEC, tv, buf, sizeof(buf));

        std::stringstream s; s.precision(1);
        s << "Thread" << id << " timestamp::now() " << buf
          << ", speed=" << std::fixed << tmr.speed(iterations) << " calls/s,";
        s.precision(3);
        s << " latency=" << tmr.latency_usec(iterations) << " us" << std::endl;
        BOOST_TEST_MESSAGE(s.str());

        // Testing gettimeofday speed
        {
            caller test(id, "utxx::now_utc()", iterations);
            test([]{ return now_utc(); });
        }

        // Testing gettimeofday speed
        {
            caller test(id, "gettimeofday()", iterations);
            test([]{ timeval tv; gettimeofday(&tv, NULL); return time_val(tv); });
        }

        // Testing boost::universal_time speed
        {
            caller test(id, "boost::microsec_clock::universal_time()", iterations);
            test(&boost::posix_time::microsec_clock::universal_time);
        }
        // Testing boost::local_time speed
        {
            using namespace boost::posix_time;
            struct w {
                static ptime time() {
                    return microsec_clock::local_time();
                }
            };
            boost::posix_time::microsec_clock::local_time();
            caller test(id, "boost::microsec_clock::local_time()", iterations);
            //test(&boost::posix_time::microsec_clock::local_time);
            test(&w::time);
        }
        // Testing CLOCK_REALTIME speed
        {
            struct option { clockid_t type; const char* desc; };
            const option options[] = {
                { CLOCK_REALTIME,           "CLOCK_REALTIME" },
                { CLOCK_MONOTONIC,          "CLOCK_MONOTONIC" },
                { CLOCK_PROCESS_CPUTIME_ID, "CLOCK_PROCESS_CPUTIME_ID" },
                { CLOCK_THREAD_CPUTIME_ID,  "CLOCK_THREAD_CPUTIME_ID" }
            };

            static clockid_t clock_type;
            struct w {
                static int time() {
                    struct timespec ts;
                    return clock_gettime( clock_type, &ts );
                }
            };

            for (uint32_t i=0; i < sizeof(options)/sizeof(option); i++) {
                std::string title = to_string("clock_gettime(", options[i].desc, ")");
                caller test(id, title.c_str(), iterations);
                clock_type = options[i].type;
                test(w::time);
            }
        }
        // Testing gettimeofday speed
        {
            caller test(id, "timestamp::update()", iterations);
            test(&timestamp::update);
        }

    }
};

BOOST_AUTO_TEST_CASE( test_timestamp_threading )
{
    high_res_timer::calibrate(100000, 2);

    if (nthreads > 0) {
        std::vector<std::shared_ptr<boost::thread>> thread(nthreads);
        boost::barrier barrier(nthreads + 1);

        for (int i=0; i < nthreads; ++i)
            thread[i].reset(new boost::thread(test1(i+1, iterations, &barrier)));

        barrier.wait();

        for (int i=0; i < nthreads; ++i)
            thread[i]->join();
    } else {
        test1 t(1, iterations, NULL);
        t();
    }
    BOOST_REQUIRE(true);
}

BOOST_AUTO_TEST_CASE( test_timestamp_format )
{
    using namespace boost::posix_time;
    ptime epoch(boost::gregorian::date(1970,1,1));
    ptime now_utc   = microsec_clock::universal_time();
    ptime now_local = microsec_clock::local_time();

    time_duration utc_diff = now_utc - epoch;

    timestamp::buf_type buf, expected, expected_utc, temp;

    sprintf(expected, "%d%02d%02d-%02d:%02d:%02d",
        (unsigned short) now_local.date().year(),
        (unsigned short) now_local.date().month(),
        (unsigned short) now_local.date().day(),
        now_local.time_of_day().hours(),
        now_local.time_of_day().minutes(),
        now_local.time_of_day().seconds());

    sprintf(expected_utc, "%d%02d%02d-%02d:%02d:%02d",
        (unsigned short) now_utc.date().year(),
        (unsigned short) now_utc.date().month(),
        (unsigned short) now_utc.date().day(),
        now_utc.time_of_day().hours(),
        now_utc.time_of_day().minutes(),
        now_utc.time_of_day().seconds());

    time_duration local_diff = now_local - epoch;
    int64_t diff = ((now_utc - now_local).total_milliseconds() + 999) / 1000;

    if (verbosity::level() > VERBOSE_NONE) {
        std::cout << " Utc offset = " << diff << std::endl;
        std::cout << " Seconds since UTC        : "
                  << utc_diff.total_seconds() << std::endl;
        std::cout << " Seconds since UTC (local): "
                  << local_diff.total_seconds() << std::endl;

        printf("Expected %s\n", expected);
    }

    struct timeval tv;

    tv.tv_sec = utc_diff.total_seconds(); tv.tv_usec = 100234;

    if (verbosity::level() > VERBOSE_NONE)
        std::cerr << "UTC Offset: " << timestamp::utc_offset() << std::endl;

    time_val tt(tv);
    timestamp::format(TIME, tt, buf);
    strncpy(temp, expected+9, 9);
    BOOST_CHECK_EQUAL(temp, buf);

    timestamp::format(TIME, tt, buf, true);
    strncpy(temp, expected_utc+9, 9);
    BOOST_REQUIRE_EQUAL(temp, buf);

    timestamp::format(TIME_WITH_USEC, tt, buf);
    snprintf(temp, 16, "%s.%06d", expected+9, (int)tv.tv_usec);
    BOOST_REQUIRE_EQUAL(temp, buf);

    timestamp::format(TIME_WITH_MSEC, tt, buf);
    snprintf(temp, 13, "%s.%03d", expected+9, (int)tv.tv_usec / 1000);
    BOOST_REQUIRE_EQUAL(temp, buf);

    timestamp::format(DATE_TIME, tt, buf);
    snprintf(temp, 18, "%s", expected);
    BOOST_REQUIRE_EQUAL(temp, buf);

    timestamp::format(DATE_TIME, tt, buf, true);
    snprintf(temp, 18, "%s", expected_utc);
    BOOST_REQUIRE_EQUAL(temp, buf);

    timestamp::format(DATE_TIME_WITH_USEC, tt, buf);
    snprintf(temp, 25, "%s.%06d", expected, (int)tv.tv_usec);
    BOOST_REQUIRE_EQUAL(temp, buf);

    timestamp::format(DATE_TIME_WITH_USEC, tt, buf, true);
    snprintf(temp, 25, "%s.%06d", expected_utc, (int)tv.tv_usec);
    BOOST_REQUIRE_EQUAL(temp, buf);

    timestamp::format(DATE_TIME_WITH_MSEC, tt, buf);
    snprintf(temp, 22, "%s.%03d", expected, (int)tv.tv_usec / 1000);
    BOOST_REQUIRE_EQUAL(temp, buf);

    timestamp::format(DATE_TIME_WITH_MSEC, tt, buf, true);
    snprintf(temp, 22, "%s.%03d", expected_utc, (int)tv.tv_usec / 1000);
    BOOST_REQUIRE_EQUAL(temp, buf);

    tv = utxx::now_utc().timeval();
    std::string str = timestamp::to_string(tt, DATE_TIME);
    BOOST_REQUIRE_EQUAL(expected, str);

    const char* p;
    p = timestamp::write_time(buf, time_val(10*3600+9*60+8, 123456), TIME, true);
    BOOST_REQUIRE_EQUAL("100908",   std::string(buf, p - buf));

    p = timestamp::write_time(buf, time_val(10*3600+9*60+8, 123456), TIME, true, ':');
    BOOST_REQUIRE_EQUAL("10:09:08", std::string(buf, p - buf));

    p = timestamp::write_time(buf, time_val(10*3600+9*60+8, 123456), TIME_WITH_MSEC, true, ':');
    BOOST_REQUIRE_EQUAL("10:09:08.123", std::string(buf, p - buf));

    p = timestamp::write_time(buf, time_val(10*3600+9*60+8, 123456), TIME_WITH_MSEC, true, '\0');
    BOOST_REQUIRE_EQUAL("100908.123", std::string(buf, p - buf));

    p = timestamp::write_time(buf, time_val(10*3600+9*60+8, 123456), TIME_WITH_MSEC, true, '\0', '\0');
    BOOST_REQUIRE_EQUAL("100908123", std::string(buf, p - buf));

    p = timestamp::write_time(buf, time_val(10*3600+9*60+8, 123456), TIME_WITH_USEC, true, '\0');
    BOOST_REQUIRE_EQUAL("100908.123456", std::string(buf, p - buf));

    p = timestamp::write_time(buf, time_val(10*3600+9*60+8, 123456), TIME_WITH_USEC, true, ':');
    BOOST_REQUIRE_EQUAL("10:09:08.123456", std::string(buf, p - buf));

    BOOST_CHECK(is_leap(0));
    BOOST_CHECK(is_leap(4));
    BOOST_CHECK(is_leap(2004));
    BOOST_CHECK(is_leap(2008));
    BOOST_CHECK(is_leap(2016));
    BOOST_CHECK(is_leap(1600));
    BOOST_CHECK(is_leap(2000));
    BOOST_CHECK(is_leap(2400));
    BOOST_CHECK(!is_leap(2001));
    BOOST_CHECK(!is_leap(2002));
    BOOST_CHECK(!is_leap(2003));
    BOOST_CHECK(!is_leap(1700));
    BOOST_CHECK(!is_leap(1800));
    BOOST_CHECK(!is_leap(2100));
    BOOST_CHECK(!is_leap(2200));
}

BOOST_AUTO_TEST_CASE( test_time_latency )
{
    if (nthreads > 0) {
        std::vector<std::shared_ptr<boost::thread>> thread(nthreads);
        boost::barrier barrier(nthreads + 1);

        for (int i=0; i < nthreads; ++i)
            thread[i].reset(new boost::thread(test2(i+1, iterations, &barrier)));

        barrier.wait();

        for (int i=0; i < nthreads; ++i)
            thread[i]->join();
    } else {
        test2 t(1, iterations, NULL);
        t();
    }
    BOOST_REQUIRE(true);
}

BOOST_AUTO_TEST_CASE( test_timestamp_time )
{
    auto ttt = now_utc();
    timestamp::update_midnight_nseconds(ttt);

    enum { ITER = 10 };

    high_res_timer::calibrate(200000, 3);

    hrtime_t t1 = high_res_timer::gettime();
    hrtime_t t2 = high_res_timer::gettime();
    if (verbosity::level() > VERBOSE_NONE)
        std::cout << "Ajacent hrtime ticks diff: " << (t2 - t1) << " ("
            << ((t2 - t1) * 1000u / high_res_timer::global_scale_factor())
            << " ns)" << std::endl;

    high_res_timer t3;
    t3.start();
    for (int i=0; i < ITER; i++);
    t3.stop();
    if (verbosity::level() > VERBOSE_NONE)
        std::cout << "Iterations: " << ITER
            << ". Elapsed nsec: " << t3.elapsed_nsec() << std::endl;

    int n = 0, m = 0;

    timestamp t;
    for (int i = 0; i < ITER; i++) {
        time_val tv1 = now_utc();
        while (tv1 == t.update()) m++;
        time_val tv2 = now_utc();

        n += (tv2 - tv1).microseconds();
    }

    if (verbosity::level() > VERBOSE_NONE) {
        std::cout << "Global factor: " << high_res_timer::global_scale_factor() << std::endl;
        std::cout << "usecs between adjacent now() calls: " << (n / ITER) << std::endl;
        std::cout << "cached time calls: " << m << std::endl;
        #ifdef DEBUG_TIMESTAMP
        std::cout << "hrcalls: "  << timestamp::hrcalls()  << " (" << ITER << " iter)" << std::endl;
        std::cout << "syscalls: " << timestamp::syscalls() << " (" << ITER << " iter)" << std::endl;
        #endif
    }
    BOOST_REQUIRE(true); // Prevent run-time from complaining of missing tests
}

BOOST_AUTO_TEST_CASE( test_timestamp_since_midnight )
{
    time_val now = timestamp::update();

    uint64_t expect_utc_sec_since_midnight =
        1000000ll * (now.sec() % 86400) + now.usec();

    //time_t ssm = timestamp::utc_midnight_seconds();

    int64_t got = timestamp::utc_usec_since_midnight(now);

    BOOST_REQUIRE_EQUAL((int64_t)expect_utc_sec_since_midnight, got);

    time_t sec = now.sec();
    struct tm tm;
    ::localtime_r(&sec, &tm);
    int l_utc_offset = tm.tm_gmtoff;

    if (verbosity::level() > VERBOSE_NONE)
        std::cerr << "UTC Offset: " << l_utc_offset << std::endl;

    int64_t expect_local_usec_since_midnight =
        1000000ll * (tm.tm_hour*3600 + tm.tm_min*60 + tm.tm_sec) + now.usec();

    got = timestamp::local_usec_since_midnight(now);

    BOOST_REQUIRE_EQUAL(expect_local_usec_since_midnight, got);

    auto env = getenv("TZ");
    setenv("TZ", "EST", 1);

    auto tv = time_val::universal_time(2000, 1, 2, 23, 59, 59, 0);
    timestamp::update_midnight_nseconds(tv);
    auto mu = timestamp::utc_next_midnight_time();
    auto ml = timestamp::local_next_midnight_time();

    auto tu = time_val::universal_time(2000, 1, 3, 0, 0, 0, 0);
    auto tl = tu + nsecs(timestamp::utc_offset_nseconds());

    BOOST_CHECK_EQUAL(mu.nsec(), tu.nsec());
    BOOST_CHECK_EQUAL(ml.nsec(), tl.nsec());

    BOOST_CHECK_EQUAL("20000102-", timestamp::cached_utc_timestamp());
    BOOST_CHECK_EQUAL("20000102-", timestamp::cached_local_timestamp());

    //------------------
    tv = ml - secs(1);
    timestamp::update_midnight_nseconds(tv);
    mu = timestamp::utc_next_midnight_time();
    ml = timestamp::local_next_midnight_time();

    tl = tv + nsecs(timestamp::utc_offset_nseconds());
    tu = time_val::universal_time(2000, 1, 4, 0, 0, 0, 0);

    BOOST_CHECK_EQUAL(mu.nsec(), tu.nsec());
    if (verbosity::level() > VERBOSE_NONE) {
        std::cout
            << "  ML=" << timestamp::to_string(ml, DATE_TIME, true, false) << "UTC"
            << ", TL=" << timestamp::to_string(tl, DATE_TIME, true, false) << "EST"
            << ", TV=" << timestamp::to_string(tv, DATE_TIME, true, false) << "EST"
            << "\n"
            << "  ML="   << ml.sec() << std::endl
            << "  TL="   << tl.sec() << std::endl;
    }
    BOOST_CHECK(ml.nanoseconds() > tl.nanoseconds());

    BOOST_CHECK_EQUAL("20000103-", timestamp::cached_utc_timestamp());
    BOOST_CHECK_EQUAL("20000102-", timestamp::cached_local_timestamp());

    //------------------
    tv += secs(1);
    auto h = abs(int(timestamp::utc_offset() / 3600));
    tu = time_val::universal_time(2000, 1, 3, h, 0, 0, 0);
    timestamp::update_midnight_nseconds(tv);

    BOOST_CHECK_EQUAL(tv.nanoseconds(), tu.nanoseconds());

    mu = timestamp::utc_next_midnight_time();
    ml = timestamp::local_next_midnight_time();

    tu = time_val::universal_time(2000, 1, 4, 0, 0, 0, 0);
    tl = tu + nsecs(timestamp::utc_offset_nseconds());

    BOOST_CHECK_EQUAL(mu.nsec(), tu.nsec());
    BOOST_CHECK_EQUAL(ml.nsec(), tl.nsec());

    BOOST_CHECK_EQUAL("20000103-", timestamp::cached_utc_timestamp());
    BOOST_CHECK_EQUAL("20000103-", timestamp::cached_local_timestamp());

    if (env)
        setenv("TZ", env, 1);
}
