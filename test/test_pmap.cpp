//----------------------------------------------------------------------------
/// \file  test_bitmap.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating bitmap.hpp functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-12-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

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
//#define BOOST_TEST_MODULE bitmap_test 

#include <boost/test/unit_test.hpp>
#include <utxx/bits.hpp>
#include <utxx/string.hpp>
#include <limits>
#include <stdio.h>
#include <utxx/verbosity.hpp>
#include <utxx/time_val.hpp>

using namespace utxx;

static const unsigned char* test_set[] = {
    "\x20",
    "\x81",
    "\x82\x41",
    "\x83\x68\x7F",
    "\x84\x6F\xAC\xEF",
    "\x85\xEf\xDe\xCd\xBc",
    "\x86\xEf\xDe\xCd\xBc\xAf",
    "\x87\xEf\xDe\xCd\xBc\xAf\x9f",
    "\x88\xEf\xDe\xCd\xBc\xAf\x9f\x8f"
};

static void test1(bool output) {
    const uint64_t mask = 0x8080808080808080;

    for (int i=0; i < utxx::length(test_set); ++i) {
        const uint64_t* v = (const uint64_t*)test_set[i];
        uint64_t stop = *v & mask;
        int      pos  = stop ? utxx::bits::bit_scan_reverse(stop) : -1;
        if (output)
            printf("%16lx -> %d\n", test_set[i], pos < 0 ? pos : (pos+1) >> 3);
    }
}

BOOST_AUTO_TEST_CASE( test_pmap )
{
    long ITERATIONS = getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 10000000;
    uint64_t value;

    boost::function<void(bool)> tests[] = { &test1, &test2 };

    for (int t=0; t < 2; t++)
    {
        time_val start = time_val::universal_time();

        tests[t](verbosity::level() >= VERBOSE_DEBUG);

        for (long i=0; i < ITERATIONS; ++i)
            tests[t](false);

        double elapsed = time_val::universal_time().now_diff(start);
        int speed = (double)ITERATIONS / elapsed;

        printf("Speed: %d it/s, elapsed: %.6fs\n", speed, elapsed);
    }
}

static void test2(bool output) {
}
/*
    //char acc[N];     // Accumulator

    for (int i=0; i < utxx::length(test_set); ++i) {
        int  n      = 0;
        bool done   = false;
        for (char const*  curr = buff; curr < end && n < N && !done; ++curr, ++n)
        {
            char  c = *curr;
            done    = (c & 0x80);     // Check the stop bit
            //acc[n]  = (c & 0x7f);     // Drop  the stop bit
        }
        if (output)
            printf("%16lx -> %d\n", test_set[i], pos < 0 ? pos : (pos+1) >> 3);
    }

  // How many entries have we got? Their bits will be stored in the HIGH part
  // of the "PMap", and the low part filled with 0s;   initial "shift" is the
  // bit size of the "0"s padding:
  //
  assert(n <= N);
  int    shift  = 64 - 7 * n;
  assert(shift >= 1);     // At least because 9 groups would use 63 bits!

  // Now store the "acc" bits in the reverse order, so that acc[0] would come
  // to the highest part of "pmap:
  //
  pmap  = 0;
  for (int i = n-1; i >= 0; --i, shift += 7)
  {
    uint64_t   tmp = acc[i];
    tmp    <<= shift;
    (*pmap) |= tmp;
  }
*/

