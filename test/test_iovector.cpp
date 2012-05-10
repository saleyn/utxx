//----------------------------------------------------------------------------
/// \file  test_atomic.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating atomic.hpp functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Copyright (c) 2011 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
// Created: 2011-09-30
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

Copyright (C) 2011 Serge Aleynikov <saleyn@gmail.com>
Copyright (c) 2011 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>

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
#include <util/iovector.hpp>

namespace util {
namespace io {

BOOST_AUTO_TEST_CASE(test_iovector)
{
    {
        // default constructor
        iovector v;
        BOOST_REQUIRE_EQUAL(0u, v.size());
        BOOST_REQUIRE_EQUAL(0u, v.length());
        BOOST_REQUIRE(v.empty());

        // testing fn: void push_back(const iovec & a_vec)
        v.push_back(make_iovec("a"));
        BOOST_REQUIRE_EQUAL(1u, v.size());
        BOOST_REQUIRE_EQUAL(1u, v.length());

        v.push_back(make_iovec("b"));
        BOOST_REQUIRE_EQUAL(2u, v.size());
        BOOST_REQUIRE_EQUAL(2u, v.length());

        v.push_back(make_iovec("c", 2));
        BOOST_REQUIRE_EQUAL(3u, v.size());
        BOOST_REQUIRE_EQUAL(4u, v.length());

        // testing fn: void clear()
        BOOST_REQUIRE(!v.empty());
        v.clear();
        BOOST_REQUIRE(v.empty());
    }

    iovec tv[] = { make_iovec("abc"), make_iovec(
            "de"), make_iovec("fghi") };

    {
        // testing: template<int M> iovector(const iovec(& a_data)[M])
        iovector v(tv);
        BOOST_REQUIRE_EQUAL(3u, v.size());
        BOOST_REQUIRE_EQUAL(9u, v.length());

        // testing: iovector(const iovector & a_rhs)
        iovector v1(v);
        BOOST_REQUIRE_EQUAL(3u, v1.size());
        BOOST_REQUIRE_EQUAL(9u, v1.length());

        // testing fn: void erase(size_t a_size)
        v.erase(2);
        BOOST_REQUIRE_EQUAL(0,
                strncmp("c", (char*)(&v)->iov_base, (&v)->iov_len));
        BOOST_REQUIRE_EQUAL(7u, v.length());

        // testing fn: void erase(size_t a_size)
        v.erase(2);
        BOOST_REQUIRE_EQUAL(0,
                strncmp("e", (char*)(&v)->iov_base, (&v)->iov_len));
        BOOST_REQUIRE_EQUAL(5u, v.length());

        // creating a copy of partially erased vector
        iovector v2(v);
        BOOST_REQUIRE_EQUAL(0,
                strncmp("e", (char*)(&v2)->iov_base, (&v2)->iov_len));
        BOOST_REQUIRE_EQUAL(5u, v2.length());

        // adding data
        v2.add(&tv[1], 2);
        BOOST_REQUIRE_EQUAL(0,
                strncmp("e", (char*)(&v2)->iov_base, (&v2)->iov_len));
        BOOST_REQUIRE_EQUAL(11u, v2.length());
    }

    {
        char t[32];

        // testing ctr: iovector(const iovec *a_data, size_t a_size)
        iovector v(&tv[0], 3);
        BOOST_REQUIRE_EQUAL(3u, v.size());
        BOOST_REQUIRE_EQUAL(9u, v.length());

        // testing fn: int copy_to(char *a_buf, size_t a_size) const
        BOOST_REQUIRE_EQUAL(9, v.copy_to(t, sizeof(t)));
        BOOST_REQUIRE_EQUAL(0, strncmp("abcdefghi", t, v.length()));
        BOOST_REQUIRE_EQUAL(0,
                strncmp("abc", (char*)(&v)->iov_base, (&v)->iov_len));
    }
}

} // namespace io
} // namespace util
