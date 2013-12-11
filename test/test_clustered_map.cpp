//----------------------------------------------------------------------------
/// \file  test_clustered_map.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for clustered_map.cpp.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-08-05
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is a part of the utxx open-source project.

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

#ifdef UTXX_STANDALONE
#   include <boost/assert.hpp>
#   define BOOST_REQUIRE(a) BOOST_ASSERT(a);
#   define BOOST_REQUIRE_EQUAL(a, b) BOOST_ASSERT(a == b);
#else
#   include <boost/test/unit_test.hpp>
#endif
#include <utxx/container/clustered_map.hpp>

typedef utxx::clustered_map<int, int> cmap;

void visitor(int k, int v, int& sum) { sum += k; }

#ifdef UTXX_STANDALONE
int main(int argc, char* argv[]) {
#else
BOOST_AUTO_TEST_CASE( test_clustered_map_lookup) {
#endif
    cmap m;
    static const int s_data[][2] = {
        {1, 10}, {2, 20}, {3, 30},
        {65,40}, {66,50}, {67,60},
        {129,70}
    };

    for (size_t i=0; i < sizeof(s_data)/sizeof(s_data[0]); i++)
        m.insert(s_data[i][0], s_data[i][1]);

    BOOST_REQUIRE_EQUAL(3, m.group_count());
    BOOST_REQUIRE_EQUAL(3, m.item_count(1));
    BOOST_REQUIRE_EQUAL(3, m.item_count(65));
    BOOST_REQUIRE_EQUAL(1, m.item_count(129));

    for (size_t i = 0; i < sizeof(s_data)/sizeof(s_data[0]); i++) {
        int* p = m.at(s_data[i][0]);
        BOOST_REQUIRE(p);
        BOOST_REQUIRE_EQUAL(s_data[i][1], *p);
    }

    int n = 0;
    for (cmap::iterator it = m.begin(), e = m.end(); it != m.end(); ++it, ++n) {
        BOOST_REQUIRE_EQUAL(s_data[n][0], it.key());
        BOOST_REQUIRE_EQUAL(s_data[n][1], it.data());
    }

    int j = 0;
    m.for_each(visitor, j);
    BOOST_REQUIRE_EQUAL(333, j);

    BOOST_REQUIRE(m.erase(2));
    BOOST_REQUIRE_EQUAL(2, m.item_count(1));
    BOOST_REQUIRE(m.erase(3));
    BOOST_REQUIRE_EQUAL(3, m.group_count());
    BOOST_REQUIRE(m.erase(129));
    BOOST_REQUIRE_EQUAL(2, m.group_count());
    BOOST_REQUIRE_EQUAL(0, m.item_count(129));

#ifdef UTXX_STANDALONE
    return 0;
#endif
}


