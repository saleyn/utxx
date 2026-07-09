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
#include <utxx/config.h>
#ifdef UTXX_HAVE_BOOST

#include <utxx/test_helper.hpp>
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
UTXX_ENUM( mm_enum5, (int64_t, Nil, -2, 0), A,  B,  C);
UTXX_ENUM( mm_enum6, (int64_t, Nil,  3, 0), A,  B,  C);
UTXX_ENUM( mm_enumz, (int,     -1),(A) (B) (C));
UTXX_ENUM( mm_enumz2, int,         (A) (B) (C));
UTXX_ENUM( mm_enumz3,(int, Nil,-3),(A) (B) (C));
UTXX_ENUM( mm_enumz4,(char,Nil,-3),(A,"AA")(B)(C));
UTXX_ENUM( mm_enumz5,(char,Nil,-3), A);
UTXX_ENUM( mm_enumz6,(char,Nil,-3),(A));
UTXX_ENUM( mm_enumz7,(char,Nil,-3),(A,"a"));
UTXX_ENUMV(mmSideT,  (int8_t,  -1), (BID)(ASK)(SIDES));
UTXX_ENUMV(mm_enumv, (char,   ' '), (A, 'a', "AAA")(BB, 'b')(CCC));
UTXX_ENUMV(mm_enumu, (char,   ' '), (A, 'a', "AAA")(BB, 'b')(CCC));
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

UTXX_ENUM(mm_large, int,
    A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17,
    A18, A19, A20, A21, A22, A23, A24, A25, A26, A27, A28, A29, A30, A31, A32,
    A33, A34, A35, A36, A37, A38, A39, A40, A41, A42, A43, A44, A45, A46, A47,
    A48, A49, A50, A51, A52, A53, A54, A55, A56, A57, A58, A59, A60, A61, A62,
    A63, A64);

UTXX_ENUMU(mm_enumu1, uint8_t, (A,  (uint8_t)1)  (B,  (uint8_t)2));
UTXX_ENUMU(mm_enumu2, uint8_t, (A, ((uint8_t)1)) (B, ((uint8_t)2)));
UTXX_ENUMU(mm_enumu3, (int, NULL_VALUE, (uint8_t)255, (uint8_t)255+1, false),
    (B, (uint8_t)0)
    (P, (uint8_t)1)
);

UTXX_ENUMU
(mm_enumu4, (char, NULL_VALUE, (char)0, (char)0+1, false),
    (A, (char)48)
    (B, (char)49)
    (C, (char)69)
    (D, (char)70)
    (E, (char)74)
);

UTXX_ENUMU
(mm_enumu5, (uint8_t, NULL_VALUE, (uint8_t)255, (uint8_t)255+1),
    (A, (uint8_t)0)
    (B, (uint8_t)1)
);

int pass_enumu3_arg(mm_enumu3 a)
{
    return a.code();
}

// Define an enum my_enum2 inside a struct:
struct oh_mm { UTXX_ENUM(mm_enum2, char, X, Y); };

TEST_CASE( test_enum )
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

    CHECK(v.empty());

    CHECK_EQUAL(0,               (int)mm_enum::UNDEFINED);
    CHECK_EQUAL(mm_enum::UNDEFINED,   mm_enum(0));
    CHECK_EQUAL(mm_enum::A,           mm_enum(1));
    CHECK_EQUAL(mm_enum::A,           mm_enum::begin());
    CHECK_EQUAL(mm_enum::C,           mm_enum::last());
    CHECK_EQUAL(mm_enum::_END_,       mm_enum::end());
    CHECK_EQUAL(1+int(mm_enum::C),    mm_enum::end());
    CHECK_EQUAL("A", mm_enum::to_string(mm_enum::A));
    CHECK_EQUAL("B", mm_enum::to_string(mm_enum::B));
    CHECK_EQUAL("C", mm_enum::to_string(mm_enum::C));
    CHECK_EQUAL("A", mm_enum::from_string("A").to_string());

    CHECK(!mm_enumz::valid(-2));
    CHECK( mm_enumz::valid(-1));
    CHECK( mm_enumz::valid(0));
    CHECK( mm_enumz::valid(1));
    CHECK( mm_enumz::valid(2));
    CHECK(!mm_enumz::valid(3));

    CHECK_EQUAL(mm_enum5::Nil,         mm_enum5(-2));
    CHECK_EQUAL(mm_enum5::A,           mm_enum5(0));
    CHECK_EQUAL(mm_enum5::A,           mm_enum5::begin());
    CHECK_EQUAL(mm_enum5::C,           mm_enum5::last());
    CHECK_EQUAL(mm_enum5::_END_,       mm_enum5::end());
    CHECK_EQUAL(1+int(mm_enum5::C),    mm_enum5::end());
    CHECK_EQUAL("A", mm_enum5::from_string("A").name());
    CHECK_EQUAL("A", mm_enum5::from_string("A").to_string());

    CHECK_EQUAL(mm_enum6::Nil,         mm_enum6(3));
    CHECK_EQUAL(mm_enum6::A,           mm_enum6(0));
    CHECK_EQUAL(mm_enum6::A,           mm_enum6::begin());
    CHECK_EQUAL(mm_enum6::C,           mm_enum6::last());
    CHECK_EQUAL(mm_enum6::_END_,       mm_enum6::end());
    CHECK_EQUAL(1+int(mm_enum6::C),    mm_enum6::end());
    CHECK_EQUAL("A", mm_enum6::from_string("A").name());
    CHECK_EQUAL("A", mm_enum6::from_string("A").to_string());

    CHECK_EQUAL("a", mm_enumz7(mm_enumz7::A).to_string());
    CHECK_EQUAL("a", mm_enumz7::from_string("a").to_string());
    CHECK_EQUAL("A", mm_enumz7(mm_enumz7::A).name());
    CHECK_EQUAL("a", mm_enumz7(mm_enumz7::A).value());

    {
        mm_enum val = mm_enum::from_string("B");
        CHECK_EQUAL("B", val.to_string());
        std::stringstream s; s << mm_enum::to_string(val);
        CHECK_EQUAL("B", s.str());

        auto val1 = mm_enumv::from_string("Bb", true);
        CHECK_EQUAL('b', (char)val1);
    }

    {
        // Iterate over all enum values defined in mm_enum type:
        std::stringstream s;
        mm_enum::for_each([&s](mm_enum e) { s << e; return true; });
        CHECK_EQUAL("ABC", s.str());
    }

    CHECK(mm_enum::A == mm_enum::from_string("A"));
    CHECK(mm_enum::B == mm_enum::from_string("B"));
    CHECK(mm_enum::C == mm_enum::from_string("C"));
    CHECK(mm_enum::UNDEFINED == mm_enum::from_string("D"));

    CHECK_EQUAL(-1, mm_enumz::UNDEFINED);
    CHECK_EQUAL( 0, mm_enumz::A);
    CHECK_EQUAL( 0, (int)mm_enumz::from_string("A"));
    CHECK(mm_enumz::A == mm_enumz::from_string("A"));
    CHECK_EQUAL("UNDEFINED", mm_enumz::to_string(mm_enumz()));
    CHECK_EQUAL("A",         mm_enumz::to_string(mm_enumz::A));
    CHECK_EQUAL("B",         mm_enumz::to_string(mm_enumz::B));
    CHECK_EQUAL("C",         mm_enumz::to_string(mm_enumz::C));
    CHECK(mm_enumz::A == mm_enumz::begin());
    CHECK(mm_enumz::C == mm_enumz::last());
    CHECK_EQUAL( 3, (int)mm_enumz::end());

    static_assert(2 == oh_mm::mm_enum2::size(), "Invalid size");
    CHECK_EQUAL("X", oh_mm::mm_enum2::to_string(oh_mm::mm_enum2::X));
    CHECK_EQUAL("Y", oh_mm::mm_enum2::to_string(oh_mm::mm_enum2::Y));

    CHECK(oh_mm::mm_enum2::X == oh_mm::mm_enum2::from_string("X"));
    CHECK(oh_mm::mm_enum2::Y == oh_mm::mm_enum2::from_string("Y"));
    CHECK(oh_mm::mm_enum2::UNDEFINED == oh_mm::mm_enum2::from_string("D"));

    {
        mm_enumu3 en1(0);

        en1 = 1;

        auto fun = [](mm_enumu3 v) { return v == mm_enumu3(1); };
        auto ok  = fun(en1);
        CHECK(ok);

        mm_enumu3 en3(mm_enumu3::NULL_VALUE);
        CHECK_EQUAL(mm_enumu3::NULL_VALUE, en3);
        CHECK_EQUAL(255, int(mm_enumu3::NULL_VALUE));
        int res = pass_enumu3_arg(en3);
        CHECK_EQUAL(255, res);

        auto en5 = mm_enumu5::from_string("A");
        CHECK(mm_enumu5::A == en5);
        en5 = mm_enumu5(0);
        CHECK(mm_enumu5::A == en5);
    }
}

TEST_CASE( test_enumv )
{
    static_assert(3 == mm_enumv::size(), "Invalid size");

    mm_enumv v;

    CHECK(v.empty());

    CHECK_EQUAL(' ',     (char)mm_enumv::UNDEFINED);
    CHECK_EQUAL(mm_enumv::A,   mm_enumv('a'));
    CHECK_EQUAL(mm_enumv::BB,  mm_enumv('b'));
    CHECK_EQUAL(mm_enumv::CCC, mm_enumv('c'));
    CHECK_EQUAL("AAA",         mm_enumv::to_string(mm_enumv::A));
    CHECK_EQUAL("BB",          mm_enumv::to_string(mm_enumv::BB));
    CHECK_EQUAL("CCC",         mm_enumv::to_string(mm_enumv::CCC));
    CHECK_EQUAL("UNDEFINED",   mm_enumv::from_string("A").to_string());
    CHECK_EQUAL("AAA",         mm_enumv::from_string("AAA").to_string());
    CHECK_EQUAL("A",           mm_enumv(mm_enumv::A).name());
    CHECK_EQUAL("AAA",         mm_enumv(mm_enumv::A).value());
    CHECK_EQUAL('a',           mm_enumv(mm_enumv::A).code());
    CHECK_EQUAL("BB",          mm_enumv(mm_enumv::BB).name());
    CHECK_EQUAL("BB",          mm_enumv(mm_enumv::BB).value());
    CHECK_EQUAL('b',           mm_enumv(mm_enumv::BB).code());
    CHECK_EQUAL("CCC",         mm_enumv(mm_enumv::CCC).name());
    CHECK_EQUAL("CCC",         mm_enumv(mm_enumv::CCC).value());
    CHECK_EQUAL('c',           mm_enumv(mm_enumv::CCC).code());

    CHECK( mm_enumv::valid(' '));
    CHECK( mm_enumv::valid('a'));
    CHECK( mm_enumv::valid('b'));
    CHECK( mm_enumv::valid('c'));
    CHECK(!mm_enumv::valid('d'));

    CHECK( mm_enumu::valid(' '));
    CHECK( mm_enumu::valid('a'));
    CHECK( mm_enumu::valid('b'));
    CHECK( mm_enumu::valid('c'));
    CHECK(!mm_enumu::valid('d'));

    {
        mm_enumv val = mm_enumv::from_string("BB");
        CHECK_EQUAL("BB", val.to_string());
        std::stringstream s; s << mm_enumv::to_string(val);
        CHECK_EQUAL("BB", s.str());
    }

    {
        // Iterate over all enum values defined in mm_enum type:
        std::stringstream s;
        mm_enumv::for_each([&s](mm_enumv::type e, auto& name_pair) {
            s << e;
            return true;
        });
        CHECK_EQUAL("AAABBCCC", s.str());

        s.str("");
        s.clear();

        mm_enumv::for_each([&s](mm_enumv::type e, auto& name_pair) {
            s << name_pair.first;
            return true;
        });
        CHECK_EQUAL("ABBCCC", s.str());

        s.str("");
        s.clear();

        mm_enumv::for_each([&s](mm_enumv::type e, auto& name_pair) {
            s << name_pair.second;
            return true;
        });
        CHECK_EQUAL("AAABBCCC", s.str());

    }

    CHECK(mm_enumv::A   == mm_enumv::from_string("AAA"));
    CHECK(mm_enumv::A   == mm_enumv::from_string_nc("aaa"));
    CHECK(mm_enumv::A   == mm_enumv::from_string_nc("a", true));
    CHECK(mm_enumv::A   == mm_enumv::from_name("A"));
    CHECK(mm_enumv::A   == mm_enumv::from_name("a", true));
    CHECK(mm_enumv::A   == mm_enumv::from_value("AAA"));
    CHECK(mm_enumv::A   == mm_enumv::from_value("aaa", true));
    CHECK(mm_enumv::BB  == mm_enumv::from_string("BB"));
    CHECK(mm_enumv::CCC == mm_enumv::from_string("CCC"));
    CHECK(mm_enumv::UNDEFINED == mm_enumv::from_string("D"));

    static_assert(3 == mm_enumv::size(), "Invalid size");
}

TEST_CASE( test_enum_flags )
{
    static_assert(5 == mm_flags::size(), "Invalid size");

    mm_flags v(3);
    CHECK(int(v) == 3);
    CHECK(v.has_all(mm_flags::A | mm_flags::B));

    v.clear();
    CHECK(v.empty());
    CHECK(v == mm_flags::NONE);

    v |= mm_flags::B;

    CHECK_EQUAL(0,        mm_flags::NONE);
    CHECK_EQUAL(1u << 0,  mm_flags::A);
    CHECK_EQUAL(1u << 1,  mm_flags::B);
    CHECK_EQUAL(1u << 2,  mm_flags::C);
    CHECK_EQUAL(1u << 3,  mm_flags::D);
    CHECK_EQUAL(1u << 4,  mm_flags::E);
    CHECK_EQUAL("A",      mm_flags(mm_flags::A).to_string());
    CHECK_EQUAL("B",      mm_flags(mm_flags::B).to_string());
    CHECK_EQUAL("A|C",    mm_flags(mm_flags::A, mm_flags::C).to_string());
    CHECK_EQUAL("A",      mm_flags::from_names("A").to_string());

    CHECK_THROW(mm_flags::from_string("A|F"), utxx::badarg_error);
    CHECK_THROW(mm_flags::from_names("A|F"),  utxx::badarg_error);
    CHECK_THROW(mm_flags::from_values("A|F"), utxx::badarg_error);

    {
        mm_flags val = mm_flags::from_string("A|B|E");
        CHECK_EQUAL("A|B|E", val.to_string());
        std::stringstream s; s << mm_flags::to_string(val);
        CHECK_EQUAL("A|B|E", s.str());
    }

    v |= mm_flags::E;

    CHECK( v.has_all(mm_flags::B | mm_flags::E));
    CHECK(!v.has_all(mm_flags::A | mm_flags::B | mm_flags::E));
    CHECK( v.has_all(mm_flags::B));
    CHECK( v.has_all(mm_flags::E));
    CHECK( v.has_any(mm_flags::B | mm_flags::E));
    CHECK( v.has_any(mm_flags::B));
    CHECK( v.has_any(mm_flags::E));
    CHECK( v.has(mm_flags::B));
    CHECK( v.has(mm_flags::E));

    {
        // Iterate over all enum values defined in my_enum type:
        std::stringstream s;
        v.for_each([&s](mm_flags e) {
            s << e;
            return true;
        });
        CHECK_EQUAL("BE", s.str());
    }

    v.clear();
    CHECK(v.empty());
    v = mm_flags::B | mm_flags::C | mm_flags::E;
    v.clear(mm_flags::C | mm_flags::E);

    CHECK(v == mm_flags::B);
    CHECK(mm_flags::_END_ == int(mm_flags::E) << 1);
    CHECK_EQUAL("A",      mm_flagz(mm_flagz::A).to_string());
    CHECK_EQUAL("bb",     mm_flagz(mm_flagz::B).to_string());
    CHECK_EQUAL("B",      mm_flagz(mm_flagz::B).names());
    CHECK_EQUAL("bb",     mm_flagz(mm_flagz::B).values());
    CHECK_EQUAL("A|Cc",   mm_flagz(mm_flagz::A, mm_flagz::C).to_string());
    CHECK_EQUAL("A|C",    mm_flagz(mm_flagz::A, mm_flagz::C).names());
    CHECK_EQUAL("A|Cc",   mm_flagz(mm_flagz::A, mm_flagz::C).values());
    CHECK_EQUAL("bb",     mm_flagz::from_names("B").to_string());
    CHECK_EQUAL("bb",     mm_flagz::from_values("bb").to_string());
    CHECK_EQUAL("B",      mm_flagz::from_values("bb").names());
    CHECK_EQUAL("A|Cc",   mm_flagz::from_values("A|Cc").values());

    CHECK_THROW(mm_flags::from_string("A|F"), utxx::badarg_error);
    CHECK_THROW(mm_flags::from_names("A|F"),  utxx::badarg_error);
    CHECK_THROW(mm_flags::from_values("A|F"), utxx::badarg_error);

    {
        mm_flags val = mm_flags::from_string("A|B|E");
        CHECK_EQUAL("A|B|E", val.to_string());
        std::stringstream s; s << mm_flags::to_string(val);
        CHECK_EQUAL("A|B|E", s.str());
    }
}

#endif
