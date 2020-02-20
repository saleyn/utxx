//----------------------------------------------------------------------------
/// \file  test_buffer.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating buffer.hpp functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2012 Serge Aleynikov <saleyn@gmail.com>
// Created: 2012-05-06
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2012 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/buffered_queue.hpp>

namespace utxx {

BOOST_AUTO_TEST_CASE( test_io_buffer )
{
    basic_io_buffer<10> v;

    v.write("abcd", 4);

    dynamic_io_buffer& b = v.to_dynamic();

    BOOST_CHECK_EQUAL(4u,  b.size());
    BOOST_CHECK_EQUAL(6u,  b.capacity());
    BOOST_CHECK_EQUAL(10u, b.max_size());
    BOOST_REQUIRE(!b.allocated());

    dynamic_io_buffer c(b);
    BOOST_CHECK_EQUAL(4u,  c.size());
    BOOST_CHECK_EQUAL(6u,  c.capacity());
    BOOST_CHECK_EQUAL(10u, c.max_size());
    BOOST_REQUIRE(c.allocated());

    b.read(2);
    b.reserve(16);
    BOOST_REQUIRE(b.allocated());
    BOOST_CHECK_EQUAL(18, b.max_size());
    BOOST_CHECK_EQUAL(16, b.capacity());
    BOOST_CHECK_EQUAL(2,  b.size());
    BOOST_CHECK_EQUAL("cd", std::string(b.rd_ptr(), b.size()));
}

#if (BOOST_VERSION < 107000)

BOOST_AUTO_TEST_CASE( test_bufferred_queue )
{
    buffered_queue<> v;
    buffered_queue<true> l_buf_owner;
    buffered_queue<false> l_buf_proxy;
    buffered_queue<true, std::allocator<double> > l_buf_alloc;
    uint32_t l_buf = 0;
    boost::asio::const_buffer hb(&l_buf, sizeof(l_buf));
    l_buf_alloc.enqueue(hb);
    l_buf_proxy.enqueue(hb);
    l_buf_owner.enqueue(hb);
}

#endif // BOOST_VERSION

} // namespace utxx
