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
#include <utxx/bitmap.hpp>
#include <limits>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_bitmap_low )
{
    bitmap64 bm;

    bm.set(3);
    bm.set(57);

    for (unsigned int i=0; i <= bitmap64::max; ++i) {
        if (i == 3 || i == 57)
            BOOST_REQUIRE(bm.is_set(i));
        else
            BOOST_REQUIRE(!bm.is_set(i));
    }

    std::stringstream s;
    bm.print(s);
    BOOST_REQUIRE_EQUAL(
        "00000010-00000000-00000000-00000000-00000000-00000000-00000000-00001000",
        s.str());

    BOOST_REQUIRE_EQUAL(3,  bm.first());
    BOOST_REQUIRE_EQUAL(57, bm.last());
    BOOST_REQUIRE_EQUAL(3,  bm.next(0));
    BOOST_REQUIRE_EQUAL(57, bm.next(3));
    BOOST_REQUIRE_EQUAL(57, bm.next(4));

    BOOST_REQUIRE_EQUAL(57,             bm.prev(63)); // valid range [63..1]
    BOOST_REQUIRE_EQUAL((int)bm.end(),  bm.prev(1));  // valid range [63..0]
    BOOST_REQUIRE_EQUAL((int)bm.end(),  bm.next(62)); // valid range [0..62]

    BOOST_REQUIRE_EQUAL(57,             bm.prev(63));
    BOOST_REQUIRE_EQUAL(3,              bm.prev(57));
    BOOST_REQUIRE_EQUAL((int)bm.end(),  bm.prev(3));

    bm.fill();
    BOOST_REQUIRE_EQUAL(uint64_t(-1),   bm.value());
}

BOOST_AUTO_TEST_CASE( test_bitmap_bit_count )
{
    {
        uint32_t n;
        n = 1 << 31;    BOOST_REQUIRE_EQUAL(1, bitcount(n));
        n = 1 << 0;     BOOST_REQUIRE_EQUAL(1, bitcount(n));
        n = 0;          BOOST_REQUIRE_EQUAL(0, bitcount(n));
        n = 1 << 0
          | 1 << 5
          | 1 << 31;    BOOST_REQUIRE_EQUAL(3, bitcount(n));
        n = std::numeric_limits<uint32_t>::max();
                        BOOST_REQUIRE_EQUAL(32,bitcount(n));
    }
    {
        uint64_t n;
        n = 1ull << 31; BOOST_REQUIRE_EQUAL(1, bitcount(n));
        n = 1ull << 63; BOOST_REQUIRE_EQUAL(1, bitcount(n));
        n = 1ull << 0;  BOOST_REQUIRE_EQUAL(1, bitcount(n));
        n = 0;          BOOST_REQUIRE_EQUAL(0, bitcount(n));
        n = 1 << 0
          | 1ull << 5
          | 1ull << 56
          | 1ull << 63; BOOST_REQUIRE_EQUAL(4, bitcount(n));
        n = std::numeric_limits<uint64_t>::max();
                        BOOST_REQUIRE_EQUAL(64,bitcount(n));
    }
}

BOOST_AUTO_TEST_CASE( test_bitmap_low_boundary )
{
    bitmap64 bm;
    unsigned int max = bitmap64::max;

    BOOST_REQUIRE_EQUAL((int)bm.end(), bm.first());
    BOOST_REQUIRE_EQUAL((int)bm.end(), bm.last());

    bm.set(0);
    bm.set(max);

    BOOST_REQUIRE_EQUAL(0,   bm.first());
    BOOST_REQUIRE_EQUAL((int)max, bm.last());
    BOOST_REQUIRE_EQUAL((int)max, bm.next(0));
    BOOST_REQUIRE_EQUAL(0,   bm.prev(1));
    BOOST_REQUIRE_EQUAL((int)max, bm.next(max-1));
    BOOST_REQUIRE_EQUAL((int)bm.end(), bm.next(max));
    BOOST_REQUIRE_EQUAL((int)bm.end(), bm.prev(0));
}

BOOST_AUTO_TEST_CASE( test_bitmap_low_clear_all )
{
    bitmap64 bm;

    BOOST_REQUIRE(bm.empty());

    bm.set(0);
    bm.set(32);
    bm.set(63);
    bm.clear();

    BOOST_REQUIRE(bm.empty());
}

BOOST_AUTO_TEST_CASE( test_bitmap_high )
{
    bitmap4096 bm;

    int ls = bitmap4096::s_lo_dim;
    int hs = bitmap4096::s_hi_dim;
    BOOST_REQUIRE_EQUAL(64, ls);
    BOOST_REQUIRE_EQUAL(64, hs);

    static const unsigned int max = bitmap4096::max-5;
    bm.set(3);
    bm.set(max);

    BOOST_REQUIRE_EQUAL(2, bm.count());

    for (unsigned int i=0; i <= max; ++i) {
        if (i == 3 || i == max)
            BOOST_REQUIRE(bm.is_set(i));
        else
            BOOST_REQUIRE(!bm.is_set(i));
    }

    {
        static const char* expected = 
          "\n64: 0400000000000000-0000000000000000-0000000000000000-0000000000000000-"
                "0000000000000000-0000000000000000-0000000000000000-0000000000000000\n"
            "56: 0000000000000000-0000000000000000-0000000000000000-0000000000000000-"
                "0000000000000000-0000000000000000-0000000000000000-0000000000000000\n"
            "48: 0000000000000000-0000000000000000-0000000000000000-0000000000000000-"
                "0000000000000000-0000000000000000-0000000000000000-0000000000000000\n"
            "40: 0000000000000000-0000000000000000-0000000000000000-0000000000000000-"
                "0000000000000000-0000000000000000-0000000000000000-0000000000000000\n"
            "32: 0000000000000000-0000000000000000-0000000000000000-0000000000000000-"
                "0000000000000000-0000000000000000-0000000000000000-0000000000000000\n"
            "24: 0000000000000000-0000000000000000-0000000000000000-0000000000000000-"
                "0000000000000000-0000000000000000-0000000000000000-0000000000000000\n"
            "16: 0000000000000000-0000000000000000-0000000000000000-0000000000000000-"
                "0000000000000000-0000000000000000-0000000000000000-0000000000000000\n"
            "08: 0000000000000000-0000000000000000-0000000000000000-0000000000000000-"
                "0000000000000000-0000000000000000-0000000000000000-0000000000000008";
        std::stringstream s;
        bm.print(s);
        BOOST_REQUIRE_EQUAL(expected, s.str().c_str());
    }

    BOOST_REQUIRE_EQUAL(3,          bm.first());
    BOOST_REQUIRE_EQUAL((int)max,   bm.last());
    BOOST_REQUIRE_EQUAL(3,          bm.next(0));
    BOOST_REQUIRE_EQUAL((int)max,   bm.next(4));
    BOOST_REQUIRE_EQUAL((int)max,   bm.prev(max+5));
    BOOST_REQUIRE_EQUAL(3,          bm.prev(max));
    BOOST_REQUIRE_EQUAL((int)bm.end(),   bm.prev(3));
}

BOOST_AUTO_TEST_CASE( bitmap_high_boundary_test )
{
    bitmap4096 bm;
    int max = bitmap4096::max;

    BOOST_REQUIRE_EQUAL((int)bm.end(), bm.first());
    BOOST_REQUIRE_EQUAL((int)bm.end(), bm.last());

    bm.set(0);
    bm.set(max);

    BOOST_REQUIRE_EQUAL(0,   bm.first());
    BOOST_REQUIRE_EQUAL(max, bm.last());
    BOOST_REQUIRE_EQUAL(max, bm.next(0));
    BOOST_REQUIRE_EQUAL(0,   bm.prev(1));
    BOOST_REQUIRE_EQUAL(max, bm.next(max-1));
}

BOOST_AUTO_TEST_CASE( bitmap_high_boundary_test2 )
{
    bitmap4096 bm;

    bm.set(8);

    BOOST_REQUIRE_EQUAL((int)bm.end(), bm.next(8));
}

BOOST_AUTO_TEST_CASE( test_bitmap_high_clear_test )
{
    bitmap4096 bm;

    BOOST_REQUIRE(bm.empty());

    bm.set(0);

    BOOST_REQUIRE(!bm.empty());
    
    bm.set(63);
    bm.set(64);

    BOOST_REQUIRE_EQUAL(3, bm.count());

    bm.clear(64);
    bm.clear(63);
    bm.clear(0);

    BOOST_REQUIRE(bm.empty());
}

BOOST_AUTO_TEST_CASE( test_bitmap_high_clear_all_test )
{
    bitmap4096 bm;

    BOOST_REQUIRE(bm.empty());

    bm.set(0);
    bm.set(64);
    bm.set(1023);

    BOOST_REQUIRE_EQUAL(3, bm.count());

    bm.clear();

    BOOST_REQUIRE(bm.empty());
}

BOOST_AUTO_TEST_CASE( test_bitmap_clear )
{
    {
        bitmap32 bm;
        bm.set(0);
        BOOST_REQUIRE_EQUAL(1, bm.is_set(0));
        BOOST_REQUIRE_EQUAL(1, bm.count());
        bm.clear(5);
        BOOST_REQUIRE_EQUAL(1, bm.count());
        BOOST_REQUIRE_EQUAL(0, bm.first());
        BOOST_REQUIRE_EQUAL(1, bm.is_set(0));
        bm.clear(0);
        BOOST_REQUIRE_EQUAL(0,    bm.count());
        BOOST_REQUIRE_EQUAL((int)bm.end(), bm.first());
        BOOST_REQUIRE_EQUAL(0,    bm.is_set(0));
    }
    {
        bitmap4096 bm;
        bm.set(4095);
        BOOST_REQUIRE_EQUAL(1, bm.is_set(4095));
        BOOST_REQUIRE_EQUAL(1, bm.count());
        bm.clear(1024);
        BOOST_REQUIRE_EQUAL(1,    bm.count());
        BOOST_REQUIRE_EQUAL(4095, bm.first());
        BOOST_REQUIRE_EQUAL(1,    bm.is_set(4095));
        bm.clear(4095);
        BOOST_REQUIRE_EQUAL(0,    bm.count());
        BOOST_REQUIRE_EQUAL((int)bm.end(), bm.first());
        BOOST_REQUIRE_EQUAL(0,    bm.is_set(4095));
    }
}

BOOST_AUTO_TEST_CASE( test_bitmap_mid )
{
    typedef bitmap_high<129> bitmap129;
    bitmap129 bm;

    int ls  = bitmap129::s_lo_dim;
    int hs  = bitmap129::s_hi_dim;
    int max = bitmap129::max;
    BOOST_REQUIRE_EQUAL(64,  ls);
    BOOST_REQUIRE_EQUAL(3,   hs);
    BOOST_REQUIRE_EQUAL(128, max);
    BOOST_REQUIRE_EQUAL(129, (int)bm.end());
    // 1 long for bitmap_low + 3 longs for the m_data array in bitmap_high
    BOOST_REQUIRE_EQUAL((3+1)*sizeof(long), sizeof(bitmap129));

    max = bitmap129::max-5;
    bm.set(3);
    bm.set(max);

    BOOST_REQUIRE_EQUAL(2, bm.count());

    for (int i=0; i <= max; ++i) {
        if (i == 3 || i == max)
            BOOST_REQUIRE(bm.is_set(i));
        else
            BOOST_REQUIRE(!bm.is_set(i));
    }

    {
        static const char* expected =
          "\n03: 0000000000000000-0800000000000000-0000000000000008";
        std::stringstream s;
        bm.print(s);
        BOOST_REQUIRE_EQUAL(expected, s.str().c_str());
    }

    BOOST_REQUIRE_EQUAL(3,          bm.first());
    BOOST_REQUIRE_EQUAL(max,        bm.last());
    BOOST_REQUIRE_EQUAL(3,          bm.next(0));
    BOOST_REQUIRE_EQUAL(max,        bm.next(4));
    BOOST_REQUIRE_EQUAL(max,        bm.prev(max+5));
    BOOST_REQUIRE_EQUAL(3,          bm.prev(max));
    BOOST_REQUIRE_EQUAL((int)bm.end(),   bm.prev(3));
}

