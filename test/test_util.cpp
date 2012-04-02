//----------------------------------------------------------------------------
/// \file  test_util.cpp
//----------------------------------------------------------------------------
/// \brief This is a helper file used to define the main function.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-30
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

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

#define BOOST_TEST_MODULE test_util

#include <boost/test/unit_test.hpp>

#include <util/detail/handler_traits.hpp>

struct Msg1;
struct Msg2;
struct Msg3;

struct Processor1 {
    void on_data(const Msg1&);
    void on_packet(const Msg2&);
    void on_message(const Msg3&);
};

struct Processor2 {
};

struct Processor3 {
    void on_data(const Msg1&);
    void on_packet(const Msg1&);
    void on_message(const Msg1&);
};

BOOST_AUTO_TEST_CASE( test_handler_traits )
{
    using namespace util;

    BOOST_REQUIRE_EQUAL(false, (bool)(has_method<Processor1, Msg1>::on_packet));
    BOOST_REQUIRE_EQUAL(true,  (bool)(has_method<Processor1, Msg1>::on_data));
    BOOST_REQUIRE_EQUAL(false, (bool)(has_method<Processor1, Msg1>::on_message));

    BOOST_REQUIRE_EQUAL(true,  (bool)(has_method<Processor1, Msg2>::on_packet));
    BOOST_REQUIRE_EQUAL(false, (bool)(has_method<Processor1, Msg2>::on_data));
    BOOST_REQUIRE_EQUAL(false, (bool)(has_method<Processor1, Msg2>::on_message));

    BOOST_REQUIRE_EQUAL(false, (bool)(has_method<Processor1, Msg3>::on_packet));
    BOOST_REQUIRE_EQUAL(false, (bool)(has_method<Processor1, Msg3>::on_data));
    BOOST_REQUIRE_EQUAL(true,  (bool)(has_method<Processor1, Msg3>::on_message));

    BOOST_REQUIRE_EQUAL(false, (bool)(has_method<Processor2, Msg1>::on_packet));
    BOOST_REQUIRE_EQUAL(false, (bool)(has_method<Processor2, Msg1>::on_data));
    BOOST_REQUIRE_EQUAL(false, (bool)(has_method<Processor2, Msg1>::on_message));

    BOOST_REQUIRE_EQUAL(true,  (bool)(has_method<Processor3, Msg1>::on_packet));
    BOOST_REQUIRE_EQUAL(true,  (bool)(has_method<Processor3, Msg1>::on_data));
    BOOST_REQUIRE_EQUAL(true,  (bool)(has_method<Processor3, Msg1>::on_message));
}

