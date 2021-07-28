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

#include <boost/test/unit_test.hpp>
#include <utxx/math.hpp>
#include <utxx/verbosity.hpp>
#include <iostream>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_math_power )
{
    BOOST_REQUIRE_EQUAL(1024,   math::power(2, 10));
    BOOST_REQUIRE_EQUAL(2187,   math::power(3, 7));
    BOOST_REQUIRE_EQUAL(390625, math::power(5, 8));
    BOOST_REQUIRE_EQUAL(0,      math::power(0, 0));
    BOOST_REQUIRE_EQUAL(0,      math::power(0, 1));
    BOOST_REQUIRE_EQUAL(1,      math::power(1, 0));
    BOOST_REQUIRE_EQUAL(1,      math::power(100, 0));
    BOOST_REQUIRE_EQUAL(100,    math::power(100, 1));
}

BOOST_AUTO_TEST_CASE( test_math_log )
{
    BOOST_REQUIRE_EQUAL(10,     math::log(1024, 2));
    BOOST_REQUIRE_EQUAL(2,      math::log(9,  3));
    BOOST_REQUIRE_EQUAL(2,      math::log(11, 3));
    BOOST_REQUIRE_EQUAL(1,      math::log(2, 2));
    BOOST_REQUIRE_EQUAL(0,      math::log(1, 2));
    BOOST_REQUIRE_EQUAL(0,      math::log(0, 2));

    BOOST_REQUIRE_EQUAL(63,     math::log2(1lu << 63));
    BOOST_REQUIRE_EQUAL(32,     math::log2(1lu << 32));
    BOOST_REQUIRE_EQUAL(10,     math::log2(1024));
    BOOST_REQUIRE_EQUAL(4,      math::log2(16));
    BOOST_REQUIRE_EQUAL(3,      math::log2(8));
    BOOST_REQUIRE_EQUAL(1,      math::log2(2));
    BOOST_REQUIRE_EQUAL(0,      math::log2(1));
    BOOST_REQUIRE_EQUAL(0,      math::log2(0));
}

BOOST_AUTO_TEST_CASE( test_math_upper_power )
{
    BOOST_REQUIRE_EQUAL(1024,   math::upper_power(1024, 2));
    BOOST_REQUIRE_EQUAL(9,      math::upper_power(9,  3));
    BOOST_REQUIRE_EQUAL(27,     math::upper_power(11, 3));
    BOOST_REQUIRE_EQUAL(16,     math::upper_power(14, 2));
    BOOST_REQUIRE_EQUAL(16,     math::upper_power(15, 2));
    BOOST_REQUIRE_EQUAL(16,     math::upper_power(16, 2));
    BOOST_REQUIRE_EQUAL(4,      math::upper_power(3,  2));
}

BOOST_AUTO_TEST_CASE( test_math_precision_digits )
{
    BOOST_CHECK_EQUAL(5,        math::precision_digits(0.00001));
    BOOST_CHECK_EQUAL(5,        math::precision_digits(0.12345));
    BOOST_CHECK_EQUAL(5,        math::precision_digits(-0.12345));
    BOOST_CHECK_EQUAL(9,        math::precision_digits(0.123456789, 1e-12));
    BOOST_CHECK_EQUAL(1,        math::precision_digits(0.1));
    BOOST_CHECK_EQUAL(0,        math::precision_digits(0.0));
    BOOST_CHECK_EQUAL(0,        math::precision_digits(1.0));
    BOOST_CHECK_EQUAL(12,       math::precision_digits(0.000000012345));
    BOOST_CHECK_EQUAL(12,       math::precision_digits(0.000000012345, 1e-13));
    BOOST_CHECK_EQUAL(8,        math::precision_digits(0.10000001));
}

BOOST_AUTO_TEST_CASE( test_math_gcd_lcm )
{
    BOOST_REQUIRE_EQUAL(2,      math::gcd(18, 4));
    BOOST_REQUIRE_EQUAL(36,     math::lcm(18, 4));
    BOOST_REQUIRE_EQUAL(4,      math::gcd(0,  4));
    BOOST_REQUIRE_EQUAL(4,      math::gcd(4,  0));
    BOOST_REQUIRE_EQUAL(0,      math::lcm(4,  0));
}

BOOST_AUTO_TEST_CASE( test_math_is_prime )
{
    BOOST_REQUIRE_EQUAL(false,  math::is_prime(0));
    BOOST_REQUIRE_EQUAL(false,  math::is_prime(1));
    BOOST_REQUIRE_EQUAL(true ,  math::is_prime(2));
    BOOST_REQUIRE_EQUAL(true ,  math::is_prime(3));
    BOOST_REQUIRE_EQUAL(false,  math::is_prime(4));
    BOOST_REQUIRE_EQUAL(true ,  math::is_prime(5));
    BOOST_REQUIRE_EQUAL(false,  math::is_prime(6));
    BOOST_REQUIRE_EQUAL(true ,  math::is_prime(7));
    BOOST_REQUIRE_EQUAL(false,  math::is_prime(8));
    BOOST_REQUIRE_EQUAL(false,  math::is_prime(9));
    BOOST_REQUIRE_EQUAL(true ,  math::is_prime(11));
    BOOST_REQUIRE_EQUAL(true ,  math::is_prime(13));
    BOOST_REQUIRE_EQUAL(false,  math::is_prime(49));
    BOOST_REQUIRE_EQUAL(true ,  math::is_prime(101));
}
