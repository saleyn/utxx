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

