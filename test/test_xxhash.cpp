//----------------------------------------------------------------------------
/// \file  test_xxhash.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for xxhash.hpp
//----------------------------------------------------------------------------
// Copyright (c) 2020 Serge Aleynikov <saleyn@gmail.com>
// Created: 2020-06-10
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
#include <utxx/xxhash.hpp>

BOOST_AUTO_TEST_CASE( test_xxhash )
{
    auto seed = 1u;
    std::string input("Test data for hashing");

    BOOST_REQUIRE_EQUAL(xxh::xxhash<32>(input.data(), input.size(), seed), utxx::xxhash<32>(input, seed));
    BOOST_REQUIRE_EQUAL(xxh::xxhash<64>(input.data(), input.size(), seed), utxx::xxhash<64>(input, seed));
}
