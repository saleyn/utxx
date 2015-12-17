//----------------------------------------------------------------------------
/// \file  test_persist_blob.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating persist_blob.hpp functionality
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-12-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of utxx open-source project.

Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/config.h>

#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#ifdef UTXX_HAVE_BOOST_TIMER_TIMER_HPP
#include <boost/timer/timer.hpp>
#endif
#include <utxx/persist_blob.hpp>
#include <utxx/string.hpp>
#include <utxx/verbosity.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::unit_test;
using namespace utxx;

static const char* s_filename = "/tmp/persist_blob.bin";

enum { ITERATIONS = 100000 };

struct test_blob {
    long i1;
    long i2;

    test_blob() : i1(0), i2(0) {}
    test_blob(long i, long j) : i1(i), i2(j) {}
    bool operator== (const test_blob& rhs) { return rhs.i1 == i1 && rhs.i2 == i2; }
};

//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( test_persist_blob_get_set )
{
    ::unlink(s_filename);
    {
        persist_blob<test_blob> o;
        test_blob orig(1, 2);

        BOOST_REQUIRE_NO_THROW(o.init(s_filename, &orig, false));

        {
            test_blob& val = o.dirty_get();
            BOOST_REQUIRE_EQUAL( val.i1, 1 );
            BOOST_REQUIRE_EQUAL( val.i2, 2 );
        }
        {
            test_blob val = o.get();
            BOOST_REQUIRE_EQUAL( val.i1, 1 );
            BOOST_REQUIRE_EQUAL( val.i2, 2 );
        }

        orig.i1 = 3;
        orig.i2 = 4;
        o.dirty_set(orig);

        {
            test_blob& val = o.dirty_get();
            BOOST_REQUIRE_EQUAL( val.i1, 3 );
            BOOST_REQUIRE_EQUAL( val.i2, 4 );
        }

        orig.i1 = 5;
        orig.i2 = 6;
        o.set(orig);

        {
            test_blob& val = o.dirty_get();
            BOOST_REQUIRE_EQUAL( val.i1, 5 );
            BOOST_REQUIRE_EQUAL( val.i2, 6 );
        }
    }
    unlink(s_filename);
}

namespace {
    class producer {
        int  m_instance;
        long m_iterations;
        persist_blob<test_blob>& m_logger;
    public:
        producer(persist_blob<test_blob>& logger, int n, long iter)
            : m_instance(n), m_iterations(iter), m_logger(logger)
        {}

        producer(const producer& a_rhs) : m_instance(a_rhs.m_instance),
            m_iterations(a_rhs.m_iterations), m_logger(a_rhs.m_logger)
        {}

        void operator() () {
            for (long i = 0; i < m_iterations; i++) {
                test_blob o(i, i<<1);
                m_logger.set(o);
                if (i % 5000 == 0) {
                    std::cerr <<
                        (to_string("producer", m_instance, " - ", i) +
                         to_string(" (o1=", o.i1, ", o2=", o.i2, ")\n"));
                }
                sched_yield();
            }
            if (verbosity::level() > VERBOSE_NONE)
                std::cout << to_string("Producer", m_instance, " finished!\n");
        }
    };

    class consumer {
        int   m_instance;
        bool* m_cancel;
        int* m_has_error;
        persist_blob<test_blob>& m_logger;
    public:
        consumer(persist_blob<test_blob>& logger, int n, bool& cancel, int& error)
            : m_instance(n), m_cancel(&cancel), m_has_error(&error), m_logger(logger)
        {}

        consumer(const consumer& a_rhs) : m_instance(a_rhs.m_instance),
            m_cancel(a_rhs.m_cancel), m_has_error(a_rhs.m_has_error),
            m_logger(a_rhs.m_logger)
        {}

        void operator() () {
            while (!*m_cancel) {
                test_blob o = m_logger.get();

                if (o.i2 != o.i1 << 1) {
                    *m_has_error = true;
                    std::cerr <<
                        to_string("Consumer", m_instance,
                            " detected error: {", o.i1, ", ", o.i2, "}\n");
                    m_has_error++;
                }
                sched_yield();
            }
            if (verbosity::level() > VERBOSE_NONE)
                std::cout << to_string("Consumer", m_instance, " finished!\n");
        }
    };
}

#ifdef UTXX_HAVE_BOOST_TIMER_TIMER_HPP

BOOST_AUTO_TEST_CASE( test_persist_blob_concurrent )
{
    using boost::timer::cpu_timer;
    using boost::timer::cpu_times;
    using boost::timer::nanosecond_type;
    const long ITERATIONS = getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 10000;

    ::unlink(s_filename);
    {
        persist_blob<test_blob> l_blob;

        BOOST_REQUIRE_NO_THROW(l_blob.init(s_filename, NULL, false));

        bool cancel = false;
        int  error  = 0;

        int n = getenv("PROD_THREADS") ? atoi(getenv("PROD_THREADS")) : 1;
        int m = getenv("CONS_THREADS") ? atoi(getenv("CONS_THREADS")) : 1;

        cpu_timer t;
        {
            static const int s_prod = n, s_cons = m;

            std::vector<std::shared_ptr<boost::thread>> l_prod_th(s_prod);
            std::vector<std::shared_ptr<boost::thread>> l_cons_th(s_cons);

            for (int i = 0; i < s_prod; i++)
                l_prod_th[i].reset(new boost::thread(producer(l_blob, i+1, ITERATIONS)));
            for (int i = 0; i < s_cons; i++)
                l_cons_th[i].reset(new boost::thread(consumer(l_blob, i+1+s_prod, cancel, error)));

            for (int i = 0; i < s_prod; i++)
                l_prod_th[i]->join();

            cancel = true;

            for (int i = 0; i < s_prod; i++)
                l_cons_th[i]->join();
        }
        cpu_times elapsed_times(t.elapsed());
        nanosecond_type t1 = elapsed_times.system + elapsed_times.user;

        if (utxx::verbosity::level() > utxx::VERBOSE_NONE) {
            printf("Persist storage iterations: %ld\n", ITERATIONS);
            printf("Persist storage time      : %.3fs (%.3fus/call)\n",
                (double)t1 / 1000000000.0, (double)t1 / ITERATIONS / 1000.0);
            printf("Errors: %d\n", error);
        }
        BOOST_REQUIRE(!error);
    }
    ::unlink(s_filename);
}

#endif
