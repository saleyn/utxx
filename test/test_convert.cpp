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

//#define BOOST_TEST_MODULE test_convert
#include <boost/test/unit_test.hpp>
#include <util/convert.hpp>
#include <util/verbosity.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/timer/timer.hpp>
#include <limits>

using namespace util;


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

BOOST_AUTO_TEST_CASE( test_convert_fast_atoi )
{
    long n;
    BOOST_REQUIRE(fast_atoi( "123456989012345678", n));
    BOOST_REQUIRE_EQUAL( 123456989012345678, n);

    BOOST_REQUIRE(fast_atoi( "-123456989012345678", n));
    BOOST_REQUIRE_EQUAL(-123456989012345678, n);

    BOOST_REQUIRE(!fast_atoi( "        123", n));
    BOOST_REQUIRE(fast_atoi_skip_ws( "        123", n));
    BOOST_REQUIRE_EQUAL(123, n);

    BOOST_REQUIRE(!fast_atoi("123ABC", n));
    BOOST_REQUIRE(fast_atoi( "123ABC", n, false));
    BOOST_REQUIRE_EQUAL(123, n);
    
    BOOST_REQUIRE(!fast_atoi( "123  ", n));
    BOOST_REQUIRE(fast_atoi( "123  ", n, false));
    BOOST_REQUIRE_EQUAL(123, n);
    
    BOOST_REQUIRE(!fast_atoi( std::string("\0\0\0\000123", 7), n));
    BOOST_REQUIRE(fast_atoi_skip_ws( std::string("\0\0\0\000123", 7), n));
    BOOST_REQUIRE_EQUAL(123, n);

    BOOST_REQUIRE(!fast_atoi( "        -123", n));
    BOOST_REQUIRE(fast_atoi_skip_ws( "        -123", n));
    BOOST_REQUIRE_EQUAL(-123, n);

    BOOST_REQUIRE(!fast_atoi( "-123ABC", n));
    BOOST_REQUIRE(fast_atoi( "-123ABC", n, false));
    BOOST_REQUIRE_EQUAL(-123, n);
    
    BOOST_REQUIRE(!fast_atoi( "-123  ", n));
    BOOST_REQUIRE(fast_atoi( "-123  ", n, false));
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

        if (verbosity::level() > util::VERBOSE_NONE) {
            printf("    fast_atoi  iterations: %ld\n", ITERATIONS);
            printf("    fast_atoi  time      : %.3fs (%.3fus/call)\n",
                (double)t1 / 1000000000.0, (double)t1 / ITERATIONS / 1000.0);
        }
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

        if (verbosity::level() > util::VERBOSE_NONE) {
            printf("          atoi iterations: %ld\n", ITERATIONS);
            printf("          atoi time      : %.3fs (%.3fus/call)\n",
                (double)t1 / 1000000000.0, (double)t1 / ITERATIONS / 1000.0);
        }
    }

    /*
    {
        cpu_timer t;
        const char buf[] = "1234567890";
        long n;

        for (int i = 0; i < ITERATIONS; i++)
            unsafe_atoi<long, sizeof(buf)>(buf, n);

        cpu_times elapsed_times(t.elapsed());
        nanosecond_type t1 = elapsed_times.system + elapsed_times.user;

        if (verbosity::level() > util::VERBOSE_NONE) {
            printf("   unsafe_atoi iterations: %ld (n = %ld)\n", ITERATIONS, n);
            printf("          atoi time      : %.3fs (%.3fus/call)\n",
                (double)t1 / 1000000000.0, (double)t1 / ITERATIONS / 1000.0);
        }
    }
    */
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

BOOST_AUTO_TEST_CASE( test_convert_ftoa_right )
{
    {
        char buf[20];
        int n = ftoa_fast(0.6, buf, sizeof(buf), 3, false);
        BOOST_REQUIRE_EQUAL("0.600", buf);
        BOOST_REQUIRE_EQUAL(5, n);

        n = ftoa_fast(123.19, buf, sizeof(buf), 3, true);
        BOOST_REQUIRE_EQUAL("123.19", buf);
        BOOST_REQUIRE_EQUAL(6, n);
    }
}

BOOST_AUTO_TEST_CASE( test_convert_itoa_right_string )
{
    BOOST_REQUIRE_EQUAL("0001", (itoa_right<int, 4>(1, '0')));
    BOOST_REQUIRE_EQUAL("1", (itoa_right<int, 10>(1)));
}
