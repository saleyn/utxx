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
#include <utxx/enumv.hpp>
#include <utxx/enum_flags.hpp>
#include <iostream>

//------------------------------------------------------------------------------
// Using UTXX_ENUM, UTXX_ENUMV, UTXX_ENUM_FLAGS
//------------------------------------------------------------------------------

UTXX_ENUM( mm_enum,   int64_t,      A,  B,  C);
UTXX_ENUM( mm_enum2, (int64_t, -2), A,  B,  C);
UTXX_ENUM( mm_enum3, (char,Nil,-3), A,  B,  C);
UTXX_ENUM( mm_enum4, (char,Nil,-3),(A,"AA") (B) (C));
UTXX_ENUM( mm_enumz, (int,     -1),(A) (B) (C));
UTXX_ENUM( mm_enumz2, int,         (A) (B) (C));
UTXX_ENUM( mm_enumz3,(int, Nil,-3),(A) (B) (C));
UTXX_ENUM( mm_enumz4,(char,Nil,-3),(A,"AA")(B)(C));
UTXX_ENUM( mm_enumz5,(char,Nil,-3), A);
UTXX_ENUM( mm_enumz6,(char,Nil,-3),(A));
UTXX_ENUM( mm_enumz7,(char,Nil,-3),(A,"a"));
UTXX_ENUMV(mmSideT,  int8_t,    -1, (BID)(ASK)(SIDES));
UTXX_ENUMV(mm_enumv, char,     ' ', (A, 'a', "AAA")(BB, 'b')(CCC));
UTXX_ENUM_FLAGS(mm_flags, uint8_t,
    A,
    B,
    C,
    D,
    E
);
UTXX_ENUM_FLAGZ(mm_flagz, uint8_t, Nil,
    (A)
    (B, "bb")
    (C, "Cc")
    (D)
);

// Define an enum my_enum2 inside a struct:
struct oh_mm { UTXX_ENUM(mm_enum2, char, X, Y); };

BOOST_AUTO_TEST_CASE( test_enum )
{
    static_assert(3 == mm_enum::size(),         "Invalid size");
    static_assert(1 == sizeof(mm_enumv),        "Invalid size");
    static_assert(1 == sizeof(mmSideT),         "Invalid size");
    static_assert(8 == sizeof(mm_enum),         "Invalid size");
    static_assert(1 == sizeof(oh_mm::mm_enum2), "Invalid size");

    static_assert(0  == mm_enum::UNDEFINED,     "Invalid value");
    static_assert(-2 == mm_enum2::UNDEFINED,    "Invalid value");
    static_assert(-3 == mm_enum3::Nil,          "Invalid value");
    static_assert(-1 == mm_enumz::UNDEFINED,    "Invalid value");
    static_assert(0  == mm_enumz2::UNDEFINED,   "Invalid value");
    static_assert(1  == mm_enumz5::size(),      "Invalid value");
    static_assert(1  == mm_enumz6::size(),      "Invalid value");
    static_assert(1  == mm_enumz7::size(),      "Invalid value");
    static_assert(-2 == mm_enumz5::A,           "Invalid value");
    static_assert(-2 == mm_enumz6::A,           "Invalid value");
    static_assert(-2 == mm_enumz7::A,           "Invalid value");

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

    BOOST_CHECK_EQUAL("a", mm_enumz7(mm_enumz7::A).to_string());
    BOOST_CHECK_EQUAL("a", mm_enumz7::from_string("a").to_string());
    BOOST_CHECK_EQUAL("A", mm_enumz7(mm_enumz7::A).name());
    BOOST_CHECK_EQUAL("a", mm_enumz7(mm_enumz7::A).value());

    {
        mm_enum val = mm_enum::from_string("B");
        BOOST_CHECK_EQUAL("B", val.to_string());
        std::stringstream s; s << mm_enum::to_string(val);
        BOOST_CHECK_EQUAL("B", s.str());

        auto val1 = mm_enumv::from_string("Bb", true);
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

    BOOST_CHECK_EQUAL(-1, mm_enumz::UNDEFINED);
    BOOST_CHECK_EQUAL( 0, mm_enumz::A);
    BOOST_CHECK_EQUAL( 0, (int)mm_enumz::from_string("A"));
    BOOST_CHECK(mm_enumz::A == mm_enumz::from_string("A"));
    BOOST_CHECK_EQUAL("UNDEFINED", mm_enumz::to_string(mm_enumz()));
    BOOST_CHECK_EQUAL("A",         mm_enumz::to_string(mm_enumz::A));
    BOOST_CHECK_EQUAL("B",         mm_enumz::to_string(mm_enumz::B));
    BOOST_CHECK_EQUAL("C",         mm_enumz::to_string(mm_enumz::C));
    BOOST_CHECK(mm_enumz::A == mm_enumz::begin());
    BOOST_CHECK(mm_enumz::C == mm_enumz::last());
    BOOST_CHECK_EQUAL( 3, (int)mm_enumz::end());

    static_assert(2 == oh_mm::mm_enum2::size(), "Invalid size");
    BOOST_CHECK_EQUAL("X", oh_mm::mm_enum2::to_string(oh_mm::mm_enum2::X));
    BOOST_CHECK_EQUAL("Y", oh_mm::mm_enum2::to_string(oh_mm::mm_enum2::Y));

    BOOST_CHECK(oh_mm::mm_enum2::X == oh_mm::mm_enum2::from_string("X"));
    BOOST_CHECK(oh_mm::mm_enum2::Y == oh_mm::mm_enum2::from_string("Y"));
    BOOST_CHECK(oh_mm::mm_enum2::UNDEFINED == oh_mm::mm_enum2::from_string("D"));
}

BOOST_AUTO_TEST_CASE( test_enumv )
{
    static_assert(3 == mm_enumv::size(), "Invalid size");

    mm_enumv v;

    BOOST_CHECK(v.empty());

    BOOST_CHECK_EQUAL(' ',     (char)mm_enumv::UNDEFINED);
    BOOST_CHECK_EQUAL(mm_enumv::A,   mm_enumv('a'));
    BOOST_CHECK_EQUAL(mm_enumv::BB,  mm_enumv('b'));
    BOOST_CHECK_EQUAL(mm_enumv::CCC, mm_enumv('c'));
    BOOST_CHECK_EQUAL("AAA",         mm_enumv::to_string(mm_enumv::A));
    BOOST_CHECK_EQUAL("BB",          mm_enumv::to_string(mm_enumv::BB));
    BOOST_CHECK_EQUAL("CCC",         mm_enumv::to_string(mm_enumv::CCC));
    BOOST_CHECK_EQUAL("UNDEFINED",   mm_enumv::from_string("A").to_string());
    BOOST_CHECK_EQUAL("AAA",         mm_enumv::from_string("AAA").to_string());
    BOOST_CHECK_EQUAL("A",           mm_enumv(mm_enumv::A).name());
    BOOST_CHECK_EQUAL("AAA",         mm_enumv(mm_enumv::A).value());
    BOOST_CHECK_EQUAL('a',           mm_enumv(mm_enumv::A).code());
    BOOST_CHECK_EQUAL("BB",          mm_enumv(mm_enumv::BB).name());
    BOOST_CHECK_EQUAL("BB",          mm_enumv(mm_enumv::BB).value());
    BOOST_CHECK_EQUAL('b',           mm_enumv(mm_enumv::BB).code());
    BOOST_CHECK_EQUAL("CCC",         mm_enumv(mm_enumv::CCC).name());
    BOOST_CHECK_EQUAL("CCC",         mm_enumv(mm_enumv::CCC).value());
    BOOST_CHECK_EQUAL('c',           mm_enumv(mm_enumv::CCC).code());

    {
        mm_enumv val = mm_enumv::from_string("BB");
        BOOST_CHECK_EQUAL("BB", val.to_string());
        std::stringstream s; s << mm_enumv::to_string(val);
        BOOST_CHECK_EQUAL("BB", s.str());
    }

    {
        // Iterate over all enum values defined in mm_enum type:
        std::stringstream s;
        mm_enumv::for_each([&s](mm_enumv::type e, auto& name_pair) {
            s << e;
            return true;
        });
        BOOST_CHECK_EQUAL("AAABBCCC", s.str());

        s.str("");
        s.clear();

        mm_enumv::for_each([&s](mm_enumv::type e, auto& name_pair) {
            s << name_pair.first;
            return true;
        });
        BOOST_CHECK_EQUAL("ABBCCC", s.str());

        s.str("");
        s.clear();

        mm_enumv::for_each([&s](mm_enumv::type e, auto& name_pair) {
            s << name_pair.second;
            return true;
        });
        BOOST_CHECK_EQUAL("AAABBCCC", s.str());

    }

    BOOST_CHECK(mm_enumv::A   == mm_enumv::from_string("AAA"));
    BOOST_CHECK(mm_enumv::A   == mm_enumv::from_string_nc("aaa"));
    BOOST_CHECK(mm_enumv::A   == mm_enumv::from_string_nc("a", true));
    BOOST_CHECK(mm_enumv::A   == mm_enumv::from_name("A"));
    BOOST_CHECK(mm_enumv::A   == mm_enumv::from_name("a", true));
    BOOST_CHECK(mm_enumv::A   == mm_enumv::from_value("AAA"));
    BOOST_CHECK(mm_enumv::A   == mm_enumv::from_value("aaa", true));
    BOOST_CHECK(mm_enumv::BB  == mm_enumv::from_string("BB"));
    BOOST_CHECK(mm_enumv::CCC == mm_enumv::from_string("CCC"));
    BOOST_CHECK(mm_enumv::UNDEFINED == mm_enumv::from_string("D"));

    static_assert(3 == mm_enumv::size(), "Invalid size");
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
    BOOST_CHECK_EQUAL(1u << 2,  mm_flags::C);
    BOOST_CHECK_EQUAL(1u << 3,  mm_flags::D);
    BOOST_CHECK_EQUAL(1u << 4,  mm_flags::E);
    BOOST_CHECK_EQUAL("A",      mm_flags(mm_flags::A).to_string());
    BOOST_CHECK_EQUAL("B",      mm_flags(mm_flags::B).to_string());
    BOOST_CHECK_EQUAL("A|C",    mm_flags(mm_flags::A, mm_flags::C).to_string());
    BOOST_CHECK_EQUAL("A",      mm_flags::from_names("A").to_string());

    BOOST_CHECK_THROW(mm_flags::from_string("A|F"), utxx::badarg_error);
    BOOST_CHECK_THROW(mm_flags::from_names("A|F"),  utxx::badarg_error);
    BOOST_CHECK_THROW(mm_flags::from_values("A|F"), utxx::badarg_error);

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
    BOOST_CHECK(mm_flags::_END_ == int(mm_flags::E) << 1);
    BOOST_CHECK_EQUAL("A",      mm_flagz(mm_flagz::A).to_string());
    BOOST_CHECK_EQUAL("bb",     mm_flagz(mm_flagz::B).to_string());
    BOOST_CHECK_EQUAL("B",      mm_flagz(mm_flagz::B).names());
    BOOST_CHECK_EQUAL("bb",     mm_flagz(mm_flagz::B).values());
    BOOST_CHECK_EQUAL("A|Cc",   mm_flagz(mm_flagz::A, mm_flagz::C).to_string());
    BOOST_CHECK_EQUAL("A|C",    mm_flagz(mm_flagz::A, mm_flagz::C).names());
    BOOST_CHECK_EQUAL("A|Cc",   mm_flagz(mm_flagz::A, mm_flagz::C).values());
    BOOST_CHECK_EQUAL("bb",     mm_flagz::from_names("B").to_string());
    BOOST_CHECK_EQUAL("bb",     mm_flagz::from_values("bb").to_string());
    BOOST_CHECK_EQUAL("B",      mm_flagz::from_values("bb").names());
    BOOST_CHECK_EQUAL("A|Cc",   mm_flagz::from_values("A|Cc").values());

    BOOST_CHECK_THROW(mm_flags::from_string("A|F"), utxx::badarg_error);
    BOOST_CHECK_THROW(mm_flags::from_names("A|F"),  utxx::badarg_error);
    BOOST_CHECK_THROW(mm_flags::from_values("A|F"), utxx::badarg_error);

    {
        mm_flags val = mm_flags::from_string("A|B|E");
        BOOST_CHECK_EQUAL("A|B|E", val.to_string());
        std::stringstream s; s << mm_flags::to_string(val);
        BOOST_CHECK_EQUAL("A|B|E", s.str());
    }
}

#endif
