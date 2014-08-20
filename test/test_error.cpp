//----------------------------------------------------------------------------
/// \file  test_error.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the error.hpp file
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-07-10
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

#include <utxx/error.hpp>
#include <boost/test/unit_test.hpp>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_error )
{
    BOOST_REQUIRE_EQUAL("a",   utxx::runtime_error("a").str());
    BOOST_REQUIRE_EQUAL("a b", utxx::runtime_error("a", "b").str());
    BOOST_REQUIRE_EQUAL("abc", utxx::runtime_error("a", "b", "c").str());

    { auto e = utxx::runtime_error("a");               BOOST_REQUIRE_EQUAL("a",   e.str()); }
    { auto e = utxx::runtime_error("a") << "b";        BOOST_REQUIRE_EQUAL("a b", e.str()); }
    { auto e = utxx::runtime_error("a") << "b" << "c"; BOOST_REQUIRE_EQUAL("abc", e.str()); }

    BOOST_REQUIRE_EQUAL("a: Success",   utxx::io_error(0, "a").str());
    BOOST_REQUIRE_EQUAL("ab: Success",  utxx::io_error(0, "a", " b").str());
    BOOST_REQUIRE_EQUAL("abc: Success", utxx::io_error(0, "a", " b", "c").str());

    { auto e = utxx::io_error(0, "a");                 BOOST_REQUIRE_EQUAL("a: Success",     e.str()); }
    { auto e = utxx::io_error(0, "a") << ". b";        BOOST_REQUIRE_EQUAL("a: Success. b",  e.str()); }
    { auto e = utxx::io_error(0, "a") << ". b" << "c"; BOOST_REQUIRE_EQUAL("a: Success. bc", e.str()); }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    BOOST_REQUIRE_EQUAL("test: Success", utxx::sock_error(fd, "test").str());
}
