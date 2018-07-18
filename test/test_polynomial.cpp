//----------------------------------------------------------------------------
/// \file  test_polynomial.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes and functions in the polynomial.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2018-06-31
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is included in utxx open-source project

Copyright (C) 2018 Serge Aleynikov <saleyn@gmail.com>

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
#include <utxx/polynomial.hpp>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_quad_polynomial )
{
    double y[] = {1, 6, 17, 34, 57, 86, 121, 162, 209, 262, 321};
    int n = std::extent<decltype(y)>::value;
    double x[n];
    for (int i=0; i < n; ++i) x[i] = i;

    double a,b,c;
    std::tie(a,b,c) = quad_polynomial(x, y, n);

    auto calc = [=](double x) { return a + b * x + c * x * x; };

    for (int i=0; i < n; ++i)
        BOOST_REQUIRE(std::abs<double>(y[i] - calc(x[i])) < 0.000001);
}

