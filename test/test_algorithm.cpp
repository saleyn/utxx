//----------------------------------------------------------------------------
/// \file  test_algorithm.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating algorithm.hpp functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2020-12-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

Copyright (C) 2011 Serge Aleynikov <saleyn@gmail.com>

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
#include <utxx/algorithm.hpp>
#include <utxx/error.hpp>
#include <set>

// Example program
#include <iostream>
#include <string>
#include <set>

namespace utxx {

BOOST_AUTO_TEST_CASE(test_algorithm_find_closest)
{
    std::set<int> empty_set;
    BOOST_CHECK(find_closest(empty_set, 5) == empty_set.end());

    std::set<int> set{ 3, 4, 7, 9 };

    auto x = find_closest(set, 1); //Returns '3'
    auto y = find_closest(set, 5); //Returns '4'
    auto a = find_closest(set, 6); //Returns '7'
    auto b = find_closest(set, 4); //Returns '4'
    auto c = find_closest(set, 10); //Returns '9'

    BOOST_CHECK_EQUAL(3, *x);
    BOOST_CHECK_EQUAL(4, *y);
    BOOST_CHECK_EQUAL(7, *a);
    BOOST_CHECK_EQUAL(4, *b);
    BOOST_CHECK_EQUAL(9, *c);
}

BOOST_AUTO_TEST_CASE(test_algorithm_find_ge)
{
    std::set<int> empty_set;
    BOOST_CHECK(find_ge(empty_set, 5) == empty_set.end());

    std::set<int> set{ 3, 4, 7, 9 };

    auto x = find_ge(set, 1); //Returns '3'
    auto a = find_ge(set, 6); //Returns '7'
    auto b = find_ge(set, 4); //Returns '4'
    auto c = find_ge(set, 8); //Returns '9'
    auto d = find_ge(set, 10); //Returns 'end'

    BOOST_CHECK_EQUAL(3, *x);
    BOOST_CHECK_EQUAL(7, *a);
    BOOST_CHECK_EQUAL(4, *b);
    BOOST_CHECK_EQUAL(9, *c);
    BOOST_CHECK(d == set.end());
}

} // namespace utxx
