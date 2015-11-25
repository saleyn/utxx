//----------------------------------------------------------------------------
/// \file   test_signal_block.cpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Test cases for signal_block functions.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-09-31
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
#include <utxx/signal_block.hpp>
#include <iostream>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_signal_block )
{
    int i = 0;
    for (const char** p = sig_names(); *p && i < 66; ++p, ++i);

    BOOST_CHECK_EQUAL(65, i);
    BOOST_CHECK_EQUAL("SIGTERM", sig_name(15));

    auto sset = sig_members_parse("sigterm|sigint", UTXX_SRC);

    BOOST_CHECK(sigismember(&sset, SIGTERM));
    BOOST_CHECK(sigismember(&sset, SIGINT));

    BOOST_CHECK_EQUAL("SIGINT|SIGTERM", sig_members(sset));

    sset = sig_init_set(SIGKILL, SIGTERM);

    BOOST_CHECK_EQUAL("SIGKILL|SIGTERM", sig_members(sset));
}