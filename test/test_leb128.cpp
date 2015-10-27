//----------------------------------------------------------------------------
/// \file  test_leb128.cpp
//----------------------------------------------------------------------------
/// \brief Little Endian Binary integer encoding test cases
/// \see LLVM
//----------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Copyright (c) 2003-2015 University of Illinois at Urbana-Champaign.
// Created: 2015-08-05
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2015 Serge Aleynikov <saleyn@gmail.com>

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
#include <boost/test/unit_test.hpp>
#include <utxx/leb128.hpp>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_leb128_encode_signed )
{
    #define EXPECT_SLEB128_EQ(EXPECTED, VALUE) \
        do { \
            const char Expected[] = EXPECTED; \
            char Actual[16]; \
            int n = encode_sleb128(VALUE, Actual); \
            Actual[n] = 0; \
            BOOST_CHECK_EQUAL(Expected, Actual); \
        } while (0)

    // Encode SLEB128
    EXPECT_SLEB128_EQ("\x00", 0);
    EXPECT_SLEB128_EQ("\x01", 1);
    EXPECT_SLEB128_EQ("\x7f", -1);
    EXPECT_SLEB128_EQ("\x3f", 63);
    EXPECT_SLEB128_EQ("\x41", -63);
    EXPECT_SLEB128_EQ("\x40", -64);
    EXPECT_SLEB128_EQ("\xbf\x7f", -65);
    EXPECT_SLEB128_EQ("\xc0\x00", 64);

    #undef EXPECT_SLEB128_EQ
}

BOOST_AUTO_TEST_CASE( test_leb128_encode_unsigned )
{
    #define EXPECT_ULEB128_EQ(EXPECTED, VALUE, PAD) \
    do { \
        const char Expected[] = EXPECTED; \
        char Actual[16]; \
        int n = encode_uleb128<PAD>(VALUE, Actual); \
        Actual[n] = 0; \
        BOOST_CHECK_EQUAL(Expected, Actual); \
    } while (0)

    // Encode ULEB128
    EXPECT_ULEB128_EQ("\x00", 0, 0);
    EXPECT_ULEB128_EQ("\x01", 1, 0);
    EXPECT_ULEB128_EQ("\x3f", 63, 0);
    EXPECT_ULEB128_EQ("\x40", 64, 0);
    EXPECT_ULEB128_EQ("\x7f", 0x7f, 0);
    EXPECT_ULEB128_EQ("\x80\x01", 0x80, 0);
    EXPECT_ULEB128_EQ("\x81\x01", 0x81, 0);
    EXPECT_ULEB128_EQ("\x90\x01", 0x90, 0);
    EXPECT_ULEB128_EQ("\xff\x01", 0xff, 0);
    EXPECT_ULEB128_EQ("\x80\x02", 0x100, 0);
    EXPECT_ULEB128_EQ("\x81\x02", 0x101, 0);

    // Encode ULEB128 with some extra padding bytes
    EXPECT_ULEB128_EQ("\x80\x00", 0, 1);
    EXPECT_ULEB128_EQ("\x80\x80\x00", 0, 2);
    EXPECT_ULEB128_EQ("\xff\x00", 0x7f, 1);
    EXPECT_ULEB128_EQ("\xff\x80\x00", 0x7f, 2);
    EXPECT_ULEB128_EQ("\x80\x81\x00", 0x80, 1);
    EXPECT_ULEB128_EQ("\x80\x81\x80\x00", 0x80, 2);

    #undef EXPECT_ULEB128_EQ
}

BOOST_AUTO_TEST_CASE( test_leb128_decode_unsigned )
{
    #define EXPECT_DECODE_ULEB128_EQ(EXPECTED, VALUE) \
    do { \
        auto p          = VALUE; \
        auto begin      = p; \
        auto Actual     = decode_uleb128(p); \
        int  ActualSize = p - begin; \
        BOOST_CHECK_EQUAL(sizeof(VALUE) - 1, size_t(ActualSize)); \
        BOOST_CHECK_EQUAL(EXPECTED, Actual); \
    } while (0)

    // Decode ULEB128
    EXPECT_DECODE_ULEB128_EQ(0u, "\x00");
    EXPECT_DECODE_ULEB128_EQ(1u, "\x01");
    EXPECT_DECODE_ULEB128_EQ(63u, "\x3f");
    EXPECT_DECODE_ULEB128_EQ(64u, "\x40");
    EXPECT_DECODE_ULEB128_EQ(0x7fu, "\x7f");
    EXPECT_DECODE_ULEB128_EQ(0x80u, "\x80\x01");
    EXPECT_DECODE_ULEB128_EQ(0x81u, "\x81\x01");
    EXPECT_DECODE_ULEB128_EQ(0x90u, "\x90\x01");
    EXPECT_DECODE_ULEB128_EQ(0xffu, "\xff\x01");
    EXPECT_DECODE_ULEB128_EQ(0x100u, "\x80\x02");
    EXPECT_DECODE_ULEB128_EQ(0x101u, "\x81\x02");
    EXPECT_DECODE_ULEB128_EQ(4294975616ULL, "\x80\xc1\x80\x80\x10");

    // Decode ULEB128 with extra padding bytes
    EXPECT_DECODE_ULEB128_EQ(0u, "\x80\x00");
    EXPECT_DECODE_ULEB128_EQ(0u, "\x80\x80\x00");
    EXPECT_DECODE_ULEB128_EQ(0x7fu, "\xff\x00");
    EXPECT_DECODE_ULEB128_EQ(0x7fu, "\xff\x80\x00");
    EXPECT_DECODE_ULEB128_EQ(0x80u, "\x80\x81\x00");
    EXPECT_DECODE_ULEB128_EQ(0x80u, "\x80\x81\x80\x00");

    #undef EXPECT_DECODE_ULEB128_EQ
}

BOOST_AUTO_TEST_CASE( test_leb128_decode_signed )
{
    #define EXPECT_DECODE_SLEB128_EQ(EXPECTED, VALUE) \
    do { \
        auto p          = VALUE; \
        auto begin      = p; \
        auto Actual     = decode_sleb128(p); \
        int  ActualSize = p - begin; \
        BOOST_CHECK_EQUAL(sizeof(VALUE) - 1, size_t(ActualSize)); \
        BOOST_CHECK_EQUAL(EXPECTED, Actual); \
    } while (0)

    // Decode SLEB128
    EXPECT_DECODE_SLEB128_EQ(0L, "\x00");
    EXPECT_DECODE_SLEB128_EQ(1L, "\x01");
    EXPECT_DECODE_SLEB128_EQ(63L, "\x3f");
    EXPECT_DECODE_SLEB128_EQ(-64L, "\x40");
    EXPECT_DECODE_SLEB128_EQ(-63L, "\x41");
    EXPECT_DECODE_SLEB128_EQ(-1L, "\x7f");
    EXPECT_DECODE_SLEB128_EQ(128L, "\x80\x01");
    EXPECT_DECODE_SLEB128_EQ(129L, "\x81\x01");
    EXPECT_DECODE_SLEB128_EQ(-129L, "\xff\x7e");
    EXPECT_DECODE_SLEB128_EQ(-128L, "\x80\x7f");
    EXPECT_DECODE_SLEB128_EQ(-127L, "\x81\x7f");
    EXPECT_DECODE_SLEB128_EQ(64L, "\xc0\x00");
    EXPECT_DECODE_SLEB128_EQ(-12345L, "\xc7\x9f\x7f");

    // Decode unnormalized SLEB128 with extra padding bytes.
    EXPECT_DECODE_SLEB128_EQ(0L, "\x80\x00");
    EXPECT_DECODE_SLEB128_EQ(0L, "\x80\x80\x00");
    EXPECT_DECODE_SLEB128_EQ(0x7fL, "\xff\x00");
    EXPECT_DECODE_SLEB128_EQ(0x7fL, "\xff\x80\x00");
    EXPECT_DECODE_SLEB128_EQ(0x80L, "\x80\x81\x00");
    EXPECT_DECODE_SLEB128_EQ(0x80L, "\x80\x81\x80\x00");

    #undef EXPECT_DECODE_SLEB128_EQ
}

BOOST_AUTO_TEST_CASE( test_leb128_encode_size_signed )
{
    // Positive Value Testing Plan:
    // (1) 128 ^ n - 1 ........ need (n+1) bytes
    // (2) 128 ^ n ............ need (n+1) bytes
    // (3) 128 ^ n * 63 ....... need (n+1) bytes
    // (4) 128 ^ n * 64 - 1 ... need (n+1) bytes
    // (5) 128 ^ n * 64 ....... need (n+2) bytes

    BOOST_CHECK_EQUAL(1, encoded_sleb128_size(0x0LL));
    BOOST_CHECK_EQUAL(1, encoded_sleb128_size(0x1LL));
    BOOST_CHECK_EQUAL(1, encoded_sleb128_size(0x3fLL));
    BOOST_CHECK_EQUAL(1, encoded_sleb128_size(0x3fLL));
    BOOST_CHECK_EQUAL(2, encoded_sleb128_size(0x40LL));

    BOOST_CHECK_EQUAL(2, encoded_sleb128_size(0x7fLL));
    BOOST_CHECK_EQUAL(2, encoded_sleb128_size(0x80LL));
    BOOST_CHECK_EQUAL(2, encoded_sleb128_size(0x1f80LL));
    BOOST_CHECK_EQUAL(2, encoded_sleb128_size(0x1fffLL));
    BOOST_CHECK_EQUAL(3, encoded_sleb128_size(0x2000LL));

    BOOST_CHECK_EQUAL(3, encoded_sleb128_size(0x3fffLL));
    BOOST_CHECK_EQUAL(3, encoded_sleb128_size(0x4000LL));
    BOOST_CHECK_EQUAL(3, encoded_sleb128_size(0xfc000LL));
    BOOST_CHECK_EQUAL(3, encoded_sleb128_size(0xfffffLL));
    BOOST_CHECK_EQUAL(4, encoded_sleb128_size(0x100000LL));

    BOOST_CHECK_EQUAL(4, encoded_sleb128_size(0x1fffffLL));
    BOOST_CHECK_EQUAL(4, encoded_sleb128_size(0x200000LL));
    BOOST_CHECK_EQUAL(4, encoded_sleb128_size(0x7e00000LL));
    BOOST_CHECK_EQUAL(4, encoded_sleb128_size(0x7ffffffLL));
    BOOST_CHECK_EQUAL(5, encoded_sleb128_size(0x8000000LL));

    BOOST_CHECK_EQUAL(5, encoded_sleb128_size(0xfffffffLL));
    BOOST_CHECK_EQUAL(5, encoded_sleb128_size(0x10000000LL));
    BOOST_CHECK_EQUAL(5, encoded_sleb128_size(0x3f0000000LL));
    BOOST_CHECK_EQUAL(5, encoded_sleb128_size(0x3ffffffffLL));
    BOOST_CHECK_EQUAL(6, encoded_sleb128_size(0x400000000LL));

    BOOST_CHECK_EQUAL(6, encoded_sleb128_size(0x7ffffffffLL));
    BOOST_CHECK_EQUAL(6, encoded_sleb128_size(0x800000000LL));
    BOOST_CHECK_EQUAL(6, encoded_sleb128_size(0x1f800000000LL));
    BOOST_CHECK_EQUAL(6, encoded_sleb128_size(0x1ffffffffffLL));
    BOOST_CHECK_EQUAL(7, encoded_sleb128_size(0x20000000000LL));

    BOOST_CHECK_EQUAL(7, encoded_sleb128_size(0x3ffffffffffLL));
    BOOST_CHECK_EQUAL(7, encoded_sleb128_size(0x40000000000LL));
    BOOST_CHECK_EQUAL(7, encoded_sleb128_size(0xfc0000000000LL));
    BOOST_CHECK_EQUAL(7, encoded_sleb128_size(0xffffffffffffLL));
    BOOST_CHECK_EQUAL(8, encoded_sleb128_size(0x1000000000000LL));

    BOOST_CHECK_EQUAL(8, encoded_sleb128_size(0x1ffffffffffffLL));
    BOOST_CHECK_EQUAL(8, encoded_sleb128_size(0x2000000000000LL));
    BOOST_CHECK_EQUAL(8, encoded_sleb128_size(0x7e000000000000LL));
    BOOST_CHECK_EQUAL(8, encoded_sleb128_size(0x7fffffffffffffLL));
    BOOST_CHECK_EQUAL(9, encoded_sleb128_size(0x80000000000000LL));

    BOOST_CHECK_EQUAL(9, encoded_sleb128_size(0xffffffffffffffLL));
    BOOST_CHECK_EQUAL(9, encoded_sleb128_size(0x100000000000000LL));
    BOOST_CHECK_EQUAL(9, encoded_sleb128_size(0x3f00000000000000LL));
    BOOST_CHECK_EQUAL(9, encoded_sleb128_size(0x3fffffffffffffffLL));
    BOOST_CHECK_EQUAL(10, encoded_sleb128_size(0x4000000000000000LL));

    BOOST_CHECK_EQUAL(10, encoded_sleb128_size(0x7fffffffffffffffLL));
    BOOST_CHECK_EQUAL(10, encoded_sleb128_size(INT64_MAX));

    // Negative Value Testing Plan:
    // (1) - 128 ^ n - 1 ........ need (n+1) bytes
    // (2) - 128 ^ n ............ need (n+1) bytes
    // (3) - 128 ^ n * 63 ....... need (n+1) bytes
    // (4) - 128 ^ n * 64 ....... need (n+1) bytes (different from positive one)
    // (5) - 128 ^ n * 65 - 1 ... need (n+2) bytes (if n > 0)
    // (6) - 128 ^ n * 65 ....... need (n+2) bytes

    BOOST_CHECK_EQUAL(1, encoded_sleb128_size(0x0LL));
    BOOST_CHECK_EQUAL(1, encoded_sleb128_size(-0x1LL));
    BOOST_CHECK_EQUAL(1, encoded_sleb128_size(-0x3fLL));
    BOOST_CHECK_EQUAL(1, encoded_sleb128_size(-0x40LL));
    BOOST_CHECK_EQUAL(1, encoded_sleb128_size(-0x40LL)); // special case
    BOOST_CHECK_EQUAL(2, encoded_sleb128_size(-0x41LL));

    BOOST_CHECK_EQUAL(2, encoded_sleb128_size(-0x7fLL));
    BOOST_CHECK_EQUAL(2, encoded_sleb128_size(-0x80LL));
    BOOST_CHECK_EQUAL(2, encoded_sleb128_size(-0x1f80LL));
    BOOST_CHECK_EQUAL(2, encoded_sleb128_size(-0x2000LL));
    BOOST_CHECK_EQUAL(3, encoded_sleb128_size(-0x207fLL));
    BOOST_CHECK_EQUAL(3, encoded_sleb128_size(-0x2080LL));

    BOOST_CHECK_EQUAL(3, encoded_sleb128_size(-0x3fffLL));
    BOOST_CHECK_EQUAL(3, encoded_sleb128_size(-0x4000LL));
    BOOST_CHECK_EQUAL(3, encoded_sleb128_size(-0xfc000LL));
    BOOST_CHECK_EQUAL(3, encoded_sleb128_size(-0x100000LL));
    BOOST_CHECK_EQUAL(4, encoded_sleb128_size(-0x103fffLL));
    BOOST_CHECK_EQUAL(4, encoded_sleb128_size(-0x104000LL));

    BOOST_CHECK_EQUAL(4, encoded_sleb128_size(-0x1fffffLL));
    BOOST_CHECK_EQUAL(4, encoded_sleb128_size(-0x200000LL));
    BOOST_CHECK_EQUAL(4, encoded_sleb128_size(-0x7e00000LL));
    BOOST_CHECK_EQUAL(4, encoded_sleb128_size(-0x8000000LL));
    BOOST_CHECK_EQUAL(5, encoded_sleb128_size(-0x81fffffLL));
    BOOST_CHECK_EQUAL(5, encoded_sleb128_size(-0x8200000LL));

    BOOST_CHECK_EQUAL(5, encoded_sleb128_size(-0xfffffffLL));
    BOOST_CHECK_EQUAL(5, encoded_sleb128_size(-0x10000000LL));
    BOOST_CHECK_EQUAL(5, encoded_sleb128_size(-0x3f0000000LL));
    BOOST_CHECK_EQUAL(5, encoded_sleb128_size(-0x400000000LL));
    BOOST_CHECK_EQUAL(6, encoded_sleb128_size(-0x40fffffffLL));
    BOOST_CHECK_EQUAL(6, encoded_sleb128_size(-0x410000000LL));

    BOOST_CHECK_EQUAL(6, encoded_sleb128_size(-0x7ffffffffLL));
    BOOST_CHECK_EQUAL(6, encoded_sleb128_size(-0x800000000LL));
    BOOST_CHECK_EQUAL(6, encoded_sleb128_size(-0x1f800000000LL));
    BOOST_CHECK_EQUAL(6, encoded_sleb128_size(-0x20000000000LL));
    BOOST_CHECK_EQUAL(7, encoded_sleb128_size(-0x207ffffffffLL));
    BOOST_CHECK_EQUAL(7, encoded_sleb128_size(-0x20800000000LL));

    BOOST_CHECK_EQUAL(7, encoded_sleb128_size(-0x3ffffffffffLL));
    BOOST_CHECK_EQUAL(7, encoded_sleb128_size(-0x40000000000LL));
    BOOST_CHECK_EQUAL(7, encoded_sleb128_size(-0xfc0000000000LL));
    BOOST_CHECK_EQUAL(7, encoded_sleb128_size(-0x1000000000000LL));
    BOOST_CHECK_EQUAL(8, encoded_sleb128_size(-0x103ffffffffffLL));
    BOOST_CHECK_EQUAL(8, encoded_sleb128_size(-0x1040000000000LL));

    BOOST_CHECK_EQUAL(8, encoded_sleb128_size(-0x1ffffffffffffLL));
    BOOST_CHECK_EQUAL(8, encoded_sleb128_size(-0x2000000000000LL));
    BOOST_CHECK_EQUAL(8, encoded_sleb128_size(-0x7e000000000000LL));
    BOOST_CHECK_EQUAL(8, encoded_sleb128_size(-0x80000000000000LL));
    BOOST_CHECK_EQUAL(9, encoded_sleb128_size(-0x81ffffffffffffLL));
    BOOST_CHECK_EQUAL(9, encoded_sleb128_size(-0x82000000000000LL));

    BOOST_CHECK_EQUAL(9, encoded_sleb128_size(-0xffffffffffffffLL));
    BOOST_CHECK_EQUAL(9, encoded_sleb128_size(-0x100000000000000LL));
    BOOST_CHECK_EQUAL(9, encoded_sleb128_size(-0x3f00000000000000LL));
    BOOST_CHECK_EQUAL(9, encoded_sleb128_size(-0x4000000000000000LL));
    BOOST_CHECK_EQUAL(10, encoded_sleb128_size(-0x40ffffffffffffffLL));
    BOOST_CHECK_EQUAL(10, encoded_sleb128_size(-0x4100000000000000LL));

    BOOST_CHECK_EQUAL(10, encoded_sleb128_size(-0x7fffffffffffffffLL));
    BOOST_CHECK_EQUAL(10, encoded_sleb128_size(-0x8000000000000000LL));
    BOOST_CHECK_EQUAL(10, encoded_sleb128_size(INT64_MIN));
}

BOOST_AUTO_TEST_CASE( test_leb128_encode_size_unsigned )
{
    // Testing Plan:
    // (1) 128 ^ n ............ need (n+1) bytes
    // (2) 128 ^ n * 64 ....... need (n+1) bytes
    // (3) 128 ^ (n+1) - 1 .... need (n+1) bytes

    BOOST_CHECK_EQUAL(1, encoded_uleb128_size(0)); // special case

    BOOST_CHECK_EQUAL(1, encoded_uleb128_size(0x1ULL));
    BOOST_CHECK_EQUAL(1, encoded_uleb128_size(0x40ULL));
    BOOST_CHECK_EQUAL(1, encoded_uleb128_size(0x7fULL));

    BOOST_CHECK_EQUAL(2, encoded_uleb128_size(0x80ULL));
    BOOST_CHECK_EQUAL(2, encoded_uleb128_size(0x2000ULL));
    BOOST_CHECK_EQUAL(2, encoded_uleb128_size(0x3fffULL));

    BOOST_CHECK_EQUAL(3, encoded_uleb128_size(0x4000ULL));
    BOOST_CHECK_EQUAL(3, encoded_uleb128_size(0x100000ULL));
    BOOST_CHECK_EQUAL(3, encoded_uleb128_size(0x1fffffULL));

    BOOST_CHECK_EQUAL(4, encoded_uleb128_size(0x200000ULL));
    BOOST_CHECK_EQUAL(4, encoded_uleb128_size(0x8000000ULL));
    BOOST_CHECK_EQUAL(4, encoded_uleb128_size(0xfffffffULL));

    BOOST_CHECK_EQUAL(5, encoded_uleb128_size(0x10000000ULL));
    BOOST_CHECK_EQUAL(5, encoded_uleb128_size(0x400000000ULL));
    BOOST_CHECK_EQUAL(5, encoded_uleb128_size(0x7ffffffffULL));

    BOOST_CHECK_EQUAL(6, encoded_uleb128_size(0x800000000ULL));
    BOOST_CHECK_EQUAL(6, encoded_uleb128_size(0x20000000000ULL));
    BOOST_CHECK_EQUAL(6, encoded_uleb128_size(0x3ffffffffffULL));

    BOOST_CHECK_EQUAL(7, encoded_uleb128_size(0x40000000000ULL));
    BOOST_CHECK_EQUAL(7, encoded_uleb128_size(0x1000000000000ULL));
    BOOST_CHECK_EQUAL(7, encoded_uleb128_size(0x1ffffffffffffULL));

    BOOST_CHECK_EQUAL(8, encoded_uleb128_size(0x2000000000000ULL));
    BOOST_CHECK_EQUAL(8, encoded_uleb128_size(0x80000000000000ULL));
    BOOST_CHECK_EQUAL(8, encoded_uleb128_size(0xffffffffffffffULL));

    BOOST_CHECK_EQUAL(9, encoded_uleb128_size(0x100000000000000ULL));
    BOOST_CHECK_EQUAL(9, encoded_uleb128_size(0x4000000000000000ULL));
    BOOST_CHECK_EQUAL(9, encoded_uleb128_size(0x7fffffffffffffffULL));

    BOOST_CHECK_EQUAL(10, encoded_uleb128_size(0x8000000000000000ULL));

    BOOST_CHECK_EQUAL(10, encoded_uleb128_size(UINT64_MAX));
}
