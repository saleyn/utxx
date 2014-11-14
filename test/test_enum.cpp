//----------------------------------------------------------------------------
/// \file   test_enum.cpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Test cases for enum stringification declaration macro.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-09-31
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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
#include <utxx/enum.hpp>
#include <iostream>

UTXX_DEFINE_ENUM_INFO;

// Define an enum with values A, B, C that can be converted to string
// and fron string using reflection class:
UTXX_DEFINE_ENUM(my_enum,
    A,  // Comment A
    B,
    C   // Comment C
)


UTXX_DEFINE_CUSTOM_ENUM_INFO(oh_my);

UTXX_DEFINE_CUSTOM_ENUM(oh_my, my_enum2, X, Y);


BOOST_AUTO_TEST_CASE( test_enum )
{
    static_assert(3 == reflection<my_enum>::size(), "Invalid size");
    static_assert(my_enum::UNDEFINED == reflection<my_enum>::end(), "Invalid end");
    static_assert(my_enum::UNDEFINED == reflection<my_enum>::end(), "Invalid end");
    BOOST_CHECK_EQUAL("A", to_string(my_enum::A));
    BOOST_CHECK_EQUAL("B", to_string(my_enum::B));
    BOOST_CHECK_EQUAL("C", to_string(my_enum::C));

    my_enum val = reflection<my_enum>::from_string("B");
    std::stringstream s; s << to_string(val);
    BOOST_CHECK_EQUAL("B", s.str());

    BOOST_CHECK(my_enum::A == reflection<my_enum>::from_string("A"));
    BOOST_CHECK(my_enum::B == reflection<my_enum>::from_string("B"));
    BOOST_CHECK(my_enum::C == reflection<my_enum>::from_string("C"));
    BOOST_CHECK(my_enum::UNDEFINED == reflection<my_enum>::from_string("D"));

    static_assert(2 == oh_my<my_enum2>::size(), "Invalid size");
    BOOST_CHECK_EQUAL("X", to_string(my_enum2::X));
    BOOST_CHECK_EQUAL("Y", to_string(my_enum2::Y));

    BOOST_CHECK(my_enum2::X == oh_my<my_enum2>::from_string("X"));
    BOOST_CHECK(my_enum2::Y == oh_my<my_enum2>::from_string("Y"));
    BOOST_CHECK(my_enum2::UNDEFINED == oh_my<my_enum2>::from_string("D"));
}
