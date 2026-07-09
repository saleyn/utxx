//----------------------------------------------------------------------------
/// \file  test_math.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes and functions in the math.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-05-06
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects

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

#include <utxx/test_helper.hpp>
#include <utxx/math.hpp>
#include <utxx/verbosity.hpp>
#include <iostream>

using namespace utxx;

TEST_CASE( test_math_power )
{
    REQUIRE_EQUAL(1024,   math::power(2, 10));
    REQUIRE_EQUAL(2187,   math::power(3, 7));
    REQUIRE_EQUAL(390625, math::power(5, 8));
    REQUIRE_EQUAL(1,      math::power(0, 0));  // 0^0 = 1 by convention
    REQUIRE_EQUAL(0,      math::power(0, 1));
    REQUIRE_EQUAL(1,      math::power(1, 0));
    REQUIRE_EQUAL(1,      math::power(100, 0));
    REQUIRE_EQUAL(100,    math::power(100, 1));
}

TEST_CASE( test_math_log )
{
    REQUIRE_EQUAL(10,     math::log(1024, 2));
    REQUIRE_EQUAL(2,      math::log(9,  3));
    REQUIRE_EQUAL(2,      math::log(11, 3));
    REQUIRE_EQUAL(1,      math::log(2, 2));
    REQUIRE_EQUAL(0,      math::log(1, 2));
    REQUIRE_EQUAL(0,      math::log(0, 2));

    REQUIRE_EQUAL(63,     math::log2(1lu << 63));
    REQUIRE_EQUAL(32,     math::log2(1lu << 32));
    REQUIRE_EQUAL(10,     math::log2(1024));
    REQUIRE_EQUAL(4,      math::log2(16));
    REQUIRE_EQUAL(3,      math::log2(8));
    REQUIRE_EQUAL(1,      math::log2(2));
    REQUIRE_EQUAL(0,      math::log2(1));
    // log2(0) is undefined behavior; skip
}

TEST_CASE( test_math_upper_power )
{
    REQUIRE_EQUAL(1024,   math::upper_power(1024, 2));
    REQUIRE_EQUAL(9,      math::upper_power(9,  3));
    REQUIRE_EQUAL(27,     math::upper_power(11, 3));
    REQUIRE_EQUAL(16,     math::upper_power(14, 2));
    REQUIRE_EQUAL(16,     math::upper_power(15, 2));
    REQUIRE_EQUAL(16,     math::upper_power(16, 2));
    REQUIRE_EQUAL(4,      math::upper_power(3,  2));
}

TEST_CASE( test_math_precision_digits )
{
    CHECK_EQUAL(5,        math::precision_digits(0.00001));
    CHECK_EQUAL(5,        math::precision_digits(0.12345));
    CHECK_EQUAL(5,        math::precision_digits(-0.12345));
    CHECK_EQUAL(9,        math::precision_digits(0.123456789, 1e-12));
    CHECK_EQUAL(1,        math::precision_digits(0.1));
    CHECK_EQUAL(0,        math::precision_digits(0.0));
    CHECK_EQUAL(0,        math::precision_digits(1.0));
    CHECK_EQUAL(12,       math::precision_digits(0.000000012345));
    CHECK_EQUAL(12,       math::precision_digits(0.000000012345, 1e-13));
    CHECK_EQUAL(8,        math::precision_digits(0.10000001));
}

TEST_CASE( test_math_gcd_lcm )
{
    REQUIRE_EQUAL(2,      math::gcd(18, 4));
    REQUIRE_EQUAL(36,     math::lcm(18, 4));
    REQUIRE_EQUAL(4,      math::gcd(0,  4));
    REQUIRE_EQUAL(4,      math::gcd(4,  0));
    REQUIRE_EQUAL(0,      math::lcm(4,  0));
}

TEST_CASE( test_math_is_prime )
{
    REQUIRE_EQUAL(false,  math::is_prime(0));
    REQUIRE_EQUAL(false,  math::is_prime(1));
    REQUIRE_EQUAL(true ,  math::is_prime(2));
    REQUIRE_EQUAL(true ,  math::is_prime(3));
    REQUIRE_EQUAL(false,  math::is_prime(4));
    REQUIRE_EQUAL(true ,  math::is_prime(5));
    REQUIRE_EQUAL(false,  math::is_prime(6));
    REQUIRE_EQUAL(true ,  math::is_prime(7));
    REQUIRE_EQUAL(false,  math::is_prime(8));
    REQUIRE_EQUAL(false,  math::is_prime(9));
    REQUIRE_EQUAL(true ,  math::is_prime(11));
    REQUIRE_EQUAL(true ,  math::is_prime(13));
    REQUIRE_EQUAL(false,  math::is_prime(49));
    REQUIRE_EQUAL(true ,  math::is_prime(101));
}

TEST_CASE( test_math_next_pow2 )
{
    // Boundary: 0 and 1 both return 1
    REQUIRE_EQUAL(1u,     math::upper_pow2(0u));
    REQUIRE_EQUAL(1u,     math::upper_pow2(1u));
    // Exact powers of two are returned unchanged
    REQUIRE_EQUAL(2u,     math::upper_pow2(2u));
    REQUIRE_EQUAL(4u,     math::upper_pow2(4u));
    REQUIRE_EQUAL(8u,     math::upper_pow2(8u));
    REQUIRE_EQUAL(16u,    math::upper_pow2(16u));
    REQUIRE_EQUAL(1024u,  math::upper_pow2(1024u));
    // Non-powers round up to the next power of two
    REQUIRE_EQUAL(4u,     math::upper_pow2(3u));
    REQUIRE_EQUAL(8u,     math::upper_pow2(5u));
    REQUIRE_EQUAL(8u,     math::upper_pow2(7u));
    REQUIRE_EQUAL(16u,    math::upper_pow2(9u));
    REQUIRE_EQUAL(16u,    math::upper_pow2(15u));
    REQUIRE_EQUAL(32u,    math::upper_pow2(17u));
    REQUIRE_EQUAL(1024u,  math::upper_pow2(1023u));
    REQUIRE_EQUAL(2048u,  math::upper_pow2(1025u));
}
