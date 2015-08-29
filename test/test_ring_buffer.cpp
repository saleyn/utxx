//----------------------------------------------------------------------------
/// \file  test_ring_buffer.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating ring_buffer.hpp functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2012 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-08-20
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

#include <utxx/ring_buffer.hpp>
#include <memory>

namespace utxx {

template <typename Buffer>
void test(const char* a_desc, size_t a_exp_capacity, size_t a_capacity,
          void* a_memory, size_t a_mem_sz, bool a_construct = true) {
    BOOST_TEST_MESSAGE("Testing " << a_desc << " ring buffer");

    std::unique_ptr<Buffer, void(*)(Buffer*&)>
        buf(
            Buffer::create(a_capacity, a_memory, a_mem_sz, a_construct),
            &Buffer::destroy
        );

    BOOST_CHECK_EQUAL(a_exp_capacity, buf->capacity());
    BOOST_CHECK_EQUAL(0u, buf->size());
    BOOST_CHECK_EQUAL(0u, buf->total_count());
    BOOST_CHECK(buf->empty());
    BOOST_CHECK(!buf->full());

    buf->add(1);
    buf->add(2);
    buf->add(3);

    BOOST_CHECK_EQUAL(3, *buf->back());
    BOOST_CHECK_EQUAL(2u, buf->last());
    BOOST_CHECK_EQUAL(3u, buf->size());
    BOOST_CHECK_EQUAL(3u, buf->total_count());
    BOOST_CHECK(!buf->empty());
    BOOST_CHECK(!buf->full());

    buf->add(4);
    BOOST_CHECK_EQUAL(4, *buf->back());
    BOOST_CHECK_EQUAL(3u, buf->last());
    BOOST_CHECK_EQUAL(4u, buf->size());
    BOOST_CHECK_EQUAL(4u, buf->total_count());
    BOOST_CHECK(buf->full());

    buf->add(5);
    BOOST_CHECK_EQUAL(5, *buf->back());
    BOOST_CHECK_EQUAL(0u, buf->last());
    BOOST_CHECK_EQUAL(4u, buf->size());
    BOOST_CHECK_EQUAL(5u, buf->total_count());
    BOOST_CHECK(buf->full());

    buf->clear();

    BOOST_CHECK_EQUAL(4u, buf->capacity());
    BOOST_CHECK_EQUAL(0u, buf->size());
    BOOST_CHECK_EQUAL(0u, buf->total_count());
    BOOST_CHECK(buf->empty());
    BOOST_CHECK(!buf->full());
}

BOOST_AUTO_TEST_CASE( test_ring_buffer )
{
    test<ring_buffer<int, 0, false>>("non-atomic", 4, 3, NULL, 0);
    test<ring_buffer<int, 0, true> >("atomic",     4, 3, NULL, 0);

    size_t n = ring_buffer<int, 0, true>::memory_size(3);
    char*  p = new char[n];

    test<ring_buffer<int, 0, true> >("atomic-externally-allocated", 4, 3, p, n);

    delete [] p;
}

} // namespace utxx
