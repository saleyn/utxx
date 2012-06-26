//----------------------------------------------------------------------------
/// \file  test_basic_udp_receiver.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes and functions in the basic_udp_receiver.hpp
//----------------------------------------------------------------------------
// Copyright (c) 2010 Dmitriy Kargapolov <dmitriy.kargapolov@gmail.com>
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2012-06-26
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the util open-source project

Copyright (C) 2012 Dmitriy Kargapolov, Serge Aleynikov

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

//#define _ALLOCATOR_MEM_DEBUG 1

#include <boost/test/unit_test.hpp>
#include <util/io/basic_udp_receiver.hpp>
#include <util/verbosity.hpp>
#include <iostream>

using namespace util;

class client : public util::io::basic_udp_receiver<client> {
    typedef util::io::basic_udp_receiver<client> base;
public:
    client(boost::asio::io_service& a_io) : base(a_io) {}

    void on_data(buffer_type& a_buf) {}
};


BOOST_AUTO_TEST_CASE( test_basic_udp_receiver )
{
    boost::asio::io_service l_service;
    client c(l_service);
    try {
        c.init(12345);
        c.init("localhost", "12345");
        BOOST_REQUIRE(false);
    } catch (std::exception& e) {
        BOOST_REQUIRE_EQUAL("open: Already open", e.what());
        c.stop();
    }

    try {
        c.init("localhost", "12345");
        c.start();
        c.stop();
        l_service.run();
        BOOST_REQUIRE(true);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        BOOST_REQUIRE(false);
    }

    try {
        c.init(boost::asio::ip::udp::endpoint());
        c.start();
        c.stop();
        l_service.run();
        BOOST_REQUIRE(true);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        BOOST_REQUIRE(false);
    }
}

