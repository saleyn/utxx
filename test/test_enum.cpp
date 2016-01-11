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
#include <boost/version.hpp>
#if (BOOST_VERSION >= 105700)

#include <boost/test/unit_test.hpp>
#include <utxx/enum.hpp>
#include <utxx/enumx.hpp>
#include <utxx/enum_flags.hpp>
#include <iostream>

//------------------------------------------------------------------------------
// Using UTXX_ENUM, UTXX_ENUMX, UTXX_ENUM_FLAGS
//------------------------------------------------------------------------------
UTXX_ENUMX
(mm_enumx0, char, ' ',
    (A,  'a')
    (Bb, 'b')
    (CCC)
);

UTXX_ENUMX(mmSideT, int8_t, -1, (BID)(ASK)(SIDES));
UTXX_ENUM( mm_enum, int64_t, A /* Comment */, B, C);
UTXX_ENUMX(mm_enumx, char, ' ', (A, 'a')(BB, 'b')(CCC));
UTXX_ENUM_FLAGS(mm_flags, uint8_t,
    A,
    B,
    C,
    D,
    E
);

// Define an enum my_enum2 inside a struct:
struct oh_mm { UTXX_ENUM(mm_enum2, char, X, Y); };

//------------------------------------------------------------------------------
// Using deprecated UTXX_DEFINE_ENUM, UTXX_DEFINE_ENUMX, UTXX_DEFINE_FLAGS
//------------------------------------------------------------------------------
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
struct oh_my { UTXX_DEFINE_ENUM(my_enum2, X, Y); };

BOOST_AUTO_TEST_CASE( test_enum )
{
    static_assert(3 == mm_enum::size(),         "Invalid size");
    static_assert(1 == sizeof(mm_enumx0),       "Invalid size");
    static_assert(1 == sizeof(mmSideT),         "Invalid size");
    static_assert(8 == sizeof(mm_enum),         "Invalid size");
    static_assert(1 == sizeof(oh_mm::mm_enum2), "Invalid size");

    mm_enum v;

    BOOST_CHECK(v.empty());

    BOOST_CHECK_EQUAL(0,               (int)mm_enum::UNDEFINED);
    BOOST_CHECK_EQUAL(mm_enum::UNDEFINED,   mm_enum(0));
    BOOST_CHECK_EQUAL(mm_enum::A,           mm_enum(1));
    BOOST_CHECK_EQUAL(mm_enum::A,           mm_enum::begin());
    BOOST_CHECK_EQUAL(mm_enum::C,           mm_enum::last());
    BOOST_CHECK_EQUAL(mm_enum::_END_,       mm_enum::end());
    BOOST_CHECK_EQUAL(1+int(mm_enum::C),    mm_enum::end());
    BOOST_CHECK_EQUAL("A", mm_enum::to_string(mm_enum::A));
    BOOST_CHECK_EQUAL("B", mm_enum::to_string(mm_enum::B));
    BOOST_CHECK_EQUAL("C", mm_enum::to_string(mm_enum::C));
    BOOST_CHECK_EQUAL("A", mm_enum::from_string("A").to_string());

    {
        mm_enum val = mm_enum::from_string("B");
        BOOST_CHECK_EQUAL("B", val.to_string());
        std::stringstream s; s << mm_enum::to_string(val);
        BOOST_CHECK_EQUAL("B", s.str());

        auto val1 = mm_enumx0::from_string("Bb", true);
        BOOST_CHECK_EQUAL('b', (char)val1);
    }

    {
        // Iterate over all enum values defined in mm_enum type:
        std::stringstream s;
        mm_enum::for_each([&s](mm_enum e) { s << e; return true; });
        BOOST_CHECK_EQUAL("ABC", s.str());
    }

    BOOST_CHECK(mm_enum::A == mm_enum::from_string("A"));
    BOOST_CHECK(mm_enum::B == mm_enum::from_string("B"));
    BOOST_CHECK(mm_enum::C == mm_enum::from_string("C"));
    BOOST_CHECK(mm_enum::UNDEFINED == mm_enum::from_string("D"));

    static_assert(2 == oh_mm::mm_enum2::size(), "Invalid size");
    BOOST_CHECK_EQUAL("X", oh_mm::mm_enum2::to_string(oh_mm::mm_enum2::X));
    BOOST_CHECK_EQUAL("Y", oh_mm::mm_enum2::to_string(oh_mm::mm_enum2::Y));

    BOOST_CHECK(oh_mm::mm_enum2::X == oh_mm::mm_enum2::from_string("X"));
    BOOST_CHECK(oh_mm::mm_enum2::Y == oh_mm::mm_enum2::from_string("Y"));
    BOOST_CHECK(oh_mm::mm_enum2::UNDEFINED == oh_mm::mm_enum2::from_string("D"));
}

BOOST_AUTO_TEST_CASE( test_enum_old )
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
    static_assert(3 == mm_enumx::size(), "Invalid size");

    mm_enumx v;

    BOOST_CHECK(v.empty());

    BOOST_CHECK_EQUAL(' ',     (char)mm_enumx::UNDEFINED);
    BOOST_CHECK_EQUAL(mm_enumx::A,   mm_enumx('a'));
    BOOST_CHECK_EQUAL(mm_enumx::BB,  mm_enumx('b'));
    BOOST_CHECK_EQUAL(mm_enumx::CCC, mm_enumx('c'));
    BOOST_CHECK_EQUAL("A",           mm_enumx::to_string(mm_enumx::A));
    BOOST_CHECK_EQUAL("BB",          mm_enumx::to_string(mm_enumx::BB));
    BOOST_CHECK_EQUAL("CCC",         mm_enumx::to_string(mm_enumx::CCC));
    BOOST_CHECK_EQUAL("A",           mm_enumx::from_string("A").to_string());

    {
        mm_enumx val = mm_enumx::from_string("BB");
        BOOST_CHECK_EQUAL("BB", val.to_string());
        std::stringstream s; s << mm_enumx::to_string(val);
        BOOST_CHECK_EQUAL("BB", s.str());
    }

    {
        // Iterate over all enum values defined in mm_enum type:
        std::stringstream s;
        mm_enumx::for_each([&s](mm_enumx e, std::string const& name) {
            s << e;
            return true;
        });
        BOOST_CHECK_EQUAL("ABBCCC", s.str());
    }

    BOOST_CHECK(mm_enumx::A   == mm_enumx::from_string("A"));
    BOOST_CHECK(mm_enumx::BB  == mm_enumx::from_string("BB"));
    BOOST_CHECK(mm_enumx::CCC == mm_enumx::from_string("CCC"));
    BOOST_CHECK(mm_enumx::UNDEFINED == mm_enumx::from_string("D"));

    static_assert(3 == mm_enumx::size(), "Invalid size");
}

BOOST_AUTO_TEST_CASE( test_enumx_old )
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
    static_assert(5 == mm_flags::size(), "Invalid size");

    mm_flags v;

    BOOST_CHECK(v.empty());
    BOOST_CHECK(v == mm_flags::NONE);

    v |= mm_flags::B;

    BOOST_CHECK_EQUAL(0,        mm_flags::NONE);
    BOOST_CHECK_EQUAL(1u << 0,  mm_flags::A);
    BOOST_CHECK_EQUAL(1u << 1,  mm_flags::B);
    BOOST_CHECK_EQUAL("A",      mm_flags(mm_flags::A).to_string());
    BOOST_CHECK_EQUAL("B",      mm_flags(mm_flags::B).to_string());
    BOOST_CHECK_EQUAL("A|C",    mm_flags(mm_flags::A, mm_flags::C).to_string());
    BOOST_CHECK_EQUAL("A",      mm_flags::from_string("A").to_string());

    BOOST_CHECK_THROW(mm_flags::from_string("A|F"), utxx::badarg_error);

    {
        mm_flags val = mm_flags::from_string("A|B|E");
        BOOST_CHECK_EQUAL("A|B|E", val.to_string());
        std::stringstream s; s << mm_flags::to_string(val);
        BOOST_CHECK_EQUAL("A|B|E", s.str());
    }

    v |= mm_flags::E;

    BOOST_CHECK( v.has_all(mm_flags::B | mm_flags::E));
    BOOST_CHECK(!v.has_all(mm_flags::A | mm_flags::B | mm_flags::E));
    BOOST_CHECK( v.has_all(mm_flags::B));
    BOOST_CHECK( v.has_all(mm_flags::E));
    BOOST_CHECK( v.has_any(mm_flags::B | mm_flags::E));
    BOOST_CHECK( v.has_any(mm_flags::B));
    BOOST_CHECK( v.has_any(mm_flags::E));
    BOOST_CHECK( v.has(mm_flags::B));
    BOOST_CHECK( v.has(mm_flags::E));

    {
        // Iterate over all enum values defined in my_enum type:
        std::stringstream s;
        v.for_each([&s](mm_flags e) {
            s << e;
            return true;
        });
        BOOST_CHECK_EQUAL("BE", s.str());
    }

    v.clear();
    BOOST_CHECK(v.empty());
    v = mm_flags::B | mm_flags::C | mm_flags::E;
    v.clear(mm_flags::C | mm_flags::E);
    BOOST_CHECK(v == mm_flags::B);
}

BOOST_AUTO_TEST_CASE( test_enum_flags_old )
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

#endif
