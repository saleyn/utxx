//----------------------------------------------------------------------------
/// \file   test_scope_exit.cpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Test cases for the scope_exit class
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-07-24
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
#include <utxx/scope_exit.hpp>
#include <iostream>

using namespace utxx;

#if __cplusplus >= 201103L

BOOST_AUTO_TEST_CASE( test_scope_exit )
{
    int i = 0;
    {
        scope_exit g([&i] () { ++i; });
        BOOST_REQUIRE_EQUAL(0, i);
    }
    BOOST_REQUIRE_EQUAL(1, i);

    {
        std::function<void()> f = [&i] () { i = 2; };
        scope_exit g(std::move(f));
        BOOST_REQUIRE_EQUAL(1, i);
    }
    BOOST_REQUIRE_EQUAL(2, i);

    {
        std::function<void()> f = [&i] () { ++i; };
        scope_exit g(f);
        BOOST_REQUIRE_EQUAL(2, i);
    }
    BOOST_REQUIRE_EQUAL(3, i);

    {
        scope_exit g([&i] () { ++i; });
        BOOST_REQUIRE_EQUAL(3, i);
        g.disable(true);
    }
    BOOST_REQUIRE_EQUAL(3, i);

    {
        auto fun = [&i] () { ++i; };
        on_scope_exit<decltype(fun)> g(fun);
        BOOST_REQUIRE_EQUAL(3, i);
    }
    BOOST_REQUIRE_EQUAL(4, i);

    {
        UTXX_SCOPE_EXIT([&i] () { ++i; });
        BOOST_REQUIRE_EQUAL(4, i);
    }
    BOOST_REQUIRE_EQUAL(5, i);
}

#endif
