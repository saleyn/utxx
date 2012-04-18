//----------------------------------------------------------------------------
/// \file  test_variant.cpp
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

#include <boost/test/unit_test.hpp>
#include <util/string.hpp>
#include <iostream>

using namespace util;

BOOST_AUTO_TEST_CASE( test_string_length )
{
    const char s[] = "abc";
    BOOST_REQUIRE_EQUAL(sizeof(s), length(s));
}

enum op_type {OP_UNDEFINED = -1, OP_ADD, OP_REMOVE, OP_REPLACE, OP_UPDATE};

BOOST_AUTO_TEST_CASE( test_string_find_index )
{
    static const char* s_ops[] = {"add", "remove", "replace", "update"};
    BOOST_REQUIRE(OP_REMOVE    == util::find_index<op_type>(s_ops, "remove"));
    BOOST_REQUIRE(OP_REPLACE   == util::find_index<op_type>(s_ops, sizeof(s_ops), "replace"));
    BOOST_REQUIRE(OP_UNDEFINED == util::find_index<op_type>(s_ops, "xxx"));
}

BOOST_AUTO_TEST_CASE( test_string_nocase )
{
    string_nocase s("AbcDe123");
    BOOST_REQUIRE_EQUAL(s, "abcde123");

    std::string s1("AbC");
    BOOST_REQUIRE_EQUAL("abc", to_lower(s1));
    BOOST_REQUIRE_EQUAL("ABC", to_upper(s1));
}

BOOST_AUTO_TEST_CASE( test_to_bin_string)
{
    std::string s("8=FIX.4.2|9=71|35=A|34=93|49=CLIENT1|52=20120418-03:04:28.925|"
                  "56=EXECUTOR|98=0|108=10|10=151|");
    std::string b = to_bin_string(s.c_str(), s.size());
    std::string c("<<56,61,70,73,88,46,52,46,50,124,57,61,55,49,124,51,53,61,65,"
                  "124,51,52,61,57,51,124,52,57,61,67,76,73,69,78,84,49,124,53,50,"
                  "61,50,48,49,50,48,52,49,56,45,48,51,58,48,52,58,50,56,46,57,50,"
                  "53,124,53,54,61,69,88,69,67,85,84,79,82,124,57,56,61,48,124,49,"
                  "48,56,61,49,48,124,49,48,61,49,53,49,124>>");
    BOOST_REQUIRE_EQUAL(c, b);
}

