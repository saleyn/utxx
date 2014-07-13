//----------------------------------------------------------------------------
/// \file  test_running_stat.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the running_stat.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-05-20
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
#include <boost/bind.hpp>
#include <algorithm>
#include <utxx/running_stat.hpp>
#include <utxx/detail/mean_variance.hpp>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_running_sum )
{
    running_sum stat;
    BOOST_REQUIRE_EQUAL(0u,  stat.count());
    BOOST_REQUIRE_EQUAL(0.0, stat.mean());
    BOOST_REQUIRE_EQUAL(0.0, stat.min());
    BOOST_REQUIRE_EQUAL(0.0, stat.max());
    running_sum s = stat;
    BOOST_REQUIRE_EQUAL(0u,  s.count());
    BOOST_REQUIRE_EQUAL(0.0, s.sum());
    BOOST_REQUIRE_EQUAL(0.0, s.min());
    BOOST_REQUIRE_EQUAL(0.0, s.max());
    s.add(10);
    s.add(15);
    BOOST_REQUIRE_EQUAL(2u,   s.count());
    BOOST_REQUIRE_EQUAL(25.0, s.sum());
    BOOST_REQUIRE_EQUAL(10.0, s.min());
    BOOST_REQUIRE_EQUAL(15.0, s.max());
    stat += s;
    BOOST_REQUIRE_EQUAL(2u,   stat.count());
    BOOST_REQUIRE_EQUAL(25.0, stat.sum());
    BOOST_REQUIRE_EQUAL(10.0, stat.min());
    BOOST_REQUIRE_EQUAL(15.0, stat.max());
    stat -= s;
    BOOST_REQUIRE_EQUAL(0u,   stat.count());
    BOOST_REQUIRE_EQUAL(0.0,  stat.sum());
    BOOST_REQUIRE_EQUAL(10.0, stat.min());
    BOOST_REQUIRE_EQUAL(15.0, stat.max());
}

BOOST_AUTO_TEST_CASE( test_running_stat )
{
    running_variance rs;
    int num[] = {2, 4, 6, 8, 10, 12, 14, 16, 18};
    std::for_each(num, *(&num+1), boost::bind(&running_variance::add, &rs, _1));

    double avg  = detail::mean(num, *(&num+1));
    double var  = detail::variance(num, *(&num+1));
    double stdd = sqrtl(var);

    BOOST_REQUIRE_EQUAL(2.0, rs.min());
    BOOST_REQUIRE_EQUAL(18.0,rs.max());
    BOOST_REQUIRE_EQUAL(9u,  rs.count());
    BOOST_REQUIRE_EQUAL(10.0,rs.mean());
    BOOST_REQUIRE_EQUAL(avg, rs.mean());
    BOOST_REQUIRE_EQUAL(var, rs.variance());
    BOOST_REQUIRE_EQUAL(stdd,rs.deviation());

    rs.clear();
    BOOST_REQUIRE_EQUAL(0.0, rs.min());
    BOOST_REQUIRE_EQUAL(0.0, rs.max());
    BOOST_REQUIRE_EQUAL(0u,  rs.count());
    BOOST_REQUIRE_EQUAL(0.0, rs.mean());
    BOOST_REQUIRE_EQUAL(0.0, rs.mean());
    BOOST_REQUIRE_EQUAL(0.0, rs.variance());
    BOOST_REQUIRE_EQUAL(0.0, rs.deviation());
}

namespace std
{
    ostream& operator<< (ostream& out, pair<int, int> const& a)
    {
        return out << '{' << a.first << ',' << a.second << '}';
    }
}

BOOST_AUTO_TEST_CASE( test_running_stats_moving_average )
{
    basic_moving_average<int, 4> ma;

    BOOST_REQUIRE_EQUAL(std::make_pair(0, 0), ma.minmax());

    BOOST_REQUIRE_EQUAL(4u, ma.capacity());

    ma.add(2); BOOST_REQUIRE_EQUAL(2.0, ma.mean());
    ma.add(4); BOOST_REQUIRE_EQUAL(3.0, ma.mean());

    BOOST_REQUIRE_EQUAL(std::make_pair(2, 4), ma.minmax());

    ma.add(6); BOOST_REQUIRE_EQUAL(4.0, ma.mean());
    ma.add(8); BOOST_REQUIRE_EQUAL(5.0, ma.mean());

    BOOST_REQUIRE_EQUAL(std::make_pair(2, 8), ma.minmax());
    BOOST_REQUIRE_EQUAL(4u, ma.samples());

    ma.add(10); BOOST_REQUIRE_EQUAL(7.0, ma.mean());
    ma.add(8);  BOOST_REQUIRE_EQUAL(8.0, ma.mean());
    
    BOOST_REQUIRE_EQUAL(std::make_pair(6,10), ma.minmax());
    BOOST_REQUIRE_EQUAL(4u, ma.samples());

    ma.clear();

    BOOST_REQUIRE_EQUAL(0u,  ma.samples());
    BOOST_REQUIRE_EQUAL(0.0, ma.mean());
}

