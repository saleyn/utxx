//----------------------------------------------------------------------------
/// \file  test_string.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the variant.hpp and test_variant.hpp.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-07-10
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

#include <utxx/string.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_string_conversion )
{
    {
        std::stringstream s; s << fixed(10.123, 8, 4);
        BOOST_CHECK_EQUAL(" 10.1230", s.str());
    }
    {
        std::stringstream s; s << fixed(10.123, 8, 4, '0');
        BOOST_CHECK_EQUAL("010.1230", s.str());
    }
}

BOOST_AUTO_TEST_CASE( test_string_length )
{
    char const* s_values[] = {"One", "Two", "Three"};
    static_assert(length(s_values) == 3, "Wrong length");

    const char s[] = "abc";
    static_assert(length(s) == 3, "Wrong length");
    BOOST_REQUIRE_EQUAL(3u, length(s));
    
    static const char* s_ops[] = {"a", "b", "c"};
    BOOST_REQUIRE_EQUAL(3u, length(s_ops));

    struct t { int a; t(int a) : a(a) {} };

    const t arr[] = { t(1), t(2) };
    BOOST_REQUIRE_EQUAL(2u, length(arr));
}

enum op_type {OP_UNDEFINED = -1, OP_ADD, OP_REMOVE, OP_REPLACE, OP_UPDATE};

BOOST_AUTO_TEST_CASE( test_string_find_pos )
{
    static const char* s_test   = "abc\n   ";
    static const char* s_end    = s_test + 7;
    static const char* s_expect = s_test + 3;
    BOOST_REQUIRE_EQUAL(s_expect, utxx::find_pos(s_test, s_end, '\n'));
    BOOST_REQUIRE_EQUAL(s_end,    utxx::find_pos(s_test, s_end, 'X'));
    BOOST_REQUIRE_EQUAL(s_end,    utxx::find_pos(s_test, s_end, '\0'));
}

BOOST_AUTO_TEST_CASE( test_string_find_index )
{
    static const char* s_ops[] = {"add", "remove", "replace", "update"};
    BOOST_CHECK(OP_REMOVE    == utxx::find_index<op_type>(s_ops, "remove  ", 6));
    BOOST_CHECK(OP_UNDEFINED == utxx::find_index<op_type>(s_ops, "remove  ", 0));
    BOOST_CHECK(OP_REPLACE   == utxx::find_index<op_type>(s_ops, sizeof(s_ops), "replace"));
    BOOST_CHECK(OP_REPLACE   == utxx::find_index<op_type>(s_ops, sizeof(s_ops), "replace ", 7));
    BOOST_CHECK(OP_UNDEFINED == utxx::find_index<op_type>(s_ops, "xxx"));
    BOOST_CHECK(OP_ADD       == utxx::find_index<op_type>(s_ops, "Add",    OP_UNDEFINED, true));
    BOOST_CHECK(OP_ADD       == utxx::find_index<op_type>(s_ops, "ADD", 3, OP_UNDEFINED, true));
    BOOST_CHECK(OP_ADD       == utxx::find_index_or_throw<op_type>(s_ops, "ADD", OP_UNDEFINED, true));
}

BOOST_AUTO_TEST_CASE( test_string_to_int64 )
{
    BOOST_REQUIRE_EQUAL(1u,       to_int64("\1"));
    BOOST_REQUIRE_EQUAL(258u,     to_int64("\1\2"));
    BOOST_REQUIRE_EQUAL(66051u,   to_int64("\1\2\3"));
    BOOST_REQUIRE_EQUAL(4276803u, to_int64("ABC"));
}

BOOST_AUTO_TEST_CASE( test_string_nocase )
{
    string_nocase s("AbcDe123");
    BOOST_REQUIRE_EQUAL(s, "abcde123");

    std::string s1("AbC");
    BOOST_REQUIRE_EQUAL("abc", to_lower(s1));
    BOOST_REQUIRE_EQUAL("ABC", to_upper(s1));
}

BOOST_AUTO_TEST_CASE( test_string_to_bin_string )
{
    {
        std::string s("abcdef01234");
        std::string b = to_bin_string(s.c_str(), s.size());
        std::string c("<<\"abcdef01234\">>");
        BOOST_REQUIRE_EQUAL(c, b);
    }
    {
        std::string s("8=FIX.4.2|9=71|35=A|34=93|49=CLIENT1|52=20120418-03:04:28.925|"
                      "56=EXECUTOR|98=0|108=10|10=151|");
        std::string b = to_bin_string(s.c_str(), s.size());
        std::string c("<<\"8=FIX.4.2|9=71|35=A|34=93|49=CLIENT1|52=20120418-03:04:28.925|"
                      "56=EXECUTOR|98=0|108=10|10=151|\">>");
        BOOST_REQUIRE_EQUAL(c, b);
        b = to_bin_string(s.c_str(), s.size(), false, false);
        std::string d("<<56,61,70,73,88,46,52,46,50,124,57,61,55,49,124,51,53,61,65,"
                    "124,51,52,61,57,51,124,52,57,61,67,76,73,69,78,84,49,124,53,50,"
                    "61,50,48,49,50,48,52,49,56,45,48,51,58,48,52,58,50,56,46,57,50,"
                    "53,124,53,54,61,69,88,69,67,85,84,79,82,124,57,56,61,48,124,49,"
                    "48,56,61,49,48,124,49,48,61,49,53,49,124>>");
        BOOST_REQUIRE_EQUAL(d, b);
    }
}



