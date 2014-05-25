//----------------------------------------------------------------------------
/// \file  test_meta.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes and functions in the meta.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-05-06
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
#include <utxx/meta.hpp>
#include <iostream>

using namespace utxx;

#if __cplusplus >= 201103L

BOOST_AUTO_TEST_CASE( test_meta )
{
    enum class B { B1 = 1, B2 = 2 };

    BOOST_STATIC_ASSERT(1 == to_underlying(B::B1));
    BOOST_REQUIRE_EQUAL(1, to_underlying(B::B1));
}

#endif
