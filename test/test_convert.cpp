//----------------------------------------------------------------------------
/// \file  test_raw_char.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for raw_char class.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-30
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the REPLOG project.

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
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_REQUIRE_EQUAL(0,  n);
        BOOST_REQUIRE_EQUAL(0,  m);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_REQUIRE_EQUAL(-1, rp-buf);

        char lout[sizeof(buf)], rout[sizeof(buf)];
        lp = itoa_left(lout, n);
        rp = itoa_right(rout, m);
        BOOST_REQUIRE_EQUAL(std::string(buf, sizeof(buf)), std::string(lout, sizeof(lout)));
        BOOST_REQUIRE_EQUAL(std::string(buf, sizeof(buf)), std::string(rout, sizeof(rout)));
        BOOST_REQUIRE_EQUAL(0,  n);
        BOOST_REQUIRE_EQUAL(0,  m);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-lout);
        BOOST_REQUIRE_EQUAL(-1, rp-rout);
    }
    {
        char buf[] = {'-','0'};
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_REQUIRE_EQUAL(0,  n);
        BOOST_REQUIRE_EQUAL(0,  m);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_REQUIRE_EQUAL(-1, rp-buf);

        char lout[sizeof(buf)], rout[sizeof(buf)];
        lp = itoa_left(lout, n);
        rp = itoa_right(rout, m);
        BOOST_REQUIRE_EQUAL(std::string(buf+1, sizeof(buf)-1), std::string(lout, sizeof(lout)-1));
        BOOST_REQUIRE_EQUAL(std::string(buf+1, sizeof(buf)-1), std::string(rout+1, sizeof(rout)-1));
        BOOST_REQUIRE_EQUAL(0,  n);
        BOOST_REQUIRE_EQUAL(0,  m);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf)-1, lp-lout);
        BOOST_REQUIRE_EQUAL(0, rp-rout);
    }
    {
        char buf[] = {'1'};
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_REQUIRE_EQUAL(1,  n);
        BOOST_REQUIRE_EQUAL(1,  m);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_REQUIRE_EQUAL(-1, rp-buf);

        char lout[sizeof(buf)], rout[sizeof(buf)];
        lp = itoa_left(lout, n);
        rp = itoa_right(rout, m);
        BOOST_REQUIRE_EQUAL(std::string(buf, sizeof(buf)), std::string(lout, sizeof(lout)));
        BOOST_REQUIRE_EQUAL(std::string(buf, sizeof(buf)), std::string(rout, sizeof(rout)));
        BOOST_REQUIRE_EQUAL(1,  n);
        BOOST_REQUIRE_EQUAL(1,  m);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-lout);
        BOOST_REQUIRE_EQUAL(-1, rp-rout);
    }
    {
        char buf[] = {'\0','-','1'};
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_REQUIRE_EQUAL(0,  n);
        BOOST_REQUIRE_EQUAL(-1, m);
        BOOST_REQUIRE_EQUAL(0, lp-buf);
        BOOST_REQUIRE_EQUAL(0, rp-buf);

        char lout[sizeof(buf)], rout[sizeof(buf)];
        lp = itoa_left(lout, -1);
        rp = itoa_right(rout, -1);
        BOOST_REQUIRE_EQUAL('\0', lout[2]);
        BOOST_REQUIRE_EQUAL(std::string(buf+1, sizeof(buf)-1), lout);
        BOOST_REQUIRE_EQUAL(std::string(buf+1, sizeof(buf)-1), std::string(rout+1, sizeof(rout)-1));
        BOOST_REQUIRE_EQUAL((int)sizeof(buf)-1, lp-lout);
        BOOST_REQUIRE_EQUAL(0,  rp-rout);

        lp = itoa_left(lout, -1,  ' ');
        rp = itoa_right(rout, -1, ' ');
        BOOST_REQUIRE_EQUAL("-1 ", std::string(lout, 3));
        BOOST_REQUIRE_EQUAL(" -1", std::string(rout, 3));
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-lout);
        BOOST_REQUIRE_EQUAL(-1, rp-rout);
    }
    {
        char buf[] = {'1', '2', '3', '4', '5'};
        BOOST_STATIC_ASSERT((int)sizeof(buf) == 5);
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_REQUIRE_EQUAL(12345, n);
        BOOST_REQUIRE_EQUAL(12345, m);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_REQUIRE_EQUAL(-1, rp-buf);

        char lout[sizeof(buf)], rout[sizeof(buf)];
        lp = itoa_left(lout, n);
        rp = itoa_right(rout, m);
        BOOST_REQUIRE_EQUAL(std::string(buf, sizeof(buf)), std::string(lout, sizeof(lout)));
        BOOST_REQUIRE_EQUAL(std::string(buf, sizeof(buf)), std::string(rout, sizeof(rout)));
        BOOST_REQUIRE_EQUAL(12345, n);
        BOOST_REQUIRE_EQUAL(12345, m);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-lout);
        BOOST_REQUIRE_EQUAL(-1,    rp-rout);
    }
    {
        char buf[] = {'-', '2', '3', '4', '5'};
        BOOST_STATIC_ASSERT(sizeof(buf) == 5);
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_REQUIRE_EQUAL(-2345, n);
        BOOST_REQUIRE_EQUAL(-2345, m);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_REQUIRE_EQUAL(-1, rp-buf);

        char lout[sizeof(buf)], rout[sizeof(buf)];
        lp = itoa_left(lout, n);
        rp = itoa_right(rout, m);
        BOOST_REQUIRE_EQUAL(std::string(buf, sizeof(buf)), std::string(lout, sizeof(lout)));
        BOOST_REQUIRE_EQUAL(std::string(buf, sizeof(buf)), std::string(rout, sizeof(rout)));
        BOOST_REQUIRE_EQUAL(-2345, n);
        BOOST_REQUIRE_EQUAL(-2345, m);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-lout);
        BOOST_REQUIRE_EQUAL(-1,    rp-rout);
    }
    {
        char buf[] = {' ', ' ', '1', '2', '3', '4', '5'};
        BOOST_STATIC_ASSERT(sizeof(buf) == 7);
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_REQUIRE_EQUAL(0, n);
        BOOST_REQUIRE_EQUAL(12345, m);
        BOOST_REQUIRE_EQUAL(0, lp-buf);
        BOOST_REQUIRE_EQUAL(1, rp-buf);

        char lbuf[] = {'1', '2', '3', '4', '5', ' ', ' '};
        char lout[sizeof(buf)], rout[sizeof(buf)] = {};
        lp = itoa_left(lout, 12345);
        rp = itoa_right(rout, 12345);
        BOOST_REQUIRE_EQUAL(std::string(lbuf, sizeof(lbuf)-2), std::string(lout, sizeof(lout)-2));
        BOOST_REQUIRE_EQUAL(std::string(buf+2, sizeof(buf)-2), std::string(rout+2, sizeof(rout)-2));
        BOOST_REQUIRE_EQUAL((int)sizeof(lbuf)-2, lp-lout);
        BOOST_REQUIRE_EQUAL(1,    rp-rout);

        bzero(lout, sizeof(lout));
        bzero(rout, sizeof(rout));
        lp = itoa_left(lout, 12345, ' ');
        rp = itoa_right(rout, 12345, ' ');
        BOOST_REQUIRE_EQUAL(std::string(lbuf, sizeof(lbuf)), std::string(lout, sizeof(lout)));
        BOOST_REQUIRE_EQUAL(std::string(buf, sizeof(buf)), std::string(rout, sizeof(rout)));
        BOOST_REQUIRE_EQUAL((int)sizeof(lbuf), lp-lout);
        BOOST_REQUIRE_EQUAL(-1,    rp-rout);
    }
    {
        char buf[] = {' ', '-', '1', '2', '3', '4', '5'};
        BOOST_STATIC_ASSERT((int)sizeof(buf) == 7);
        int n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_REQUIRE_EQUAL(0, n);      // Error
        BOOST_REQUIRE_EQUAL(-12345, m); // Success
        BOOST_REQUIRE_EQUAL(0, lp-buf); // nothing was parsed - pointing to the beginning of the buffer.
        BOOST_REQUIRE_EQUAL(0, rp-buf); // points to buf[0] - parsing was done from right to left.

        char lbuf[] = {'-', '1', '2', '3', '4', '5', 'X'};
        char lout[sizeof(buf)], rout[sizeof(buf)] = {};
        lp = itoa_left(lout, -12345);
        rp = itoa_right(rout, -12345);
        BOOST_REQUIRE_EQUAL(std::string(lbuf, sizeof(lbuf)-1), std::string(lout, sizeof(lout)-1));
        BOOST_REQUIRE_EQUAL(std::string(buf+1, sizeof(buf)-1), std::string(rout+1, sizeof(rout)-1));
        BOOST_REQUIRE_EQUAL((int)sizeof(lbuf)-1, lp-lout);
        BOOST_REQUIRE_EQUAL(0,    rp-rout);
    }
    {
        long expect = -1053806107;
        char buf[]  = {'-','1','0','5','3','8','0','6','1','0','7'};
        long n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_REQUIRE_EQUAL(expect, n);
        BOOST_REQUIRE_EQUAL(expect, m);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_REQUIRE_EQUAL(-1, rp-buf);
    }
    {
        char buf[]  = {'-',' ',' ','1','0','5'};
        long n, m;
        const char* lp = atoi_left(buf, n);
        const char* rp = atoi_right(buf, m);
        BOOST_REQUIRE_EQUAL(0,   n); // This is really invalid input
        BOOST_REQUIRE_EQUAL(105, m); // Invalid input
        BOOST_REQUIRE_EQUAL(1,  lp-buf);
        BOOST_REQUIRE_EQUAL(2,  rp-buf);
    }
    {
        char buf[19];
        int64_t n = std::numeric_limits<int64_t>::max();
        std::stringstream s; s << n;
        strncpy(buf, s.str().c_str(), s.str().size());
        BOOST_REQUIRE_EQUAL(sizeof(buf), s.str().size());
        int64_t k = boost::lexical_cast<int64_t>(s.str());
        int64_t i, j;
        BOOST_REQUIRE_EQUAL(n, k);
        const char* lp = atoi_left(buf, i);
        const char* rp = atoi_right(buf, j);
        BOOST_REQUIRE_EQUAL(n, i);
        BOOST_REQUIRE_EQUAL(n, j);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_REQUIRE_EQUAL(-1, rp-buf);
        //BOOST_REQUIRE_EQUAL(0,    rp-rout);
    }
    {
        char buf[20];
        int64_t n = std::numeric_limits<int64_t>::min();
        BOOST_REQUIRE(n < 0);
        std::stringstream s; s << n;
        strncpy(buf, s.str().c_str(), s.str().size());
        BOOST_REQUIRE_EQUAL(sizeof(buf), s.str().size());
        int64_t k = boost::lexical_cast<int64_t>(s.str());
        int64_t i, j;
        BOOST_REQUIRE_EQUAL(n, k);
        const char* lp = atoi_left(buf, i);
        const char* rp = atoi_right(buf, j);
        BOOST_REQUIRE_EQUAL(n, i);
        BOOST_REQUIRE_EQUAL(n, j);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_REQUIRE_EQUAL(-1, rp-buf);
    }
    {
        char buf[20];
        uint64_t n = std::numeric_limits<uint64_t>::max();
        uint64_t m = std::numeric_limits<uint64_t>::min();
        BOOST_REQUIRE(n > 0);
        BOOST_REQUIRE(m == 0);
        std::stringstream s; s << n;
        strncpy(buf, s.str().c_str(), s.str().size());
        BOOST_REQUIRE_EQUAL(sizeof(buf), s.str().size());
        uint64_t k = boost::lexical_cast<uint64_t>(s.str());
        uint64_t i, j;
        BOOST_REQUIRE_EQUAL(n, k);
        const char* lp = atoi_left(buf, i);
        const char* rp = atoi_right(buf, j);
        BOOST_REQUIRE_EQUAL(n, i);
        BOOST_REQUIRE_EQUAL(n, j);
        BOOST_REQUIRE_EQUAL((int)sizeof(buf), lp-buf);
        BOOST_REQUIRE_EQUAL(-1, rp-buf);
    }
    {
        char buf[10];
        char* p = buf;
        itoa<int>(0, p);
        BOOST_REQUIRE_EQUAL("0", buf);
        BOOST_REQUIRE_EQUAL(1, p - buf);
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
        BOOST_REQUIRE_EQUAL(n, m);
        const char* p = itoa_left(test, n);
        m = p-test;
        BOOST_REQUIRE_EQUAL((int64_t)strlen(buf), m);
        BOOST_REQUIRE_EQUAL(buf, std::string(test, m));
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
        BOOST_REQUIRE_EQUAL(tests[i].fatoi_success,
                            fast_atoi(tests[i].test, n));
        if (tests[i].fatoi_success) BOOST_REQUIRE_EQUAL(tests[i].expected, n);

        const char* e = fun_signed[tests[i].len](tests[i].test, n);
        bool       ok = !tests[i].expected || e == (tests[i].test + tests[i].len);

        if (tests[i].expected != n || !ok)
            BOOST_TEST_MESSAGE("Test signed #" << i << " failed");
        BOOST_REQUIRE_EQUAL(tests[i].expected, n);
        BOOST_REQUIRE(ok);

        if (tests[i].expected >= 0) {
            e  = fun_unsigned[tests[i].len](tests[i].test, u);
            ok = !tests[i].expected || e == (tests[i].test + tests[i].len);

            if (tests[i].expected != (long)u || !ok)
                BOOST_TEST_MESSAGE("Test unsigned #" << i << " failed");
            BOOST_REQUIRE_EQUAL(tests[i].expected, (long)u);
            BOOST_REQUIRE(ok);
        }

    }
}

BOOST_AUTO_TEST_CASE( test_convert_fast_atoi )
{
    long n;

    BOOST_REQUIRE(!fast_atoi("123ABC", n));
    BOOST_REQUIRE((fast_atoi<long,false>( "123ABC", n)));
    BOOST_REQUIRE_EQUAL(123, n);

    BOOST_REQUIRE(!fast_atoi( "123  ", n));
    BOOST_REQUIRE((fast_atoi<long,false>( "123  ",  n)));
    BOOST_REQUIRE_EQUAL(123, n);

    BOOST_REQUIRE(!fast_atoi( std::string("\0\0\0\000123", 7), n));
    BOOST_REQUIRE(fast_atoi_skip_ws( std::string("\0\0\0\000123", 7), n));
    BOOST_REQUIRE_EQUAL(123, n);

    BOOST_REQUIRE(!fast_atoi( "        -123", n));
    BOOST_REQUIRE(fast_atoi_skip_ws( "        -123", n));
    BOOST_REQUIRE_EQUAL(-123, n);

    BOOST_REQUIRE(!fast_atoi( "-123ABC", n));
    BOOST_REQUIRE((fast_atoi<long,false>( "-123ABC",  n)));
    BOOST_REQUIRE_EQUAL(-123, n);

    BOOST_REQUIRE(!fast_atoi( "-123  ",  n));
    BOOST_REQUIRE((fast_atoi<long,false>( "-123  ",   n)));
    BOOST_REQUIRE_EQUAL(-123, n);

    BOOST_REQUIRE(!fast_atoi( std::string("\0\0\0\000-123", 8), n));
    BOOST_REQUIRE(fast_atoi_skip_ws( std::string("\0\0\0\000-123", 8), n));
    BOOST_REQUIRE_EQUAL(-123, n);
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
        BOOST_REQUIRE(fast_atoi(buf, n));
        BOOST_REQUIRE_EQUAL(1234567890, n);

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
        BOOST_REQUIRE_EQUAL(1234567890, n);

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
        BOOST_REQUIRE_EQUAL(1234567890, atoi(buf.c_str()));

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
    BOOST_REQUIRE_EQUAL(2345, n);
    BOOST_REQUIRE_EQUAL(12345, m);

    m = 0;
    atoi_right<long, 5>("12345", m, '5');
    BOOST_REQUIRE_EQUAL(1234, m);

    n = m = 0;
    atoi_left<long, 7> ("12345  ", n, ' ');
    atoi_right<long, 7>("  12345", m, ' ');
    BOOST_REQUIRE_EQUAL(12345, n);
    BOOST_REQUIRE_EQUAL(12345, m);

    n = m = 0;
    atoi_left<long, 9> ("  12345  ", n, ' ');
    atoi_right<long, 9>("  12345  ", m, ' ');
    BOOST_REQUIRE_EQUAL(12345, n);
    BOOST_REQUIRE_EQUAL(12345, m);

    n = m = 0;
    atoi_left<long, 7>("0012345", n, '0');
    atoi_right<long, 7>("1234500", m, '0');
    BOOST_REQUIRE_EQUAL(12345, n);
    BOOST_REQUIRE_EQUAL(12345, m);
}

BOOST_AUTO_TEST_CASE( test_convert_ftoa )
{
    char buf[32];
    int n = ftoa_left(0.6, buf, sizeof(buf), 3, false);
    BOOST_REQUIRE_EQUAL("0.600", buf);
    BOOST_REQUIRE_EQUAL(5, n);

    n = ftoa_left(123.19, buf, sizeof(buf), 3, true);
    BOOST_REQUIRE_EQUAL("123.19", buf);
    BOOST_REQUIRE_EQUAL(6, n);

    n = ftoa_left(0.999, buf, sizeof(buf), 2, false);
    BOOST_REQUIRE_EQUAL("1.00", buf);
    BOOST_REQUIRE_EQUAL(4, n);

    n = ftoa_left(1.005, buf, sizeof(buf), 2, false);
    BOOST_REQUIRE_EQUAL("1.01", buf);
    BOOST_REQUIRE_EQUAL(4, n);

    n = ftoa_left(1.005, buf, sizeof(buf), 2, true);
    BOOST_REQUIRE_EQUAL("1.01", buf);
    BOOST_REQUIRE_EQUAL(4, n);

    n = ftoa_left(-1.0, buf, sizeof(buf), 20, false);
    BOOST_REQUIRE_EQUAL("-1.00000000000000000000", buf);
    BOOST_REQUIRE_EQUAL(23, n);

    union {double d; int64_t i;} f;
    f.i = 0x7ff0000000000000ll;
    n = ftoa_left(f.d, buf, sizeof(buf), 29, true);
    BOOST_REQUIRE_EQUAL("inf", buf);
    BOOST_REQUIRE_EQUAL(3, n);

    f.i = 0xfff0000000000000ll;
    n = ftoa_left(f.d, buf, sizeof(buf), 29, true);
    BOOST_REQUIRE_EQUAL("-inf", buf);
    BOOST_REQUIRE_EQUAL(4, n);

    f.i = 0x7ff0000000000001ll;
    n = ftoa_left(f.d, buf, sizeof(buf), 29, true);
    BOOST_REQUIRE_EQUAL("nan", buf);
    BOOST_REQUIRE_EQUAL(3, n);

    f.i = 0xfff0000000000001ll; /* The left-most sign bif doesn't matter for nan */
    n = ftoa_left(f.d, buf, sizeof(buf), 29, true);
    BOOST_REQUIRE_EQUAL("nan", buf);
    BOOST_REQUIRE_EQUAL(3, n);

    n = ftoa_left(1.0, buf, sizeof(buf), 29, true);
    BOOST_REQUIRE_EQUAL("1.0", buf);
    BOOST_REQUIRE_EQUAL(3, n);

    n = ftoa_left(1.0, buf, sizeof(buf), 30, true);
    BOOST_REQUIRE_EQUAL(-1, n);

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
    BOOST_REQUIRE_EQUAL("0001", (itoa_right<int, 4>(1, '0')));
    BOOST_REQUIRE_EQUAL("0000", (itoa_right<int, 4>(0, '0')));
    BOOST_REQUIRE_EQUAL("   0", (itoa_right<int, 4>(0, ' ')));
    BOOST_REQUIRE_EQUAL("1",    (itoa_right<int, 10>(1)));
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
