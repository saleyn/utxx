//----------------------------------------------------------------------------
/// \file  test_atomic.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating atomic.hpp functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Copyright (c) 2011 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
// Created: 2011-10-24
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

Copyright (C) 2011 Serge Aleynikov <saleyn@gmail.com>
Copyright (c) 2011 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>

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

#include <utxx/iovec.hpp>

namespace utxx {
namespace io {
BOOST_AUTO_TEST_CASE(test_iovec)
{
    {
        // default constructor
        basic_iovector<5> v;
        BOOST_REQUIRE_EQUAL(0u, v.size());
        BOOST_REQUIRE_EQUAL(0u, v.length());
        BOOST_REQUIRE(v.empty());

        v.push_back("a", 1);
        BOOST_REQUIRE_EQUAL(1u, v.size());
        BOOST_REQUIRE_EQUAL(1u, v.length());

        v.push_back("b", 1);
        BOOST_REQUIRE_EQUAL(2u, v.size());
        BOOST_REQUIRE_EQUAL(2u, v.length());

        v.push_back("c", 2);
        BOOST_REQUIRE_EQUAL(3u, v.size());
        BOOST_REQUIRE_EQUAL(4u, v.length());

        // testing fn: void reset()
        BOOST_REQUIRE(!v.empty());
        v.reset();
        BOOST_REQUIRE(v.empty());
    }
}
} // namespace io
} // namespace utxx
