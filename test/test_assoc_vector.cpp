//----------------------------------------------------------------------------
/// \file  test_assoc_vector.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating atomic.hpp functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-04-15
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

Copyright (C) 2015 Serge Aleynikov <saleyn@gmail.com>

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
#include <utxx/container/assoc_vector.hpp>
#include <stdio.h>

using namespace std;
using namespace utxx;

BOOST_AUTO_TEST_CASE( test_assoc_vector )
{
    assoc_vector<int, string> v{{1,"a"}, {2, "b"}, {4, "c"}, {7, "d"}};

    auto it = v.find(2);
    BOOST_CHECK(it != v.end());
    BOOST_CHECK_EQUAL("b", it->second);
    it = v.find(4);
    BOOST_CHECK_EQUAL("c", it->second);

    it = v.lower_bound(3);
    BOOST_CHECK_EQUAL(4, it->first);
    it = v.upper_bound(3);
    BOOST_CHECK_EQUAL(4, it->first);
}
