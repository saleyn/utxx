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
#include <utxx/enumx.hpp>
#include <utxx/enum_flags.hpp>
#include <iostream>

UTXX_DEFINE_ENUMX
(my_enumx0, ' ',
    (A,  'a')
    (BB, 'b')
    (CCC)
);

UTXX_DEFINE_ENUMX(my_enumx, ' ', (A, 'a')(BB, 'b')(CCC));

UTXX_DEFINE_ENUMX(SideT, -1, (BID)(ASK)(SIDES));

// Define an enum with values A, B, C that can be converted to string
// and fron string using reflection class:
UTXX_DEFINE_ENUM(my_enum,
    A,  // Comment A
    B,
    C   // Comment C
);

UTXX_DEFINE_FLAGS(my_flags,
    A,
    B,
    C,
    D,
    E
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
    BOOST_CHECK_EQUAL(my_enum::UNDEFINED,   my_enum(0));
    BOOST_CHECK_EQUAL(my_enum::A,           my_enum(1));
    BOOST_CHECK_EQUAL(my_enum::A,           my_enum::begin());
    BOOST_CHECK_EQUAL(my_enum::C,           my_enum::last());
    BOOST_CHECK_EQUAL(my_enum::_END_,       my_enum::end());
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

BOOST_AUTO_TEST_CASE( test_enumx )
{
    static_assert(3 == my_enumx::size(), "Invalid size");

    my_enumx v;

    BOOST_CHECK(v.empty());

    BOOST_CHECK_EQUAL(' ',     (char)my_enumx::UNDEFINED);
    BOOST_CHECK_EQUAL(my_enumx::A,   my_enumx('a'));
    BOOST_CHECK_EQUAL(my_enumx::BB,  my_enumx('b'));
    BOOST_CHECK_EQUAL(my_enumx::CCC, my_enumx('c'));
    BOOST_CHECK_EQUAL("A",           my_enumx::to_string(my_enumx::A));
    BOOST_CHECK_EQUAL("BB",          my_enumx::to_string(my_enumx::BB));
    BOOST_CHECK_EQUAL("CCC",         my_enumx::to_string(my_enumx::CCC));
    BOOST_CHECK_EQUAL("A",           my_enumx::from_string("A").to_string());

    {
        my_enumx val = my_enumx::from_string("BB");
        BOOST_CHECK_EQUAL("BB", val.to_string());
        std::stringstream s; s << my_enumx::to_string(val);
        BOOST_CHECK_EQUAL("BB", s.str());
    }

    {
        // Iterate over all enum values defined in my_enum type:
        std::stringstream s;
        my_enumx::for_each([&s](my_enumx e) {
            s << e;
            return true;
        });
        BOOST_CHECK_EQUAL("ABBCCC", s.str());
    }

    BOOST_CHECK(my_enumx::A   == my_enumx::from_string("A"));
    BOOST_CHECK(my_enumx::BB  == my_enumx::from_string("BB"));
    BOOST_CHECK(my_enumx::CCC == my_enumx::from_string("CCC"));
    BOOST_CHECK(my_enumx::UNDEFINED == my_enumx::from_string("D"));

    static_assert(3 == my_enumx::size(), "Invalid size");
}

BOOST_AUTO_TEST_CASE( test_enum_flags )
{
    static_assert(5 == my_flags::size(), "Invalid size");

    my_flags v;

    BOOST_CHECK(v.empty());
    BOOST_CHECK(v == my_flags::NONE);

    v |= my_flags::B;

    BOOST_CHECK_EQUAL(0,        my_flags::NONE);
    BOOST_CHECK_EQUAL(1u << 0,  my_flags::A);
    BOOST_CHECK_EQUAL(1u << 1,  my_flags::B);
    BOOST_CHECK_EQUAL("A",      my_flags(my_flags::A).to_string());
    BOOST_CHECK_EQUAL("B",      my_flags(my_flags::B).to_string());
    BOOST_CHECK_EQUAL("A|C",    my_flags(my_flags::A, my_flags::C).to_string());
    BOOST_CHECK_EQUAL("A",      my_flags::from_string("A").to_string());

    BOOST_CHECK_THROW(my_flags::from_string("A|F"), utxx::badarg_error);

    {
        my_flags val = my_flags::from_string("A|B|E");
        BOOST_CHECK_EQUAL("A|B|E", val.to_string());
        std::stringstream s; s << my_flags::to_string(val);
        BOOST_CHECK_EQUAL("A|B|E", s.str());
    }

    v |= my_flags::E;

    BOOST_CHECK( v.has_all(my_flags::B | my_flags::E));
    BOOST_CHECK(!v.has_all(my_flags::A | my_flags::B | my_flags::E));
    BOOST_CHECK( v.has_all(my_flags::B));
    BOOST_CHECK( v.has_all(my_flags::E));
    BOOST_CHECK( v.has_any(my_flags::B | my_flags::E));
    BOOST_CHECK( v.has_any(my_flags::B));
    BOOST_CHECK( v.has_any(my_flags::E));
    BOOST_CHECK( v.has(my_flags::B));
    BOOST_CHECK( v.has(my_flags::E));

    {
        // Iterate over all enum values defined in my_enum type:
        std::stringstream s;
        v.for_each([&s](my_flags e) {
            s << e;
            return true;
        });
        BOOST_CHECK_EQUAL("BE", s.str());
    }

    v.clear();
    BOOST_CHECK(v.empty());
    v = my_flags::B | my_flags::C | my_flags::E;
    v.clear(my_flags::C | my_flags::E);
    BOOST_CHECK(v == my_flags::B);
}
