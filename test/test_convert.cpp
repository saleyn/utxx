//----------------------------------------------------------------------------
/// \file  test_convert.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for integer to/from string conversion classes.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-30
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx project.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/config.h>

//#define BOOST_TEST_MODULE test_convert
#include <boost/test/unit_test.hpp>
#include <utxx/convert.hpp>
#include <utxx/fast_itoa.hpp>
#include <utxx/verbosity.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#ifdef UTXX_HAVE_BOOST_TIMER_TIMER_HPP
#include <boost/timer/timer.hpp>
#endif
#include <limits>

using namespace utxx;


BOOST_AUTO_TEST_CASE( test_convert )
{
    {
        char buf1[] = "0";
        char buf[] = {'0'};
        BOOST_STATIC_ASSERT(sizeof(buf1) == 1 + sizeof(buf));
        BOOST_STATIC_ASSERT(sizeof(buf1) == 2);
        int n, m;
        const char* lp = atoi_left (buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_CHECK_EQUAL(0,  n);
        BOOST_CHECK_EQUAL(0,  m);
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_CHECK_EQUAL(-1, rp-buf);

        char lout[sizeof(buf)], rout[sizeof(buf)];
        lp = itoa_left(lout, n);
        rp = itoa_right(rout, m);
        BOOST_CHECK_EQUAL(std::string(buf, sizeof(buf)), std::string(lout, sizeof(lout)));
        BOOST_CHECK_EQUAL(std::string(buf, sizeof(buf)), std::string(rout, sizeof(rout)));
        BOOST_CHECK_EQUAL(0,  n);
        BOOST_CHECK_EQUAL(0,  m);
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-lout);
        BOOST_CHECK_EQUAL(-1, rp-rout);
    }
    {
        char buf[] = {'-','0'};
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_CHECK_EQUAL(0,  n);
        BOOST_CHECK_EQUAL(0,  m);
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_CHECK_EQUAL(-1, rp-buf);

        char lout[sizeof(buf)], rout[sizeof(buf)];
        lp = itoa_left(lout, n);
        rp = itoa_right(rout, m);
        BOOST_CHECK_EQUAL(std::string(buf+1, sizeof(buf)-1), std::string(lout, sizeof(lout)-1));
        BOOST_CHECK_EQUAL(std::string(buf+1, sizeof(buf)-1), std::string(rout+1, sizeof(rout)-1));
        BOOST_CHECK_EQUAL(0,  n);
        BOOST_CHECK_EQUAL(0,  m);
        BOOST_CHECK_EQUAL((int)sizeof(buf)-1, lp-lout);
        BOOST_CHECK_EQUAL(0, rp-rout);
    }
    {
        char buf[] = {'1'};
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_CHECK_EQUAL(1,  n);
        BOOST_CHECK_EQUAL(1,  m);
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_CHECK_EQUAL(-1, rp-buf);

        char lout[sizeof(buf)], rout[sizeof(buf)];
        lp = itoa_left(lout, n);
        rp = itoa_right(rout, m);
        BOOST_CHECK_EQUAL(std::string(buf, sizeof(buf)), std::string(lout, sizeof(lout)));
        BOOST_CHECK_EQUAL(std::string(buf, sizeof(buf)), std::string(rout, sizeof(rout)));
        BOOST_CHECK_EQUAL(1,  n);
        BOOST_CHECK_EQUAL(1,  m);
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-lout);
        BOOST_CHECK_EQUAL(-1, rp-rout);
    }
    {
        char buf[] = {'\0','-','1'};
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_CHECK_EQUAL(0,  n);
        BOOST_CHECK_EQUAL(-1, m);
        BOOST_CHECK_EQUAL(0, lp-buf);
        BOOST_CHECK_EQUAL(0, rp-buf);

        char lout[sizeof(buf)], rout[sizeof(buf)];
        lp = itoa_left(lout, -1);
        rp = itoa_right(rout, -1);
        BOOST_CHECK_EQUAL('\0', lout[2]);
        BOOST_CHECK_EQUAL(std::string(buf+1, sizeof(buf)-1), lout);
        BOOST_CHECK_EQUAL(std::string(buf+1, sizeof(buf)-1), std::string(rout+1, sizeof(rout)-1));
        BOOST_CHECK_EQUAL((int)sizeof(buf)-1, lp-lout);
        BOOST_CHECK_EQUAL(0,  rp-rout);

        lp = itoa_left(lout, -1,  ' ');
        rp = itoa_right(rout, -1, ' ');
        BOOST_CHECK_EQUAL("-1 ", std::string(lout, 3));
        BOOST_CHECK_EQUAL(" -1", std::string(rout, 3));
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-lout);
        BOOST_CHECK_EQUAL(-1, rp-rout);
    }
    {
        char buf[] = {'1', '2', '3', '4', '5'};
        BOOST_STATIC_ASSERT((int)sizeof(buf) == 5);
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_CHECK_EQUAL(12345, n);
        BOOST_CHECK_EQUAL(12345, m);
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_CHECK_EQUAL(-1, rp-buf);

        char lout[sizeof(buf)], rout[sizeof(buf)];
        lp = itoa_left(lout, n);
        rp = itoa_right(rout, m);
        BOOST_CHECK_EQUAL(std::string(buf, sizeof(buf)), std::string(lout, sizeof(lout)));
        BOOST_CHECK_EQUAL(std::string(buf, sizeof(buf)), std::string(rout, sizeof(rout)));
        BOOST_CHECK_EQUAL(12345, n);
        BOOST_CHECK_EQUAL(12345, m);
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-lout);
        BOOST_CHECK_EQUAL(-1,    rp-rout);
    }
    {
        char buf[] = {'-', '2', '3', '4', '5'};
        BOOST_STATIC_ASSERT(sizeof(buf) == 5);
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_CHECK_EQUAL(-2345, n);
        BOOST_CHECK_EQUAL(-2345, m);
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_CHECK_EQUAL(-1, rp-buf);

        char lout[sizeof(buf)], rout[sizeof(buf)];
        lp = itoa_left(lout, n);
        rp = itoa_right(rout, m);
        BOOST_CHECK_EQUAL(std::string(buf, sizeof(buf)), std::string(lout, sizeof(lout)));
        BOOST_CHECK_EQUAL(std::string(buf, sizeof(buf)), std::string(rout, sizeof(rout)));
        BOOST_CHECK_EQUAL(-2345, n);
        BOOST_CHECK_EQUAL(-2345, m);
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-lout);
        BOOST_CHECK_EQUAL(-1,    rp-rout);
    }
    {
        char buf[] = {' ', ' ', '1', '2', '3', '4', '5'};
        BOOST_STATIC_ASSERT(sizeof(buf) == 7);
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_CHECK_EQUAL(0, n);
        BOOST_CHECK_EQUAL(12345, m);
        BOOST_CHECK_EQUAL(0, lp-buf);
        BOOST_CHECK_EQUAL(1, rp-buf);

        char lbuf[] = {'1', '2', '3', '4', '5', ' ', ' '};
        char lout[sizeof(buf)], rout[sizeof(buf)] = {};
        lp = itoa_left(lout, 12345);
        rp = itoa_right(rout, 12345);
        BOOST_CHECK_EQUAL(std::string(lbuf, sizeof(lbuf)-2), std::string(lout, sizeof(lout)-2));
        BOOST_CHECK_EQUAL(std::string(buf+2, sizeof(buf)-2), std::string(rout+2, sizeof(rout)-2));
        BOOST_CHECK_EQUAL((int)sizeof(lbuf)-2, lp-lout);
        BOOST_CHECK_EQUAL(1,    rp-rout);

        bzero(lout, sizeof(lout));
        bzero(rout, sizeof(rout));
        lp = itoa_left(lout, 12345, ' ');
        rp = itoa_right(rout, 12345, ' ');
        BOOST_CHECK_EQUAL(std::string(lbuf, sizeof(lbuf)), std::string(lout, sizeof(lout)));
        BOOST_CHECK_EQUAL(std::string(buf, sizeof(buf)), std::string(rout, sizeof(rout)));
        BOOST_CHECK_EQUAL((int)sizeof(lbuf), lp-lout);
        BOOST_CHECK_EQUAL(-1,    rp-rout);
    }
    {
        char buf[] = {' ', '-', '1', '2', '3', '4', '5'};
        BOOST_STATIC_ASSERT((int)sizeof(buf) == 7);
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_CHECK_EQUAL(0, n);      // Error
        BOOST_CHECK_EQUAL(-12345, m); // Success
        BOOST_CHECK_EQUAL(0, lp-buf); // nothing was parsed - pointing to the beginning of the buffer.
        BOOST_CHECK_EQUAL(0, rp-buf); // points to buf[0] - parsing was done from right to left.

        char lbuf[] = {'-', '1', '2', '3', '4', '5', 'X'};
        char lout[sizeof(buf)], rout[sizeof(buf)] = {};
        lp = itoa_left(lout, -12345);
        rp = itoa_right(rout, -12345);
        BOOST_CHECK_EQUAL(std::string(lbuf, sizeof(lbuf)-1), std::string(lout, sizeof(lout)-1));
        BOOST_CHECK_EQUAL(std::string(buf+1, sizeof(buf)-1), std::string(rout+1, sizeof(rout)-1));
        BOOST_CHECK_EQUAL((int)sizeof(lbuf)-1, lp-lout);
        BOOST_CHECK_EQUAL(0,    rp-rout);
    }
    {
        long expect = -1053806107;
        char buf[]  = {'-','1','0','5','3','8','0','6','1','0','7'};
        long n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_CHECK_EQUAL(expect, n);
        BOOST_CHECK_EQUAL(expect, m);
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_CHECK_EQUAL(-1, rp-buf);
    }
    {
        char buf[]  = {'-',' ',' ','1','0','5'};
        long n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_CHECK_EQUAL(0,   n); // This is really invalid input
        BOOST_CHECK_EQUAL(105, m); // Invalid input
        BOOST_CHECK_EQUAL(1,  lp-buf);
        BOOST_CHECK_EQUAL(2,  rp-buf);
    }
    {
        char buf[19];
        int64_t n = std::numeric_limits<int64_t>::max();
        std::stringstream s; s << n;
        strncpy(buf, s.str().c_str(), s.str().size());
        BOOST_CHECK_EQUAL(sizeof(buf), s.str().size());
        int64_t k = boost::lexical_cast<int64_t>(s.str());
        int64_t i, j;
        BOOST_CHECK_EQUAL(n, k);
        const char* lp = atoi_left(buf, i);
        const char* rp = atoi_right(buf, j);
        BOOST_CHECK_EQUAL(n, i);
        BOOST_CHECK_EQUAL(n, j);
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_CHECK_EQUAL(-1, rp-buf);
        //BOOST_CHECK_EQUAL(0,    rp-rout);
    }
    {
        char buf[20];
        int64_t n = std::numeric_limits<int64_t>::min();
        BOOST_CHECK(n < 0);
        std::stringstream s; s << n;
        strncpy(buf, s.str().c_str(), s.str().size());
        BOOST_CHECK_EQUAL(sizeof(buf), s.str().size());
        int64_t k = boost::lexical_cast<int64_t>(s.str());
        int64_t i, j;
        BOOST_CHECK_EQUAL(n, k);
        const char* lp = atoi_left(buf, i);
        const char* rp = atoi_right(buf, j);
        BOOST_CHECK_EQUAL(n, i);
        BOOST_CHECK_EQUAL(n, j);
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_CHECK_EQUAL(-1, rp-buf);
    }
    {
        char buf[20];
        uint64_t n = std::numeric_limits<uint64_t>::max();
        uint64_t m = std::numeric_limits<uint64_t>::min();
        BOOST_CHECK(n > 0);
        BOOST_CHECK(m == 0);
        std::stringstream s; s << n;
        strncpy(buf, s.str().c_str(), s.str().size());
        BOOST_CHECK_EQUAL(sizeof(buf), s.str().size());
        uint64_t k = boost::lexical_cast<uint64_t>(s.str());
        uint64_t i, j;
        BOOST_CHECK_EQUAL(n, k);
        const char* lp = atoi_left(buf, i);
        const char* rp = atoi_right(buf, j);
        BOOST_CHECK_EQUAL(n, i);
        BOOST_CHECK_EQUAL(n, j);
        BOOST_CHECK_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_CHECK_EQUAL(-1, rp-buf);
    }
    {
        char buf[10];
        char* p = buf;
        itoa<int>(0, p);
        BOOST_CHECK_EQUAL("0", buf);
        BOOST_CHECK_EQUAL(1, p - buf);
    }
}

BOOST_AUTO_TEST_CASE( test_convert_fast_atoi2 )
{

    for (int64_t i=1, j=1; j < 100000; ++j) {
        char buf[21], test[21];
        i = (rand() % 2 ? -1 : 1) * 1ul << (j % 64);
        sprintf(buf, "%-ld", i);
        int64_t n = atol(buf), m;
        atoi_left(buf, m);
        BOOST_CHECK_EQUAL(n, m);
        const char* p = itoa_left(test, n);
        m = p-test;
        BOOST_CHECK_EQUAL((int64_t)strlen(buf), m);
        BOOST_CHECK_EQUAL(buf, std::string(test, m));
    }
}

__attribute__ ((noinline)) void atoi2(const std::string& s, long& n) {
    n = atoi(s.c_str());
}

BOOST_AUTO_TEST_CASE( test_convert_unsafe_fixed_atol )
{
    struct utest { const char* test; long expected; bool fatoi_success; int len; };
    const utest tests[] = {
        { "123456989012345678",   123456989012345678, true, 18 },   // 0
        { "-123456989012345678", -123456989012345678, true, 19 },   // 1
        { "   123",               123,               false, 6  },   // 2
        // unsafe_fixed_ato{u}l does no checking of valid '0'..'9' chars
        // hence the result may not be as expected. E.g.: 'A' & 0xF -> 1
        { "123ABC",               123123,            false, 6 },   // 3
        { "123   ",               123000,            false, 6 },   // 4
        { "\0\0\000123",          0,                 false, 6 },   // 5
        // unsafe_fixed_ato{u}l does no checking of valid '0'..'9' chars
        // hence the result may not be as expected. E.g.: 'A' & 0xF -> 1
        { "-123ABC",              -123123,           false, 7 },   // 6
        { "-123   ",              -123000,           false, 7 },   // 7
        { "-\0\0\000123",         -123,              false, 7 },   // 8
        { "\0\0\000-123",         0,                 false, 7 },   // 9
        { "-   123",              -123,              false, 7 },   // 10
        { "-000123",              -123,              true,  7 },   // 11
    };

    typedef const char* (*fun_u)(const char*, uint64_t&);
    typedef const char* (*fun_s)(const char*, int64_t&);

    static const fun_s fun_signed[] = {
        NULL,
        &unsafe_fixed_atol<1>,
        &unsafe_fixed_atol<2>,
        &unsafe_fixed_atol<3>,
        &unsafe_fixed_atol<4>,
        &unsafe_fixed_atol<5>,
        &unsafe_fixed_atol<6>,
        &unsafe_fixed_atol<7>,
        &unsafe_fixed_atol<8>,
        &unsafe_fixed_atol<9>,
        &unsafe_fixed_atol<10>,
        &unsafe_fixed_atol<11>,
        &unsafe_fixed_atol<12>,
        &unsafe_fixed_atol<13>,
        &unsafe_fixed_atol<14>,
        &unsafe_fixed_atol<15>,
        &unsafe_fixed_atol<16>,
        &unsafe_fixed_atol<17>,
        &unsafe_fixed_atol<18>,
        &unsafe_fixed_atol<19>
    };
    static const fun_u fun_unsigned[] = {
        NULL,
        &unsafe_fixed_atoul<1>,
        &unsafe_fixed_atoul<2>,
        &unsafe_fixed_atoul<3>,
        &unsafe_fixed_atoul<4>,
        &unsafe_fixed_atoul<5>,
        &unsafe_fixed_atoul<6>,
        &unsafe_fixed_atoul<7>,
        &unsafe_fixed_atoul<8>,
        &unsafe_fixed_atoul<9>,
        &unsafe_fixed_atoul<10>,
        &unsafe_fixed_atoul<11>,
        &unsafe_fixed_atoul<12>,
        &unsafe_fixed_atoul<13>,
        &unsafe_fixed_atoul<14>,
        &unsafe_fixed_atoul<15>,
        &unsafe_fixed_atoul<16>,
        &unsafe_fixed_atoul<17>,
        &unsafe_fixed_atoul<18>,
        &unsafe_fixed_atoul<19>
    };

    long n;
    unsigned long u;

    for (uint32_t i=0; i < sizeof(tests)/sizeof(tests[0]); ++i) {
        bool res = fast_atoi(tests[i].test, n);
        if (tests[i].fatoi_success != res)
            BOOST_TEST_MESSAGE(
                "Case i=" << i << ", res=" << res <<
                ", n=" << n << ", test=" << tests[i].test);
        BOOST_CHECK_EQUAL(tests[i].fatoi_success,
                            fast_atoi(tests[i].test, n));
        if (tests[i].fatoi_success) BOOST_CHECK_EQUAL(tests[i].expected, n);

        const char* e = fun_signed[tests[i].len](tests[i].test, n);
        bool       ok = !tests[i].expected || e == (tests[i].test + tests[i].len);

        if (tests[i].expected != n || !ok)
            BOOST_TEST_MESSAGE("Test signed #" << i << " failed");
        BOOST_CHECK_EQUAL(tests[i].expected, n);
        BOOST_CHECK(ok);

        if (tests[i].expected >= 0) {
            e  = fun_unsigned[tests[i].len](tests[i].test, u);
            ok = !tests[i].expected || e == (tests[i].test + tests[i].len);

            if (tests[i].expected != (long)u || !ok)
                BOOST_TEST_MESSAGE("Test unsigned #" << i << " failed");
            BOOST_CHECK_EQUAL(tests[i].expected, (long)u);
            BOOST_CHECK(ok);
        }

    }
}

BOOST_AUTO_TEST_CASE( test_convert_fast_atoi )
{
    long n;

    BOOST_CHECK(!fast_atoi("123ABC", n));
    BOOST_CHECK((fast_atoi<long,false>( "123ABC", n)));
    BOOST_CHECK_EQUAL(123, n);

    BOOST_CHECK(!fast_atoi( "123  ", n));
    BOOST_CHECK((fast_atoi<long,false>( "123  ",  n)));
    BOOST_CHECK_EQUAL(123, n);

    BOOST_CHECK(!fast_atoi( std::string("\0\0\0\000123", 7), n));
    BOOST_CHECK(fast_atoi_skip_ws( std::string("\0\0\0\000123", 7), n));
    BOOST_CHECK_EQUAL(123, n);

    BOOST_CHECK(!fast_atoi( "        -123", n));
    BOOST_CHECK(fast_atoi_skip_ws( "        -123", n));
    BOOST_CHECK_EQUAL(-123, n);

    BOOST_CHECK(!fast_atoi( "-123ABC", n));
    BOOST_CHECK((fast_atoi<long,false>( "-123ABC",  n)));
    BOOST_CHECK_EQUAL(-123, n);

    BOOST_CHECK(!fast_atoi( "-123  ",  n));
    BOOST_CHECK((fast_atoi<long,false>( "-123  ",   n)));
    BOOST_CHECK_EQUAL(-123, n);

    BOOST_CHECK(!fast_atoi( std::string("\0\0\0\000-123", 8), n));
    BOOST_CHECK(fast_atoi_skip_ws( std::string("\0\0\0\000-123", 8), n));
    BOOST_CHECK_EQUAL(-123, n);
}

BOOST_AUTO_TEST_CASE( test_convert_fast_atoi_speed )
{
    using boost::timer::cpu_timer;
    using boost::timer::cpu_times;
    using boost::timer::nanosecond_type;

    const long ITERATIONS = getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 1000000;

    BOOST_TEST_MESSAGE(       "             iterations: " << ITERATIONS);
    {
        cpu_timer t;
        const std::string buf("1234567890");
        long n;
        BOOST_CHECK(fast_atoi(buf, n));
        BOOST_CHECK_EQUAL(1234567890, n);

        for (int i = 0; i < ITERATIONS; i++)
            fast_atoi(buf, n);

        cpu_times elapsed_times(t.elapsed());
        nanosecond_type t1 = elapsed_times.system + elapsed_times.user;

        std::stringstream s;
        s << boost::format("         fast_atoi time: %.3fs (%.3fus/call)")
            % ((double)t1 / 1000000000.0) % ((double)t1 / ITERATIONS / 1000.0);
        BOOST_TEST_MESSAGE(s.str());
    }
    {
        cpu_timer t;
        const std::string buf("1234567890");
        const char* p = buf.c_str();
        long n = unsafe_fixed_atoul<10>(p);
        BOOST_CHECK_EQUAL(1234567890, n);

        for (int i = 0; i < ITERATIONS; i++) {
            p = buf.c_str();
            unsafe_fixed_atoul<10>(p);
            // Trick the compiler into thinking that p changed to prevent loop opimization
            asm volatile("" : "+g"(p));
        }

        cpu_times elapsed_times(t.elapsed());
        nanosecond_type t1 = elapsed_times.system + elapsed_times.user;

        std::stringstream s;
        s << boost::format("unsafe_fixed_atoul time: %.3fs (%.3fus/call)")
            % ((double)t1 / 1000000000.0) % ((double)t1 / ITERATIONS / 1000.0);

        BOOST_TEST_MESSAGE(s.str());
    }
    {
        cpu_timer t;
        const std::string buf("1234567890");
        long n;
        BOOST_CHECK_EQUAL(1234567890, atoi(buf.c_str()));

        for (int i = 0; i < ITERATIONS; i++)
            atoi2(buf, n);

        cpu_times elapsed_times(t.elapsed());
        nanosecond_type t1 = elapsed_times.system + elapsed_times.user;

        std::stringstream s;
        s << boost::format("              atoi time: %.3fs (%.3fus/call)")
            % ((double)t1 / 1000000000.0) % ((double)t1 / ITERATIONS / 1000.0);
        BOOST_TEST_MESSAGE(s.str());
    }
}

BOOST_AUTO_TEST_CASE( test_convert_skip_left )
{
    long n, m;
    char buf[] = {'1','2','3','4','5'};
    atoi_left(buf, n, '1');
    atoi_right(buf, m, '1');
    BOOST_CHECK_EQUAL(2345, n);
    BOOST_CHECK_EQUAL(12345, m);

    m = 0;
    atoi_right<long, 5>("12345", m, '5');
    BOOST_CHECK_EQUAL(1234, m);

    n = m = 0;
    atoi_left<long, 7> ("12345  ", n, ' ');
    atoi_right<long, 7>("  12345", m, ' ');
    BOOST_CHECK_EQUAL(12345, n);
    BOOST_CHECK_EQUAL(12345, m);

    n = m = 0;
    atoi_left<long, 9> ("  12345  ", n, ' ');
    atoi_right<long, 9>("  12345  ", m, ' ');
    BOOST_CHECK_EQUAL(12345, n);
    BOOST_CHECK_EQUAL(12345, m);

    n = m = 0;
    atoi_left<long, 7>("0012345", n, '0');
    atoi_right<long, 7>("1234500", m, '0');
    BOOST_CHECK_EQUAL(12345, n);
    BOOST_CHECK_EQUAL(12345, m);
}

BOOST_AUTO_TEST_CASE( test_convert_ftoa )
{
    char buf[32];
    int n = ftoa_left(0.6, buf, sizeof(buf), 3, false);
    BOOST_CHECK_EQUAL("0.600", buf);
    BOOST_CHECK_EQUAL(5, n);

    n = ftoa_left(123.19, buf, sizeof(buf), 3, true);
    BOOST_CHECK_EQUAL("123.19", buf);
    BOOST_CHECK_EQUAL(6, n);

    n = ftoa_left(0.999, buf, sizeof(buf), 2, false);
    BOOST_CHECK_EQUAL("1.00", buf);
    BOOST_CHECK_EQUAL(4, n);

    // Note that 1.005 is really 1.0049999999...
    n = ftoa_left(1.005, buf, sizeof(buf), 2, false);
    BOOST_CHECK_EQUAL("1.00", buf);
    BOOST_CHECK_EQUAL(4, n);

    // Note that 1.005 is really 1.0049999999...
    n = ftoa_left(1.005, buf, sizeof(buf), 2, true);
    BOOST_CHECK_EQUAL("1.0", buf);
    BOOST_CHECK_EQUAL(3, n);

    n = ftoa_left(-1.005, buf, sizeof(buf), 2, false);
    BOOST_CHECK_EQUAL("-1.00", buf);
    BOOST_CHECK_EQUAL(5, n);

    n = ftoa_left(-1.005, buf, sizeof(buf), 2, true);
    BOOST_CHECK_EQUAL("-1.0", buf);
    BOOST_CHECK_EQUAL(4, n);

    n = ftoa_left(0.145, buf, sizeof(buf), 1, true);
    BOOST_CHECK_EQUAL("0.1", buf);
    BOOST_CHECK_EQUAL(3, n);

    n = ftoa_left(-1.0, buf, sizeof(buf), 20, false);
    BOOST_CHECK_EQUAL("-1.00000000000000000000", buf);
    BOOST_CHECK_EQUAL(23, n);

    union {double d; int64_t i;} f;
    f.i = 0x7ff0000000000000ll;
    n = ftoa_left(f.d, buf, sizeof(buf), 29, true);
    BOOST_CHECK_EQUAL("inf", buf);
    BOOST_CHECK_EQUAL(3, n);

    f.i = 0xfff0000000000000ll;
    n = ftoa_left(f.d, buf, sizeof(buf), 29, true);
    BOOST_CHECK_EQUAL("-inf", buf);
    BOOST_CHECK_EQUAL(4, n);

    f.i = 0x7ff0000000000001ll;
    n = ftoa_left(f.d, buf, sizeof(buf), 29, true);
    BOOST_CHECK_EQUAL("nan", buf);
    BOOST_CHECK_EQUAL(3, n);

    f.i = 0xfff0000000000001ll; /* The left-most sign bif doesn't matter for nan */
    n = ftoa_left(f.d, buf, sizeof(buf), 29, true);
    BOOST_CHECK_EQUAL("nan", buf);
    BOOST_CHECK_EQUAL(3, n);

    n = ftoa_left(1.0, buf, sizeof(buf), 29, true);
    BOOST_CHECK_EQUAL("1.0", buf);
    BOOST_CHECK_EQUAL(3, n);

    n = ftoa_left(1.0, buf, sizeof(buf), 30, true);
    BOOST_CHECK_EQUAL(-1, n);

    BOOST_CHECK_THROW(ftoa_right(1.0, buf, -4, 2), std::invalid_argument);
    BOOST_CHECK_THROW(ftoa_right(1.0, buf,  4, 4), std::invalid_argument);

    ftoa_right(1.0, buf, 5, 2);
    BOOST_CHECK_EQUAL(" 1.00", std::string(buf, 5));
    ftoa_right(-1.0, buf, 5, 2);
    BOOST_CHECK_EQUAL("-1.00", std::string(buf, 5));
    BOOST_CHECK_THROW(ftoa_right(-1.0, buf, 4, 2), std::invalid_argument);

    ftoa_right(189.23, buf, 9, 2, '0');
    BOOST_CHECK_EQUAL("000189.23", std::string(buf, 9));

    ftoa_right(189.23, buf, 11, 4, '0');
    BOOST_CHECK_EQUAL("000189.2300", std::string(buf, 11));

    ftoa_right(-1.8249376054, buf, 10, 5, ' ');
    BOOST_CHECK_EQUAL("  -1.82494", std::string(buf, 10));

    ftoa_right(-12.8249376, buf, 10, 5, ' ');
    BOOST_CHECK_EQUAL(" -12.82494", std::string(buf, 10));

    ftoa_right(-123.8249376, buf, 10, 5, ' ');
    BOOST_CHECK_EQUAL("-123.82494", std::string(buf, 10));
}

BOOST_AUTO_TEST_CASE( test_convert_itoa_right_string )
{
    BOOST_CHECK_EQUAL("0001", (itoa_right<int, 4>(1, '0')));
    BOOST_CHECK_EQUAL("0000", (itoa_right<int, 4>(0, '0')));
    BOOST_CHECK_EQUAL("   0", (itoa_right<int, 4>(0, ' ')));
    BOOST_CHECK_EQUAL("1",    (itoa_right<int, 10>(1)));
}

BOOST_AUTO_TEST_CASE( test_convert_itoa_hex )
{
    char  buf[6];
    char* p = buf;

    BOOST_CHECK(itoa_hex(0xA23F, p, sizeof(buf)-1));
    BOOST_CHECK_EQUAL("A23F", buf);

    p = buf;
    BOOST_CHECK(itoa_hex(0, p, sizeof(buf)-1));

    p = buf;
    BOOST_CHECK(itoa_hex(0x12345678, p, sizeof(buf)-1) > int(sizeof(buf)-1));
}

BOOST_AUTO_TEST_CASE( test_convert_itoa_bits )
{
    BOOST_CHECK_EQUAL("10000000", (itoa_bits<uint64_t,true, 2>(1ul    << 63, true)));
    BOOST_CHECK_EQUAL("11000000", (itoa_bits<uint64_t,true, 2>(0xc0ul << 56, true)));

    BOOST_CHECK_EQUAL("11000000-00010111", (itoa_bits<uint64_t,true, 2>(0xc017ul << 48, true)));

    BOOST_CHECK_EQUAL(
        "00000010-00000000-00000000-00000000-00000000-00000000-00000000-00001000",
        (itoa_bits<uint64_t,true>(1ul << 57 | 1ul << 3)));

    BOOST_CHECK_EQUAL("0xAB00000000000000",(itoa_bits<uint64_t,true,1>(0xab00ul << 48)));
    BOOST_CHECK_EQUAL("0xAB00000000000000",(itoa_bits<uint64_t,true,-1>(0xab00ul << 48, true)));
    BOOST_CHECK_EQUAL("10101011",          (itoa_bits<uint64_t,true,1>(0xab00ul << 48, true)));
    BOOST_CHECK_EQUAL("0xABCD",            (itoa_bits<uint64_t,true,1>(0xabcd)));
    BOOST_CHECK_EQUAL("0xABCDEF1234",      (itoa_bits<uint64_t,true,1>(0xabcdef1234ul)));
    BOOST_CHECK_EQUAL("0xABCDEF1234",      (itoa_bits<uint64_t,true,2>(0xabcdef1234ul)));
    BOOST_CHECK_EQUAL("0xABCDEF1234",      (itoa_bits<uint64_t,true,7>(0xabcdef1234ul)));

    //------

    BOOST_CHECK_EQUAL("10000000",          (itoa_bits<uint64_t,false, 2>(1ul << 7, true)));
    BOOST_CHECK_EQUAL("11000000",          (itoa_bits<uint64_t,false, 2>(0xc0ul,   true)));

    BOOST_CHECK_EQUAL("11000000-00010111", (itoa_bits<uint64_t,false, 2>(0xc017, true)));

    BOOST_CHECK_EQUAL(
        "00000010-00000000-00000000-00000000-00000000-00000000-00000000-00001000",
        (itoa_bits<uint64_t,false>(1ul << 57 | 1ul << 3)));

    BOOST_CHECK_EQUAL("0xAB",              (itoa_bits<uint64_t,false,1>(0xab)));
    BOOST_CHECK_EQUAL("10101011",          (itoa_bits<uint64_t,false,1>(0xab, true)));
    BOOST_CHECK_EQUAL("0xABCD",            (itoa_bits<uint64_t,false,1>(0xabcd)));
    BOOST_CHECK_EQUAL("0xAB00",            (itoa_bits<uint64_t,false,-1>(0xab00ul, true)));
    BOOST_CHECK_EQUAL("0xABCDEF1234",      (itoa_bits<uint64_t,false,1>(0xabcdef1234ul)));
    BOOST_CHECK_EQUAL("0xABCDEF1234",      (itoa_bits<uint64_t,false,2>(0xabcdef1234ul)));
    BOOST_CHECK_EQUAL("0xABCDEF1234",      (itoa_bits<uint64_t,false,7>(0xabcdef1234ul)));
}


/* For internal use by sys_double_to_chars_fast() */
static char* find_first_trailing_zero(char* p)
{
    for (; *(p-1) == '0'; --p);
    if (*(p-1) == '.') ++p;
    return p;
}

/* Convert float to string
 *   decimals must be >= 0
 *   if compact != 0, the trailing 0's will be truncated
 */
int
sys_double_to_chars_fast(double f, char *buffer, int buffer_size, int decimals,
			 int compact)
{
    static const double pow10v[] = {
        1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8, 1e9,
        1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18
    };

    static constexpr const int    FRAC_SIZE    = 52;
    static constexpr const size_t MAX_DECIMALS = (sizeof(pow10v) / sizeof(pow10v[0]));
    static constexpr const size_t MAX_FLOAT    = ((size_t)1 << (FRAC_SIZE+1));

    double af;
    size_t int_part, frac_part;
    int neg;
    char *p = buffer;

    if (decimals < 0)
        return -1;

    if (f < 0) {
        neg = 1;
        af = -f;
    }
    else {
        neg = 0;
        af = f;
    }

    /* Don't bother with optimizing too large numbers or too large precision */
    if (af > MAX_FLOAT || decimals >= long(MAX_DECIMALS)) {
        int len = snprintf(buffer, buffer_size, "%.*f", decimals, f);
        char* p = buffer + len;
        if (len >= buffer_size)
            return -1;
        /* Delete trailing zeroes */
        if (compact)
            p = find_first_trailing_zero(p);
        *p = '\0';
        return p - buffer;
    }

    if (decimals) {
        double int_f = floor(af);
        double frac_f = round((af - int_f) * pow10v[decimals]);

        int_part = (size_t)int_f;
        frac_part = (size_t)frac_f;

        if (frac_f >= pow10v[decimals]) {
            /* rounding overflow carry into int_part */
            int_part++;
            frac_part = 0;
        }

        do {
            if (!frac_part) {
                do {
                    *p++ = '0';
                } while (--decimals);
                break;
            }
            *p++ = (char)((frac_part % 10) + '0');
            frac_part /= 10;
        } while (--decimals);

        *p++ = '.';
    }
    else
        int_part = (size_t)lround(af);

    if (!int_part) {
        *p++ = '0';
    } else {
        do {
            *p++ = (char)((int_part % 10) + '0');
            int_part /= 10;
        }while (int_part);
    }
    if (neg)
        *p++ = '-';

    {/* Reverse string */
        int i = 0;
        int j = p - buffer - 1;
        for ( ; i < j; i++, j--) {
            char tmp = buffer[i];
            buffer[i] = buffer[j];
            buffer[j] = tmp;
        }
    }

    /* Delete trailing zeroes */
    if (compact)
        p = find_first_trailing_zero(p);
    *p = '\0';
    return p - buffer;
}

BOOST_AUTO_TEST_CASE( test_convert_ftoa2 )
{
    char buf[32];
    int n = sys_double_to_chars_fast(0.6, buf, sizeof(buf), 3, false);
    BOOST_CHECK_EQUAL("0.600", buf);
    BOOST_CHECK_EQUAL(5, n);

    n = sys_double_to_chars_fast(123.19, buf, sizeof(buf), 3, true);
    BOOST_CHECK_EQUAL("123.19", buf);
    BOOST_CHECK_EQUAL(6, n);

    n = sys_double_to_chars_fast(0.999, buf, sizeof(buf), 2, false);
    BOOST_CHECK_EQUAL("1.00", buf);
    BOOST_CHECK_EQUAL(4, n);

    // Note that 1.005 is really 1.0049999999...
    n = sys_double_to_chars_fast(1.005, buf, sizeof(buf), 2, false);
    BOOST_CHECK_EQUAL("1.00", buf);
    BOOST_CHECK_EQUAL(4, n);

    // Note that 1.005 is really 1.0049999999...
    n = sys_double_to_chars_fast(1.005, buf, sizeof(buf), 2, true);
    BOOST_CHECK_EQUAL("1.0", buf);
    BOOST_CHECK_EQUAL(3, n);

    n = sys_double_to_chars_fast(-1.005, buf, sizeof(buf), 2, false);
    BOOST_CHECK_EQUAL("-1.00", buf);
    BOOST_CHECK_EQUAL(5, n);

    n = sys_double_to_chars_fast(-1.005, buf, sizeof(buf), 2, true);
    BOOST_CHECK_EQUAL("-1.0", buf);
    BOOST_CHECK_EQUAL(4, n);

    n = sys_double_to_chars_fast(0.145, buf, sizeof(buf), 1, true);
    BOOST_CHECK_EQUAL("0.1", buf);
    BOOST_CHECK_EQUAL(3, n);

    n = sys_double_to_chars_fast(-1.0, buf, sizeof(buf), 20, false);
    BOOST_CHECK_EQUAL("-1.00000000000000000000", buf);
    BOOST_CHECK_EQUAL(23, n);

    union {double d; int64_t i;} f;
    f.i = 0x7ff0000000000000ll;
    n = sys_double_to_chars_fast(f.d, buf, sizeof(buf), 29, true);
    BOOST_CHECK_EQUAL("inf", buf);
    BOOST_CHECK_EQUAL(3, n);

    f.i = 0xfff0000000000000ll;
    n = sys_double_to_chars_fast(f.d, buf, sizeof(buf), 29, true);
    BOOST_CHECK_EQUAL("-inf", buf);
    BOOST_CHECK_EQUAL(4, n);

    f.i = 0x7ff0000000000001ll;
    n = sys_double_to_chars_fast(f.d, buf, sizeof(buf), 29, true);
    BOOST_CHECK_EQUAL("nan", buf);
    BOOST_CHECK_EQUAL(3, n);

    f.i = 0xfff0000000000001ll;
    n = sys_double_to_chars_fast(f.d, buf, sizeof(buf), 29, true);
    BOOST_CHECK_EQUAL("-nan", buf);
    BOOST_CHECK_EQUAL(4, n);

    n = sys_double_to_chars_fast(1.0, buf, sizeof(buf), 29, true);
    BOOST_CHECK_EQUAL("1.0", buf);
    BOOST_CHECK_EQUAL(3, n);

    n = sys_double_to_chars_fast(1.0, buf, sizeof(buf), 30, true);
    BOOST_CHECK_EQUAL(-1, n);

    using boost::timer::cpu_timer;
    using boost::timer::cpu_times;
    using boost::timer::nanosecond_type;

    const long ITERATIONS = getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 1000000;
    {
        std::vector<double> data;
        char buf1[256], buf2[256], buf3[256];

        for (int i=0; i < 100000; ++i)
            data.push_back(double(rand()) / (double(RAND_MAX)/100000));
        
        {
            cpu_timer t;
            for (int i=0, j=-1; i < ITERATIONS; i++) {
                if (++j == data.size()) j = 0;
                auto n1 = sys_double_to_chars_fast(data[j], buf1, sizeof(buf1), 10, true);
            }
            cpu_times elapsed_times(t.elapsed());
            nanosecond_type t1 = elapsed_times.system + elapsed_times.user;

            std::stringstream s;
            s << boost::format("         new: %.3fs (%.3fus/call)")
                % ((double)t1 / 1000000000.0) % ((double)t1 / ITERATIONS / 1000.0);
            BOOST_TEST_MESSAGE(s.str());
        }
        {
            cpu_timer t;
            for (int i=0, j=-1; i < ITERATIONS; i++) {
                if (++j == data.size()) j = 0;
                auto n1 = ftoa_left(data[j], buf2, sizeof(buf2), 10, true);
            }
            cpu_times elapsed_times(t.elapsed());
            nanosecond_type t1 = elapsed_times.system + elapsed_times.user;

            std::stringstream s;
            s << boost::format("         old: %.3fs (%.3fus/call)")
                % ((double)t1 / 1000000000.0) % ((double)t1 / ITERATIONS / 1000.0);
            BOOST_TEST_MESSAGE(s.str());
        }
        {
            cpu_timer t;
            for (int i=0, j=-1; i < ITERATIONS; i++) {
                if (++j == data.size()) j = 0;
                auto n1 = snprintf(buf3, sizeof(buf3), "%.*f", 10, data[j]);
            }
            cpu_times elapsed_times(t.elapsed());
            nanosecond_type t1 = elapsed_times.system + elapsed_times.user;

            std::stringstream s;
            s << boost::format("         prn: %.3fs (%.3fus/call)")
                % ((double)t1 / 1000000000.0) % ((double)t1 / ITERATIONS / 1000.0);
            BOOST_TEST_MESSAGE(s.str());
        }

        BOOST_CHECK_EQUAL(buf1, buf2);
        BOOST_CHECK_EQUAL(buf1, buf3);
        BOOST_CHECK_EQUAL(buf2, buf3);
    }
}
