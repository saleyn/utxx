/**
 * \file
 * \brief Test cases for classes and functions in the collections.hpp
 *
 * \author Dmitriy Kargapolov <dmitriy dot kargapolov at gmail dot com>
 * \since 30 Jul 2012
 *
 * Copyright (C) 2012 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
 * Use, modification and distribution are subject to the Boost Software
 * License, Version 1.0 (See accompanying file LICENSE_1_0.txt or copy
 * at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/test/unit_test.hpp>
#include <utxx/collections.hpp>

namespace collections_test {

std::list<int> v;

class iseq: public std::list<int> {
public:
    iseq(int n) {
        for (int i = 0; i < n; i++) {
            int k = rand() % 100;
            push_back(k);
            v.push_back(k);
        }
        sort(compare);
    }
    static bool compare(const int& lhs, const int& rhs) {
        return lhs < rhs;
    }
};

BOOST_AUTO_TEST_SUITE( test_collections )

BOOST_AUTO_TEST_CASE( int_seq_merge_test )
{
    utxx::collections<iseq> m;
    for (int i=0; i<100; i++) {
        m.add(iseq(rand() % 100));
    }
    v.sort(&iseq::compare);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(m.begin(), m.end(), v.begin(), v.end());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace collections_test
