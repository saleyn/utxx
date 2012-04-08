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

This file is part of util open-source project.

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

#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <util/persist_blob.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::unit_test;
using namespace util;

static const char* filename = "/tmp/persist_blob.bin";

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
    ::unlink(filename);
    {
        persist_blob<test_blob> o;
        test_blob orig(1, 2);

        int res = o.init(filename, &orig);
        if (res < 0)
            std::cerr << "Initialization error: " << errno << std::endl;
        BOOST_REQUIRE(res >= 0);

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
    unlink(filename);
}

class producer {
    int m_instance;
    int m_iterations;
    persist_blob<test_blob>& m_logger;
public:
    producer(persist_blob<test_blob>& logger, int n, int iter) 
        : m_instance(n), m_iterations(iter), m_logger(logger)
    {}

    producer(const producer& a_rhs) : m_instance(a_rhs.m_instance),
        m_iterations(a_rhs.m_iterations), m_logger(a_rhs.m_logger)
    {}

    void operator() () {
        for (int i = 0; i < m_iterations; i++) {
            test_blob o(i, i<<1);
            m_logger.set(o);
            if (i % 1000 == 0) {
                uint32_t v[2];
                m_logger.vsn(v[0], v[1]);
                std::cerr << "producer" << m_instance << " - " << i 
                          << "(v1=" << v[0] << ", v2=" << v[1] << ") contentions: r="
                          << m_logger.read_contentions()
                          << ", w=" << m_logger.write_contentions() << std::endl;
            }
            sched_yield();
        }
    }
};

class consumer {
    int   m_instance;
    bool* m_cancel;
    bool* m_has_error;
    persist_blob<test_blob>& m_logger;
public:
    consumer(persist_blob<test_blob>& logger, int n, bool& cancel, bool& error) 
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
                std::cerr << "Thread" << m_instance 
                          << " detected error: {" << o.i1 << ", " << o.i2 << "}" << std::endl;
                break;
            }
            sched_yield();
        }
    }
};

BOOST_AUTO_TEST_CASE( test_persist_blob_concurrent )
{
    enum { ITERATIONS = 100000 };

    ::unlink(filename);
    {
        persist_blob<test_blob> l_blob;
        
        int ok = l_blob.init(filename);

        BOOST_REQUIRE ( ok == 0 );

        bool cancel = false, error = false;
        {
            int n = getenv("PROD_THREADS") ? atoi(getenv("PROD_THREADS")) : 2;
            int m = getenv("CONS_THREADS") ? atoi(getenv("CONS_THREADS")) : 2;
            static const int s_prod = n, s_cons = m;

            boost::shared_ptr<boost::thread> l_prod_th[s_prod];
            boost::shared_ptr<boost::thread> l_cons_th[s_cons];

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

        std::cerr << "    Iterations : " << ITERATIONS << std::endl;
        std::cerr << "    Contentions: rd=" << l_blob.read_contentions() << 
                     ", wt=" << l_blob.write_contentions() << std::endl;
        BOOST_REQUIRE(!error);
    }
    ::unlink(filename);
}

