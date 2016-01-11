//----------------------------------------------------------------------------
/// \file  futex.hpp
//----------------------------------------------------------------------------
/// \brief Test concurrent futex notification.
/// Futex class is an enhanced C++ version of Rusty Russell's furlock
/// interface found in:
/// http://www.kernel.org/pub/linux/kernel/people/rusty/futex-2.2.tar.gz
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-09-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

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
#if __cplusplus >= 201103L

#include <boost/test/unit_test.hpp>
#include <utxx/futex.hpp>
#include <utxx/timestamp.hpp>
#include <utxx/verbosity.hpp>
#include <thread>

// Compile with:
//   make CPPFLAGS="-DDEBUG_ASYNC_LOGGER=2 -DPERF_STATS" -j3
// Run with:
//   VERBOSE=DEBUG test/test_utxx -t test_futex -l test_suite

using namespace utxx;

static int PROD_ITERATIONS  = getenv("PROD_ITERATIONS")
                            ? atoi(getenv("PROD_ITERATIONS")) : 20;
static int PROD_SLEEP_MS    = getenv("PROD_SLEEP_MS")
                            ? atoi(getenv("PROD_SLEEP_MS")) : 0;

static void print(bool prod, int id, int iter, futex& fut,
                  int res, wakeup_result fres, int offset)
{
    if (verbosity::level() == utxx::VERBOSE_NONE)
        return;

    std::stringstream s;
    s << timestamp::to_string().c_str()
      << (offset ? std::string(offset, ' ') : std::string())
      << (prod ? " producer" : " consumer") << "[" << id  << "]: iter=" << iter
      << ", res=" << (prod ? std::to_string(res) : to_string(fres))
      << ", val="   << fut.value() << '\n';
    printf("%s", s.str().c_str());
}

void producer(futex& fut, int id)
{
    std::string offset(40*id, ' ');

    for (int i=0; i < PROD_ITERATIONS; ++i) {
        int res = fut.signal();
        print(true, id, i, fut, res, wakeup_result::ERROR, 40*id);

        if (PROD_SLEEP_MS)
            usleep((i%2) == 0 ? 1 : PROD_SLEEP_MS*1000);
    }
}

void consumer(futex& fut, int id )
{
    int v = fut.value();

    for (int i=0; true; ++i) {
        struct timespec ts = { 1, 0 };
        wakeup_result res = fut.wait(&ts, &v);
        print(false, id, i, fut, 0, res, 0);

        if (res == wakeup_result::TIMEDOUT)
            break;
    }

    if (verbosity::level() > utxx::VERBOSE_NONE)
        std::cout << "Testing std::chrono wait" << std::endl;

    std::chrono::milliseconds ms(1000);
    auto res = fut.wait(ms, &v);
    print(false, id, 0, fut, 0, res, 0);
}

BOOST_AUTO_TEST_CASE( test_futex )
{
    futex fut;

    std::thread prod1([&fut]() { producer(fut, 1); });
    std::thread prod2([&fut]() { producer(fut, 2); });
    std::thread cons ([&fut]() { consumer(fut, 1); });

    prod1.join();
    prod2.join();
    cons.join();

#ifdef PERF_STATS
    std::cout << "Futex wake          count = " << fut.wake_count()          << std::endl;
    std::cout << "Futex wake_signaled count = " << fut.wake_signaled_count() << std::endl;
    std::cout << "Futex wait          count = " << fut.wait_count()          << std::endl;
    std::cout << "Futex wake_fast     count = " << fut.wake_fast_count()     << std::endl;
    std::cout << "Futex wait_fast     count = " << fut.wait_fast_count()     << std::endl;
    std::cout << "Futex wait_spin     count = " << fut.wait_spin_count()     << std::endl;
#endif

    BOOST_REQUIRE(true);
}

#endif
