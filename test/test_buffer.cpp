//----------------------------------------------------------------------------
/// \file  test_buffer.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the buffer.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-30
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

#include <boost/test/unit_test.hpp>
#include <util/buffer.hpp>

using namespace util;

template <int N>
struct basic_rec_type {
    char msg[N];
    basic_rec_type() { msg[0] = 0; }
};

BOOST_AUTO_TEST_CASE( test_basic_io_buffer )
{
    basic_io_buffer<40> buf;
    BOOST_REQUIRE_EQUAL(40u,    buf.max_size());
    BOOST_REQUIRE_EQUAL(0u,     buf.size());
    BOOST_REQUIRE_EQUAL(40u,    buf.capacity());
    BOOST_REQUIRE_EQUAL(false,  buf.allocated());

    buf.reserve(50);
    BOOST_REQUIRE_EQUAL(50u,    buf.max_size());
    BOOST_REQUIRE_EQUAL(0u,     buf.size());
    BOOST_REQUIRE_EQUAL(50u,    buf.capacity());
    BOOST_REQUIRE_EQUAL(true,   buf.allocated());

    buf.deallocate();
    BOOST_REQUIRE_EQUAL(40u,    buf.max_size());
    BOOST_REQUIRE_EQUAL(0u,     buf.size());
    BOOST_REQUIRE_EQUAL(40u,    buf.capacity());
    BOOST_REQUIRE_EQUAL(false,  buf.allocated());

    buf.reserve(50);
    buf.commit(15);
    buf.read(10);
    BOOST_REQUIRE_EQUAL(50u,    buf.max_size());
    BOOST_REQUIRE_EQUAL(5u,     buf.size());
    BOOST_REQUIRE_EQUAL(35u,    buf.capacity());
    BOOST_REQUIRE_EQUAL(true,   buf.allocated());
    buf.reset();
    BOOST_REQUIRE_EQUAL(50u,    buf.max_size());
    BOOST_REQUIRE_EQUAL(0u,     buf.size());
    BOOST_REQUIRE_EQUAL(50u,    buf.capacity());
    BOOST_REQUIRE_EQUAL(true,   buf.allocated());
    buf.deallocate();

    buf.reserve(30);
    BOOST_REQUIRE_EQUAL(40u,    buf.max_size());
    BOOST_REQUIRE_EQUAL(0u,     buf.size());
    BOOST_REQUIRE_EQUAL(40u,    buf.capacity());
    BOOST_REQUIRE_EQUAL(false,  buf.allocated());

    strncpy(buf.wr_ptr(), "1234567890", 10);
    BOOST_REQUIRE_EQUAL(40u,    buf.max_size());
    BOOST_REQUIRE_EQUAL(0u,     buf.size());
    BOOST_REQUIRE_EQUAL(40u,    buf.capacity());
    buf.commit(10);
    BOOST_REQUIRE_EQUAL(40u,    buf.max_size());
    BOOST_REQUIRE_EQUAL(10u,    buf.size());
    BOOST_REQUIRE_EQUAL(30u,    buf.capacity());

    BOOST_REQUIRE_EQUAL(0,      memcmp("1234567890", buf.rd_ptr(), 10));
    buf.read(10);

    BOOST_REQUIRE_EQUAL(40u,    buf.max_size());
    BOOST_REQUIRE_EQUAL(0u,     buf.size());
    BOOST_REQUIRE_EQUAL(30u,    buf.capacity());

    char* p = buf.write("xx", 2);
    BOOST_REQUIRE_EQUAL(2u,     buf.size());
    BOOST_REQUIRE_EQUAL(28u,    buf.capacity());
    BOOST_REQUIRE_EQUAL(28,    buf.end() - p);

    buf.crunch();
    BOOST_REQUIRE_EQUAL(40u,    buf.max_size());
    BOOST_REQUIRE_EQUAL(2u,     buf.size());
    BOOST_REQUIRE_EQUAL(38u,    buf.capacity());

    BOOST_REQUIRE(buf.read(3) == NULL);
    BOOST_REQUIRE(buf.read(2) != NULL);

    BOOST_REQUIRE_EQUAL(0u,     buf.size());
    BOOST_REQUIRE_EQUAL(38u,    buf.capacity());

    buf.crunch();
    BOOST_REQUIRE_EQUAL(0u,     buf.size());
    BOOST_REQUIRE_EQUAL(40u,    buf.capacity());

    {
        // Test memory allocator
        typedef basic_rec_type<20> rec_type;
        typedef std::allocator<rec_type> alloc_t;
        basic_io_buffer<10, alloc_t> buf;
        buf.reserve(100);
        BOOST_REQUIRE_EQUAL(100u,  buf.max_size());
        BOOST_REQUIRE_EQUAL(0u,    buf.size());
        BOOST_REQUIRE_EQUAL(100u,  buf.capacity());
        buf.deallocate();
        BOOST_REQUIRE_EQUAL(10u,  buf.max_size());
        BOOST_REQUIRE_EQUAL(0u,   buf.size());
        BOOST_REQUIRE_EQUAL(10u,  buf.capacity());
    }
}

BOOST_AUTO_TEST_CASE( test_basic_io_buffer_lwm )
{
    basic_io_buffer<40> buf;

    buf.commit(30);
    BOOST_REQUIRE_EQUAL(30u,   buf.size());
    BOOST_REQUIRE_EQUAL(10u,   buf.capacity());
    buf.read(5);
    BOOST_REQUIRE_EQUAL(25u,   buf.size());
    BOOST_REQUIRE_EQUAL(10u,   buf.capacity());

    buf.reset();
    BOOST_REQUIRE_EQUAL(0u,    buf.size());
    BOOST_REQUIRE_EQUAL(40u,   buf.capacity());

    buf.wr_lwm(16);
    buf.commit(20);

    // 1.

    buf.read(5); // Write pointer is below wr_lwm
    BOOST_REQUIRE_EQUAL(15u,   buf.size());
    BOOST_REQUIRE_EQUAL(20u,   buf.capacity());

    buf.commit(5);
    BOOST_REQUIRE_EQUAL(20u,   buf.size());
    BOOST_REQUIRE_EQUAL(15u,   buf.capacity());  // below wr_lwm

    // 2.

    buf.read(5);

    BOOST_REQUIRE_EQUAL(15u,   buf.size());
    BOOST_REQUIRE_EQUAL(15u,   buf.capacity());  // auto-increased

    // 3.

    buf.read_and_crunch(5); // This invokes the crunch() function that will cause
                            // automatic increase of capacity space.
    BOOST_REQUIRE_EQUAL(10u,   buf.size());
    BOOST_REQUIRE_EQUAL(30u,   buf.capacity());  // auto-increased
}

BOOST_AUTO_TEST_CASE( test_record_buffers )
{
    typedef basic_rec_type<10> rec_type;

    record_buffers<rec_type, 5> bufs;

    const rec_type* it  = bufs.begin();
    const rec_type* end = bufs.end();

    BOOST_REQUIRE_EQUAL(5u,     bufs.max_size());
    BOOST_REQUIRE_EQUAL(0u,     bufs.size());
    BOOST_REQUIRE_EQUAL(5u,     bufs.capacity());
    BOOST_REQUIRE_EQUAL(false,  bufs.allocated());
    BOOST_REQUIRE_EQUAL(it + 5, end);
    const rec_type* next = bufs.write(1);
    BOOST_REQUIRE_EQUAL(it+1,   next);
    BOOST_REQUIRE_EQUAL(1u,     bufs.size());
    BOOST_REQUIRE_EQUAL(4u,     bufs.capacity());

    bufs.reserve(7);
    it  = bufs.begin();
    end = bufs.end();

    BOOST_REQUIRE_EQUAL(7u,     bufs.max_size());
    BOOST_REQUIRE_EQUAL(1u,     bufs.size());
    BOOST_REQUIRE_EQUAL(6u,     bufs.capacity());
    BOOST_REQUIRE_EQUAL(true,   bufs.allocated());
    next = bufs.write(6);
    BOOST_REQUIRE_EQUAL(7u,     bufs.size());
    BOOST_REQUIRE_EQUAL(0u,     bufs.capacity());
    BOOST_REQUIRE_EQUAL(end, next);
    next = bufs.read();
    BOOST_REQUIRE_EQUAL(it, next);
    for (int i=0; i < 6; i++) next = bufs.read();
    BOOST_REQUIRE_EQUAL(end-1, next);
    BOOST_REQUIRE_EQUAL(0u,     bufs.size());
    BOOST_REQUIRE_EQUAL(0u,     bufs.capacity());

    bufs.reset();
    BOOST_REQUIRE_EQUAL(7u,     bufs.max_size());
    BOOST_REQUIRE_EQUAL(0u,     bufs.size());
    BOOST_REQUIRE_EQUAL(7u,     bufs.capacity());
    BOOST_REQUIRE_EQUAL(true,   bufs.allocated());
}
