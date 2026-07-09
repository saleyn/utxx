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

#include <utxx/test_helper.hpp>
#include <algorithm>
#include <iomanip>
#include <utxx/running_stat.hpp>
#include <utxx/detail/mean_variance.hpp>
#include <utxx/time_val.hpp>
#include <utxx/persist_array.hpp>

using namespace utxx;

using running_sum      = basic_running_sum<double>;
using running_variance = basic_running_variance<double>;

TEST_CASE( test_running_stat_sum )
{
    running_sum stat;
    REQUIRE_EQUAL(0u,  stat.count());
    REQUIRE_EQUAL(0.0, stat.mean());
    REQUIRE_EQUAL(0.0, stat.min());
    REQUIRE_EQUAL(0.0, stat.max());
    running_sum s = stat;
    REQUIRE_EQUAL(0u,  s.count());
    REQUIRE_EQUAL(0.0, s.sum());
    REQUIRE_EQUAL(0.0, s.min());
    REQUIRE_EQUAL(0.0, s.max());
    s.add(10);
    s.add(15);
    REQUIRE_EQUAL(2u,   s.count());
    REQUIRE_EQUAL(25.0, s.sum());
    REQUIRE_EQUAL(10.0, s.min());
    REQUIRE_EQUAL(15.0, s.max());
    stat += s;
    REQUIRE_EQUAL(2u,   stat.count());
    REQUIRE_EQUAL(25.0, stat.sum());
    REQUIRE_EQUAL(10.0, stat.min());
    REQUIRE_EQUAL(15.0, stat.max());
    stat -= s;
    REQUIRE_EQUAL(0u,   stat.count());
    REQUIRE_EQUAL(0.0,  stat.sum());
    REQUIRE_EQUAL(10.0, stat.min());
    REQUIRE_EQUAL(15.0, stat.max());
}

TEST_CASE( test_running_stat_variance )
{
    running_sum      sum;
    running_variance var;

    for (int i=0; i < 10; i++) {
        sum.add(i);
        var.add(i);
        CHECK_EQUAL(sum.mean(), var.mean());
        TEST_MESSAGE("Mean: " << sum.mean() << " Variance: " << var.variance());
    }
}

TEST_CASE( test_running_stat )
{
    running_variance rs;
    int num[] = {2, 4, 6, 8, 10, 12, 14, 16, 18};
    std::for_each(num, *(&num+1), [&rs](auto v){ rs.add(v); });

    double avg  = detail::mean(num, *(&num+1));
    double var  = detail::variance(num, *(&num+1));
    double stdd = sqrtl(var);

    REQUIRE_EQUAL(2.0, rs.min());
    REQUIRE_EQUAL(18.0,rs.max());
    REQUIRE_EQUAL(9u,  rs.count());
    REQUIRE_EQUAL(10.0,rs.mean());
    REQUIRE_EQUAL(avg, rs.mean());
    REQUIRE_EQUAL(var, rs.variance());
    REQUIRE_EQUAL(stdd,rs.deviation());

    rs.clear();
    REQUIRE_EQUAL(0.0, rs.min());
    REQUIRE_EQUAL(0.0, rs.max());
    REQUIRE_EQUAL(0u,  rs.count());
    REQUIRE_EQUAL(0.0, rs.mean());
    REQUIRE_EQUAL(0.0, rs.mean());
    REQUIRE_EQUAL(0.0, rs.variance());
    REQUIRE_EQUAL(0.0, rs.deviation());
}

namespace std
{
    ostream& operator<< (ostream& out, pair<int, int> const& a)
    {
        return out << '{' << a.first << ',' << a.second << '}';
    }
}

TEST_CASE( test_running_stat_moving_average )
{
    #if __cplusplus >= 201103L
    {
        typedef basic_moving_average<int, 4, false> moving_avg;
        moving_avg a;
        a.add(1);
        a.add(2);
        auto sum = a.sum();

        moving_avg b(a);
        CHECK_EQUAL(sum, a.sum());

        moving_avg c(std::move(a));
        CHECK_EQUAL(sum,      c.sum());
        CHECK_EQUAL(0,        a.sum());
        CHECK(a.samples() == nullptr);
    }
    #endif

    {
        basic_moving_average<int, 4, true> ma;

        auto ZERO = std::make_pair
            (std::numeric_limits<int>::max(), std::numeric_limits<int>::min());

        CHECK_EQUAL(ZERO, ma.minmax());

        ma.add(0);
        REQUIRE_EQUAL(std::make_pair(0,0), ma.minmax());
        ma.clear();

        CHECK_EQUAL(4u, ma.capacity());
        CHECK_EQUAL(0u, ma.total());

        ma.add(2); REQUIRE_EQUAL(2.0, ma.mean());
        ma.add(4); REQUIRE_EQUAL(3.0, ma.mean());

        CHECK_EQUAL(2u, ma.total());
        CHECK_EQUAL(2u, ma.size());

        auto mm = ma.minmax();
        CHECK_EQUAL(std::make_pair(2, 4), mm);

        ma.add(6); REQUIRE_EQUAL(4.0, ma.mean());
        ma.add(8); REQUIRE_EQUAL(5.0, ma.mean());

        mm = ma.minmax();
        CHECK_EQUAL(std::make_pair(2, 8), mm);
        REQUIRE_EQUAL(4u, ma.size());

        ma.add(10); REQUIRE_EQUAL(7.0, ma.mean());
        ma.add(8);  REQUIRE_EQUAL(8.0, ma.mean());

        mm = ma.minmax();
        CHECK_EQUAL(std::make_pair(6,10), mm);

        ma.add(2);  REQUIRE_EQUAL(7.0, ma.mean());
        ma.add(16); REQUIRE_EQUAL(9.0, ma.mean());

        mm = ma.minmax();
        CHECK_EQUAL(std::make_pair(2,16), mm);

        ma.add(6);  REQUIRE_EQUAL(8.0, ma.mean());
        ma.add(4);  REQUIRE_EQUAL(7.0, ma.mean());
        ma.add(10); REQUIRE_EQUAL(9.0, ma.mean());

        mm = ma.minmax();
        CHECK_EQUAL(std::make_pair(4,16), mm);

        CHECK_EQUAL(4u,  ma.size());
        CHECK_EQUAL(36,  ma.sum());
        CHECK_EQUAL(11u, ma.total());
        ma.clear();

        REQUIRE_EQUAL(0u,   ma.size());
        REQUIRE_EQUAL(0.0,  ma.mean());
        REQUIRE_EQUAL(ZERO, ma.minmax());
    }

    {
        basic_moving_average<int, 4> ma;

        auto ZERO = std::make_pair
            (std::numeric_limits<int>::max(), std::numeric_limits<int>::min());

        REQUIRE_EQUAL(ZERO, ma.minmax());

        ma.add(0);
        REQUIRE_EQUAL(std::make_pair(0,0), ma.minmax());
        ma.clear();

        REQUIRE_EQUAL(4u, ma.capacity());

        ma.add(2); REQUIRE_EQUAL(2.0, ma.mean());
        ma.add(4); REQUIRE_EQUAL(3.0, ma.mean());

        REQUIRE_EQUAL(std::make_pair(2, 4), ma.minmax());

        ma.add(6); REQUIRE_EQUAL(4.0, ma.mean());
        ma.add(8); REQUIRE_EQUAL(5.0, ma.mean());

        REQUIRE_EQUAL(std::make_pair(2, 8), ma.minmax());
        REQUIRE_EQUAL(4u, ma.size());

        ma.add(10); REQUIRE_EQUAL(7.0, ma.mean());
        ma.add(8);  REQUIRE_EQUAL(8.0, ma.mean());

        REQUIRE_EQUAL(std::make_pair(6,10), ma.minmax());
        REQUIRE_EQUAL(4u, ma.size());
        REQUIRE_EQUAL(32, ma.sum());

        ma.clear();

        REQUIRE_EQUAL(0u,  ma.size());
        REQUIRE_EQUAL(0.0, ma.mean());
    }
}

TEST_CASE( test_running_stats_moving_average_perf )
{
    const long ITERATIONS = getenv("ITERATIONS")
                          ? atoi(getenv("ITERATIONS")) : 10000;
    std::vector<double> data(ITERATIONS);
    data[0] = 0.0;
    for (long k = 1; k < ITERATIONS; ++k)
        data[k] = (1.0 * rand() / (RAND_MAX)) - 0.5 + data[k - 1];

    std::vector<int> windows{ 16, 32, 64, 128, 256, 1024, 4096 };
    char buf[256];

    sprintf(buf, "== Add == (win) | std (us) | fast (us)| Ratio");
    TEST_MESSAGE(buf);

    for (auto i : windows) {
        double elapsed1, elapsed2;
        double dummy = 0;
        {
            basic_moving_average<double, 0, false> ma(i);
            timer t;
            for (auto v : data) {
                ma.add(v);
                dummy += ma.last();
            }
            elapsed1 = t.elapsed();
        }
        {
            basic_moving_average<double, 0, true> ma(i);
            timer t;
            for (auto v : data) {
                ma.add(v);
                dummy += ma.last();
            }
            elapsed2 = t.elapsed();
        }
        sprintf(buf, "  %13d | %8.3f | %8.3f | %8.3f | %s", i,
                (elapsed1 / ITERATIONS * 1000000),
                (elapsed2 / ITERATIONS * 1000000),
                (elapsed1 / elapsed2),
                dummy != 0 ? "ok" : "not ok"
               );
        TEST_MESSAGE(buf);
    }

    sprintf(buf, "== MinMax (win) | std (us) | fast (us)| Ratio");
    TEST_MESSAGE(buf);

    for (auto i : windows) {
        double elapsed1, elapsed2;
        double dummy = 0;
        {
            basic_moving_average<double, 0, false> ma(i);
            timer t;
            for (auto v : data) {
                ma.add(v);
                dummy += ma.minmax().first;
            }
            elapsed1 = t.elapsed();
        }
        {
            basic_moving_average<double, 0, true> ma(i);
            timer t;
            for (auto v : data) {
                ma.add(v);
                dummy += ma.minmax().first;
            }
            elapsed2 = t.elapsed();
        }
        sprintf(buf, "  %13d | %8.3f | %8.3f | %8.3f | %s", i,
                (elapsed1 / ITERATIONS * 1000000),
                (elapsed2 / ITERATIONS * 1000000),
                (elapsed1 / elapsed2),
                dummy != 0 ? "ok" : "not ok"
               );
        TEST_MESSAGE(buf);
    }
}

TEST_CASE( test_running_stat_moving_average_check )
{
    const long ITERATIONS = getenv("ITERATIONS")
                          ? atoi(getenv("ITERATIONS")) : 100000;
    std::vector<int> windows{ 16, 32, 64, 128, 256, 1024, 4096 };
    char buf[256];

    sprintf(buf, "== Match  (win) | Result");
    TEST_MESSAGE(buf);

    {
        std::vector<int> data(ITERATIONS);
        data[0] = 0;
        for (long k = 1; k < ITERATIONS; ++k)
            data[k] = rand() % 1000; // - (data[k - 1] / 2);

        for (auto i : windows) {
            basic_moving_average<double, 0, false> ms(i);
            basic_moving_average<double, 0, true>  mf(i);
            long j = 0;
            for (auto v : data) {
                ms.add(v); auto rs = ms.minmax();
                mf.add(v); auto rf = mf.minmax();

                if (rs != rf) {
                    TEST_MESSAGE("Window " << i << " mismatch at " <<
                                    j << " (value=" << v <<
                                    ") diff: " << (rs.first - rf.first));
                    int k = 0;
                    basic_moving_average<double, 0, false> mrs(i);
                    basic_moving_average<double, 0, true>  mrf(i);

                    for (auto d : data) {
                        if (k == j-1) {
                            (void)0;
                        }
                        mrs.add(d); auto rrs = mrs.minmax();
                        mrf.add(d); auto rrf = mrf.minmax();

                        if (k++ == j+5) break;

                        TEST_MESSAGE("[" << std::setw(8) << k << "]: "
                                      << std::setw(15) << d
                                      << std::setw(25) << rrs
                                      << std::setw(25) << rrf);
                    }
                    REQUIRE_EQUAL(rs, rf);
                }
            }
            sprintf(buf, "  %13d | ok", i);
            TEST_MESSAGE(buf);
        }
    }

    {
        std::vector<double> data(ITERATIONS);
        data[0] = 0.0;
        for (long k = 1; k < ITERATIONS; ++k)
            data[k] = (1.0 * rand() / (RAND_MAX)) - 0.5 + data[k - 1];

        for (auto i : windows) {
            basic_moving_average<double, 0, false> ms(i);
            basic_moving_average<double, 0, true>  mf(i);
            long j = 0;
            for (auto v : data) {
                ms.add(v);
                auto rs = ms.minmax();
                mf.add(v);
                auto rf = mf.minmax();

                static const double eps = 0.0000000001;
                if (j++ > 0 && (rs.first - rf.first) > eps) {
                    if (rs != rf) {
                        TEST_MESSAGE("Window " << i << " mismatch at " <<
                                      j << " (value=" << v <<
                                      ") diff: " << (rs.first - rf.first));
                        int k = 0;

                        for (auto d : data) {
                            TEST_MESSAGE("[" << std::setw(8) << k++ << "]: " << d);
                            if (k == j) break;
                        }
                    }
                    REQUIRE((rs.first - rf.first) < eps);
                }
            }
            sprintf(buf, "  %13d | ok", i);
            TEST_MESSAGE(buf);
        }
    }
}
