//----------------------------------------------------------------------------
/// \file  test_url.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes and functions in the url.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-05-10
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
#include <utxx/url.hpp>
#include <utxx/verbosity.hpp>
#include <iostream>

using namespace std;
using namespace utxx;

BOOST_AUTO_TEST_CASE( test_url )
{
    addr_info l_url;
    BOOST_REQUIRE(parse_url("tcp://127.0.0.1", l_url));
    BOOST_REQUIRE_EQUAL(TCP,         l_url.proto);
    BOOST_REQUIRE_EQUAL("127.0.0.1", l_url.addr);
    BOOST_REQUIRE_EQUAL("",          l_url.port);
    BOOST_REQUIRE_EQUAL("",          l_url.path);
    BOOST_REQUIRE(l_url.is_ipv4());

    BOOST_REQUIRE(parse_url("tcp://localhost:2345", l_url));
    BOOST_REQUIRE_EQUAL(TCP,         l_url.proto);
    BOOST_REQUIRE_EQUAL("localhost", l_url.addr);
    BOOST_REQUIRE_EQUAL("2345",      l_url.port);
    BOOST_REQUIRE_EQUAL("",          l_url.path);
    BOOST_REQUIRE(!l_url.is_ipv4());

    BOOST_REQUIRE(parse_url("http://127.0.0.1", l_url));
    BOOST_REQUIRE_EQUAL(TCP,         l_url.proto);
    BOOST_REQUIRE_EQUAL("127.0.0.1", l_url.addr);
    BOOST_REQUIRE_EQUAL("80",        l_url.port);
    BOOST_REQUIRE_EQUAL("",          l_url.path);
    BOOST_REQUIRE(l_url.is_ipv4());

    BOOST_REQUIRE(parse_url("tcp://233.37.0.10@127.0.0.1;eth1:1024/temp", l_url));
    BOOST_REQUIRE_EQUAL(TCP,         l_url.proto);
    BOOST_REQUIRE_EQUAL("233.37.0.10@127.0.0.1;eth1", l_url.addr);
    BOOST_REQUIRE_EQUAL("1024",      l_url.port);
    BOOST_REQUIRE_EQUAL("/temp",     l_url.path);
    BOOST_REQUIRE(l_url.is_ipv4());

    BOOST_REQUIRE(parse_url("udp://myhome.com:1234", l_url));
    BOOST_REQUIRE_EQUAL(UDP,         l_url.proto);
    BOOST_REQUIRE_EQUAL("myhome.com",l_url.addr);
    BOOST_REQUIRE_EQUAL("1234",      l_url.port);
    BOOST_REQUIRE_EQUAL("",          l_url.path);
    BOOST_REQUIRE(!l_url.is_ipv4());

    BOOST_REQUIRE(parse_url("http://localhost", l_url));
    BOOST_REQUIRE_EQUAL(TCP,         l_url.proto);
    BOOST_REQUIRE_EQUAL("localhost", l_url.addr);
    BOOST_REQUIRE_EQUAL("80",        l_url.port);
    BOOST_REQUIRE_EQUAL("",          l_url.path);
    BOOST_REQUIRE(!l_url.is_ipv4());

    BOOST_REQUIRE(parse_url("https://localhost", l_url));
    BOOST_REQUIRE_EQUAL(TCP,         l_url.proto);
    BOOST_REQUIRE_EQUAL("https",     l_url.proto_str());
    BOOST_REQUIRE_EQUAL("localhost", l_url.addr);
    BOOST_REQUIRE_EQUAL("443",       l_url.port);
    BOOST_REQUIRE_EQUAL("",          l_url.path);
    BOOST_REQUIRE(!l_url.is_ipv4());

    BOOST_REQUIRE(parse_url("http://google.com:8000/a/b/d?a=3", l_url));
    BOOST_REQUIRE_EQUAL(TCP,         l_url.proto);
    BOOST_REQUIRE_EQUAL("google.com",l_url.addr);
    BOOST_REQUIRE_EQUAL("8000",      l_url.port);
    BOOST_REQUIRE_EQUAL("/a/b/d?a=3",l_url.path);
    BOOST_REQUIRE(!l_url.is_ipv4());

    BOOST_REQUIRE(parse_url("uds:///tmp/path", l_url));
    BOOST_REQUIRE_EQUAL(UDS,         l_url.proto);
    BOOST_REQUIRE_EQUAL("",          l_url.addr);
    BOOST_REQUIRE_EQUAL("",          l_url.port);
    BOOST_REQUIRE_EQUAL("/tmp/path", l_url.path);
    BOOST_REQUIRE(!l_url.is_ipv4());

    BOOST_REQUIRE(parse_url("file:///tmp/path", l_url));
    BOOST_REQUIRE_EQUAL(FILENAME,    l_url.proto);
    BOOST_REQUIRE_EQUAL("",          l_url.addr);
    BOOST_REQUIRE_EQUAL("",          l_url.port);
    BOOST_REQUIRE_EQUAL("/tmp/path", l_url.path);
    BOOST_REQUIRE(!l_url.is_ipv4());

    BOOST_REQUIRE(parse_url("cmd://7z -so x temp.7z", l_url));
    BOOST_REQUIRE_EQUAL(CMD,         l_url.proto);
    BOOST_REQUIRE_EQUAL("",          l_url.addr);
    BOOST_REQUIRE_EQUAL("",          l_url.port);
    BOOST_REQUIRE_EQUAL("cmd://7z -so x temp.7z", l_url.url);
    BOOST_REQUIRE_EQUAL("7z -so x temp.7z",       l_url.path);
    BOOST_REQUIRE(!l_url.is_ipv4());

    string ip = "123.45.67.89";
    BOOST_REQUIRE(make_pair(ip, 10) == split_addr("123.45.67.89:10"));
    BOOST_REQUIRE(make_pair(ip, -1) == split_addr("123.45.67.89"));
    BOOST_REQUIRE(make_pair(ip, -1) == split_addr("123.45.67.89:99999"));
    BOOST_CHECK_THROW(split_addr(ip, true), runtime_error);
    BOOST_CHECK_THROW(split_addr("123.45.67.89:99999", true), runtime_error);
}