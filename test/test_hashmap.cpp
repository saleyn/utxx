//----------------------------------------------------------------------------
/// \brief This is a test file for validating hashmap.hpp functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-12-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

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

#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>
#include <utxx/hashmap.hpp>
#include <utxx/time_val.hpp>
#include <utxx/verbosity.hpp>

using namespace utxx;

static std::string srandom(int a_maxlen) {
    int len = (rand() % a_maxlen) + 1;
    char buf[len];
    for (int i=0; i < len; ++i)
        buf[i] = ' ' + (rand() % ('}' - ' '));
    return buf;
}

BOOST_AUTO_TEST_CASE( test_hashmap )
{
    typedef
        detail::basic_hash_map<const char*, int, detail::hash_fun<const char*> >
        hashtable;

    hashtable tab(30);

    tab["abc"]                               = 1;
    tab["abc_bcd_def_efgh"]                  = 2;
    tab["efg_xxxxxxx_yyyyyyyyyy"]            = 3;
    tab["Quick fox jumps over the lazy dog"] = 4;

    BOOST_REQUIRE_EQUAL(1, tab["abc"]);
    BOOST_REQUIRE_EQUAL(2, tab["abc_bcd_def_efgh"]);
    BOOST_REQUIRE_EQUAL(3, tab["efg_xxxxxxx_yyyyyyyyyy"]);
    BOOST_REQUIRE_EQUAL(4, tab["Quick fox jumps over the lazy dog"]);

    {
        std::string s("abc");
        int n = detail::hash_fun<std::string>()(s);
        BOOST_REQUIRE_EQUAL(-759293558, n);
    }

    const int COUNT      = 1024*1024;
    const int ITERATIONS = getenv("ITERATIONS")
                         ? atoi(getenv("ITERATIONS"))
                         : 10*1024*1024;

    std::vector<std::string> data(COUNT);

    for (int i=0; i < COUNT; ++i)
        data[i] = srandom(32);

    timer perf;

    long sum = 0;
    for (int i=0; i < ITERATIONS; ++i) {
        sum += detail::hash_fun<const char*>()(data[i % COUNT].c_str());
    }

    double elapsed = perf.elapsed();

    BOOST_MESSAGE(
        (boost::format("StrHashFun speed: %.3f us/call") %
                       (1000000.0 * elapsed / ITERATIONS)).str());
}