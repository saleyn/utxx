//----------------------------------------------------------------------------
/// \file  test_shared_ptr.hpp
//----------------------------------------------------------------------------
/// \brief Test implementation of shared_ptr.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-10-25
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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
#include <utxx/shared_ptr.hpp>
#include <utxx/time_val.hpp>
#include <utxx/string.hpp>
#include <random>
#include <iostream>

BOOST_AUTO_TEST_CASE( test_shared_ptr )
{
    auto ptr1 = utxx::make_shared<int>(10);
    auto ptr2 = utxx::make_shared<int>(20);

    BOOST_CHECK(ptr1 != ptr2);
    BOOST_CHECK(ptr1 <  ptr2);
    BOOST_CHECK(ptr1 <= ptr2);
    BOOST_CHECK(ptr2 >  ptr1);
    BOOST_CHECK(ptr2 >= ptr1);

    {
        auto p = ptr1;
        BOOST_CHECK(p == ptr1);
    }

    BOOST_CHECK(bool(ptr1));
    ptr1.reset();
    BOOST_CHECK(!ptr1);
}

BOOST_AUTO_TEST_CASE( test_shared_ptr_perf )
{
    static const int ITERATIONS = getenv("ITERATIONS")
                                ? atoi(getenv("ITERATIONS")) : 1000000;

    time_t seed = time(NULL);

    double elapsed1, elapsed2;
    double latency1, latency2;
    long   count1,   count2;
    long   sum1=0,   sum2=0;

    {
        std::vector<utxx::shared_ptr<long>> vector(ITERATIONS);
        vector[0] = utxx::make_shared<long>(10);
        utxx::timer tm;
        for (int i=1; i < ITERATIONS; i++)
            vector[i] = vector[i-1];

        std::mt19937 mt(seed);
        std::uniform_int_distribution<int> dist(0, ITERATIONS-1);

        for (int i=0; i < ITERATIONS; i++) {
            auto p = vector[dist(mt)];
            sum1 += *p;
        }

        elapsed1 = tm.elapsed();
        latency1 = tm.latency_usec(ITERATIONS);
        count1   = vector[0].use_count();

        BOOST_CHECK_EQUAL(count1, ITERATIONS);
    }

    {
        std::vector<std::shared_ptr<long>> vector(ITERATIONS);
        vector[0] = std::make_shared<long>(10);
        utxx::timer tm;
        for (int i=1; i < ITERATIONS; i++)
            vector[i] = vector[i-1];

        std::mt19937 mt(seed);
        std::uniform_int_distribution<int> dist(0, ITERATIONS-1);

        for (int i=0; i < ITERATIONS; i++) {
            auto p = vector[dist(mt)];
            sum2 += *p;
        }

        elapsed2 = tm.elapsed();
        latency2 = tm.latency_usec(ITERATIONS);
        count2   = vector[0].use_count();

        BOOST_CHECK_EQUAL(count2, ITERATIONS);
    }

    BOOST_TEST_MESSAGE(" utxx::shared_ptr speed: "
                    << utxx::fixed(double(ITERATIONS)/elapsed1, 10, 0)
                    << " calls/s, latency: "
                    << utxx::fixed(latency1, 5, 3) << "us, use_count=" << count1
                    << ", sum=" << sum1);
    BOOST_TEST_MESSAGE("  std::shared_ptr speed: "
                    << utxx::fixed(double(ITERATIONS)/elapsed2, 10, 0)
                    << " calls/s, latency: "
                    << utxx::fixed(latency2, 5, 3) << "us, use_count=" << count2
                    << ", sum=" << sum2);
    BOOST_TEST_MESSAGE("    utxx / std: " << utxx::fixed(elapsed1/elapsed2, 6, 4) << " times");
}
