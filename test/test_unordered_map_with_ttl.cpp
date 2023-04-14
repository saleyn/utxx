//----------------------------------------------------------------------------
/// \file  test_unordered_map_with_ttl.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the unordered_map_with_ttl.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2022-04-15
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source projects

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
#include <utxx/unordered_map_with_ttl.hpp>
#include <utxx/string.hpp>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_unordered_map_with_ttl )
{
    unordered_map_with_ttl<int, int> map(1000);

    BOOST_REQUIRE(map.try_add(1, 123, 10000));
    BOOST_REQUIRE(map.try_add(2, 234, 10000));
    BOOST_REQUIRE_EQUAL(2, map.size());

    BOOST_CHECK( map.try_add(1, 123, 11000));
    BOOST_REQUIRE_EQUAL(1, map.size());
    BOOST_CHECK(!map.try_add(1, 123, 11500));
    BOOST_REQUIRE_EQUAL(1, map.size());
    BOOST_REQUIRE_EQUAL(1, map.refresh(12000));
    BOOST_REQUIRE_EQUAL(0, map.size());
    BOOST_CHECK( map.try_add(1, 123, 12000));
    BOOST_REQUIRE_EQUAL(1, map.size());
}


