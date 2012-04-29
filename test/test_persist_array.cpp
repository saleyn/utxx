//----------------------------------------------------------------------------
/// \file  test_persist_array.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating persist_array.hpp functionality
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
#include <util/persist_array.hpp>
#include <util/string.hpp>
#include <util/verbosity.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::unit_test;
using namespace util;

static const char* s_filename = "/tmp/persist_array.bin";

enum { ITERATIONS = 100000 };

struct test_blob {
    long i1;
    long i2;
    long data[10];

    test_blob() : i1(0), i2(0) {}
    test_blob(long i, long j) : i1(i), i2(j) {}
    bool operator== (const test_blob& rhs) {
        return memcmp(this, &rhs, sizeof(test_blob));
    }
    std::ostream& operator<< (std::ostream& out) {
        return out << "i1=" << i1 << ", i2=" << i2;
    }
};

typedef persist_array<test_blob, 4> persist_type;

//-----------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( test_persist_array_get_set )
{
    ::unlink(s_filename);
    {
        persist_type a;
        test_blob orig(1, 2);

        bool l_created;
        BOOST_REQUIRE_NO_THROW(l_created = a.init(s_filename, 1, false));
        BOOST_REQUIRE(l_created);
        BOOST_REQUIRE_EQUAL(0, a.count());
        BOOST_REQUIRE_EQUAL(1, a.capacity());
        
        size_t n;
        BOOST_REQUIRE_NO_THROW(n = a.allocate_rec());
        BOOST_REQUIRE_EQUAL(0, n);
        BOOST_REQUIRE_THROW(a.allocate_rec(), std::runtime_error);

        test_blob* b = a.get(n);
        BOOST_REQUIRE(b);

        {
            persist_type::scoped_lock g(a.get_lock(n));
            b->i1 = 10;
            b->i2 = 20;
        }
        BOOST_REQUIRE_EQUAL(10, a[n].i1);
        BOOST_REQUIRE_EQUAL(20, a[n].i2);
    }

    {
        persist_type a;
        test_blob orig(1, 2);

        bool l_created;
        BOOST_REQUIRE_NO_THROW(l_created = a.init(s_filename, 1, false));
        BOOST_REQUIRE(!l_created);
        BOOST_REQUIRE_EQUAL(1, a.count());
        BOOST_REQUIRE_EQUAL(1, a.capacity());
        
        test_blob* b = a.get(0);
        BOOST_REQUIRE(b);

        BOOST_REQUIRE_EQUAL(10, a[0].i1);
        BOOST_REQUIRE_EQUAL(20, a[0].i2);
    }

    unlink(s_filename);
}

namespace {
    class producer {
        int  m_instance;
        size_t m_iterations;
        persist_type& m_storage;
    public:
        producer(persist_type& a, int n, long iter) 
            : m_instance(n), m_iterations(iter), m_storage(a)
        {}

        producer(const producer& a_rhs) : m_instance(a_rhs.m_instance),
            m_iterations(a_rhs.m_iterations), m_storage(a_rhs.m_storage)
        {}

        void operator() () {
            for (size_t i = 0; m_storage.count() < m_iterations; i++) {
                test_blob o(m_instance, i+1);
                try {
                    m_storage.add(o);
                } catch (std::runtime_error& e) {
                    BOOST_REQUIRE_EQUAL("Out of storage capacity!", e.what());
                    break;
                }
            }
            if (verbosity::level() > VERBOSE_NONE)
                std::cout << to_string("Producer", m_instance, " finished!\n");
        }
    };

}

BOOST_AUTO_TEST_CASE( test_persist_array_concurrent )
{
    const long ITERATIONS = getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 10000;

    ::unlink(s_filename);
    {
        persist_type l_storage;
        bool l_created;
        
        BOOST_REQUIRE_NO_THROW(l_created = l_storage.init(s_filename, ITERATIONS, false));
        BOOST_REQUIRE(l_created);

        int n = getenv("PROD_THREADS") ? atoi(getenv("PROD_THREADS")) : 1;

        {
            static const int s_prod = n;

            boost::shared_ptr<boost::thread> l_prod_th[s_prod];

            for (int i = 0; i < s_prod; i++)
                l_prod_th[i].reset(new boost::thread(producer(l_storage, i+1, ITERATIONS)));

            for (int i = 0; i < s_prod; i++)
                l_prod_th[i]->join();

        }

        std::vector<long> l_stats(n);

        for (size_t i=0; i < l_storage.count(); i++) {
            test_blob& b = l_storage[i];
            BOOST_REQUIRE_EQUAL(l_stats[b.i1-1], b.i2-1);
            l_stats[b.i1-1] = b.i2;
        }

        BOOST_REQUIRE_EQUAL(ITERATIONS, l_storage.count());
    }
    ::unlink(s_filename);
}
