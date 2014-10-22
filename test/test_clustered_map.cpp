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
#include <boost/timer.hpp>
#include <utxx/container/clustered_map.hpp>

#if __cplusplus >= 201103L
#include <random>
#else
#include <boost/random.hpp>
#endif

typedef utxx::clustered_map<size_t, int> cmap;

void visitor(size_t k, int v, int& sum) { sum += k; }

double sample() {
  restart:
    double u = ((double) rand() / (RAND_MAX)) * 2 - 1;
    double v = ((double) rand() / (RAND_MAX)) * 2 - 1;
    double r = u * u + v * v;
    if (r == 0 || r > 1) goto restart;
    double c = sqrt(-2 * log(r) / r);
    return u * c;
}

#ifdef UTXX_STANDALONE
int main(int argc, char* argv[]) {
#else
BOOST_AUTO_TEST_CASE( test_clustered_map ) {
#endif
    cmap m;
    static const int s_data[][2] = {
        {1, 10}, {2, 20}, {3, 30},
        {65,40}, {66,50}, {67,60},
        {129,70}
    };

    for (size_t i=0; i < sizeof(s_data)/sizeof(s_data[0]); i++)
        m.insert(s_data[i][0], s_data[i][1]);

    BOOST_REQUIRE_EQUAL(3u, m.group_count());
    BOOST_REQUIRE_EQUAL(3u, m.item_count(1));
    BOOST_REQUIRE_EQUAL(3u, m.item_count(65));
    BOOST_REQUIRE_EQUAL(1u, m.item_count(129));

    for (size_t i = 0; i < sizeof(s_data)/sizeof(s_data[0]); i++) {
        int* p = m.at(s_data[i][0]);
        BOOST_REQUIRE(p);
        BOOST_REQUIRE_EQUAL(s_data[i][1], *p);
    }

    int n = 0;
    for (cmap::iterator it = m.begin(), e = m.end(); it != e; ++it, ++n) {
        BOOST_REQUIRE_EQUAL(s_data[n][0], (int)it.key());
        BOOST_REQUIRE_EQUAL(s_data[n][1], it.data());
    }

    int j = 0;
    m.for_each(visitor, j);
    BOOST_REQUIRE_EQUAL(333, j);

    BOOST_REQUIRE(m.erase(2));
    BOOST_REQUIRE_EQUAL(2u, m.item_count(1));
    BOOST_REQUIRE(m.erase(3));
    BOOST_REQUIRE_EQUAL(3u, m.group_count());
    BOOST_REQUIRE(m.erase(129));
    BOOST_REQUIRE_EQUAL(2u, m.group_count());
    BOOST_REQUIRE_EQUAL(0u, m.item_count(129));

    m.clear();

    BOOST_REQUIRE(m.empty());

    const long ITERATIONS = getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 1000000;
    {
        int mean  = 2*4096;
        int sigma = 30;
        double elapsed1, elapsed2;

        {
            utxx::clustered_map<size_t, int, 2> m;

            #if __cplusplus >= 201103L
            std::default_random_engine generator;
            std::normal_distribution<double> distribution(mean, sigma);
            #else
            time_t seed = time(0);
            boost::mt19937 generator(static_cast<unsigned>(seed));
            boost::normal_distribution<double> normal_distribution(mean, sigma);
            boost::variate_generator<boost::mt19937&, boost::normal_distribution<double> >
                gaussian_rnd(generator, normal_distribution);
            #endif

            boost::timer t;
            for (long i = 0; i < ITERATIONS; i++) {
            recalc1:
                #if __cplusplus >= 201103L
                int number = (int)distribution(generator);
                #else
                int number = (int)gaussian_rnd();
                #endif
                if ((number < 0) || (number > 2*mean)) goto recalc1;
                int& n = m.insert((int)number);
                n++;
            }

            elapsed1 = t.elapsed();
            char buf[80];
            sprintf(buf, "clustered_map speed=%.0f ins/s, latency=%.3fus, size=%lu",
                   (double)ITERATIONS/elapsed1, elapsed1 * 1000000 / ITERATIONS,
                   m.group_count());
            BOOST_MESSAGE(buf);
        }

        {
            std::map<int, int> m;

            #if __cplusplus >= 201103L
            std::default_random_engine generator;
            std::normal_distribution<double> distribution(mean, sigma);
            #else
            time_t seed = time(0);
            boost::mt19937 generator(static_cast<unsigned>(seed));
            boost::normal_distribution<double> normal_distribution(mean, sigma);
            boost::variate_generator<boost::mt19937&, boost::normal_distribution<double> >
                gaussian_rnd(generator, normal_distribution);
            #endif

            boost::timer t;
            for (long i = 0; i < ITERATIONS; i++) {
            recalc2:
                #if __cplusplus >= 201103L
                int number = (int)distribution(generator);
                #else
                int number = (int)gaussian_rnd();
                #endif
                if ((number < 0) || (number > 2*mean)) goto recalc2;
                int& n = m[(int)number];
                n++;
            }

            elapsed2 = t.elapsed();
            char buf[80];
            sprintf(buf, "std::map      speed=%.0f ins/s, latency=%.3fus, l1size=%lu",
                   (double)ITERATIONS/elapsed2, elapsed2 * 1000000 / ITERATIONS, m.size());
            BOOST_MESSAGE(buf);
        }

        BOOST_MESSAGE("Performance(clustered_map / std::map) = " << elapsed2 / elapsed1);
    }

#ifdef UTXX_STANDALONE
    return 0;
#endif
}


