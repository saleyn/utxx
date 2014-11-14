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

// Define an enum with values A, B, C that can be converted to string
// and fron string using reflection class:
UTXX_DEFINE_ENUM(my_enum,
    A,  // Comment A
    B,
    C   // Comment C
);

// Define an enum my_enum2 inside a struct:
struct oh_my {
    UTXX_DEFINE_ENUM(my_enum2, X, Y);
};

BOOST_AUTO_TEST_CASE( test_enum )
{
    static_assert(3 == my_enum::size(), "Invalid size");

    my_enum v;

    BOOST_CHECK(v.empty());

    BOOST_CHECK_EQUAL(0,               (int)my_enum::UNDEFINED);
    BOOST_CHECK_EQUAL(my_enum::A,           my_enum::first());
    BOOST_CHECK_EQUAL(my_enum::C,           my_enum::last());
    BOOST_CHECK_EQUAL(1+int(my_enum::C),    my_enum::end());
    BOOST_CHECK_EQUAL("A", my_enum::to_string(my_enum::A));
    BOOST_CHECK_EQUAL("B", my_enum::to_string(my_enum::B));
    BOOST_CHECK_EQUAL("C", my_enum::to_string(my_enum::C));
    BOOST_CHECK_EQUAL("A", my_enum::from_string("A").to_string());

    {
        my_enum val = my_enum::from_string("B");
        BOOST_CHECK_EQUAL("B", val.to_string());
        std::stringstream s; s << my_enum::to_string(val);
        BOOST_CHECK_EQUAL("B", s.str());
    }

    {
        // Iterate over all enum values defined in my_enum type:
        std::stringstream s;
        my_enum::for_each([&s](my_enum e) { s << e; return true; });
        BOOST_CHECK_EQUAL("ABC", s.str());
    }

    BOOST_CHECK(my_enum::A == my_enum::from_string("A"));
    BOOST_CHECK(my_enum::B == my_enum::from_string("B"));
    BOOST_CHECK(my_enum::C == my_enum::from_string("C"));
    BOOST_CHECK(my_enum::UNDEFINED == my_enum::from_string("D"));

    static_assert(2 == oh_my::my_enum2::size(), "Invalid size");
    BOOST_CHECK_EQUAL("X", oh_my::my_enum2::to_string(oh_my::my_enum2::X));
    BOOST_CHECK_EQUAL("Y", oh_my::my_enum2::to_string(oh_my::my_enum2::Y));

    BOOST_CHECK(oh_my::my_enum2::X == oh_my::my_enum2::from_string("X"));
    BOOST_CHECK(oh_my::my_enum2::Y == oh_my::my_enum2::from_string("Y"));
    BOOST_CHECK(oh_my::my_enum2::UNDEFINED == oh_my::my_enum2::from_string("D"));
}
