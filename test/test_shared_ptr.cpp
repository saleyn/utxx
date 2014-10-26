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
#include <iostream>

BOOST_AUTO_TEST_CASE( test_shared_ptr )
{
    auto ptr1 = utxx::make_shared<int>(10);
    auto ptr2 = utxx::make_shared<int>(20);

    BOOST_CHECK_NE(ptr1 != ptr2);
    BOOST_CHECK(ptr1 <  ptr2);
    BOOST_CHECK(ptr1 <= ptr2);
    BOOST_CHECK(ptr2 >  ptr1);
    BOOST_CHECK(ptr2 >= ptr1);

    {
        auto p = ptr1;
        BOOST_CHECK_EQUAL(p, ptr1);
    }

    BOOST_CHECK(ptr1);
    ptr1.reset();
    BOOST_CHECK(!ptr1);
}

BOOST_AUTO_TEST_CASE( test_shared_ptr_perf )
{
    static const int ITERATIONS = getenv("ITERATIONS")
                                ? atoi(getenv("ITERATIONS")) : 1000000;
    double elapsed1, elapsed2;
    double latency1, latency2;
    {
        std::vector<utxx::shared_ptr<long>> vector(ITERATIONS);
        *(vector[0]) = 10;
        utxx::timer tm;
        for (int i=1; i < ITERATIONS; i++)
            vector[i] = vector[i-1];
        elapsed1 = tm.elapsed();
        latency1 = tm.latency_usec(ITERATIONS);
    }

    {
        std::vector<std::shared_ptr<long>> vector(ITERATIONS);
        *(vector[0]) = 10;
        utxx::timer tm;
        for (int i=1; i < ITERATIONS; i++)
            vector[i] = vector[i-1];
        elapsed2 = tm.elapsed();
        latency2 = tm.latency_usec(ITERATIONS);
    }

    BOOST_MESSAGE(" utxx::shared_ptr speed: "
                    << fixed(double(ITERATIONS)/elapsed1, 10, 0)
                    << " calls/s, latency: "
                    << fixed(double(ITERATIONS)/latency1, 3, 0) << "us");
    BOOST_MESSAGE("  std::shared_ptr speed: "
                    << fixed(double(ITERATIONS)/elapsed2, 10, 0)
                    << " calls/s, latency: "
                    << fixed(double(ITERATIONS)/latency2, 3, 0) << "us");
    BOOST_MESSAGE("    utxx / std: " << fixed(elapsed1/elapsed2, 6, 4) << " times");

}

