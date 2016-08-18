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
    addr_info url;
    BOOST_REQUIRE(parse_url("tcp://127.0.0.1", url));
    BOOST_REQUIRE_EQUAL(TCP,         url.proto);
    BOOST_REQUIRE_EQUAL("127.0.0.1", url.addr);
    BOOST_REQUIRE_EQUAL("",          url.port);
    BOOST_REQUIRE_EQUAL("",          url.path);
    BOOST_REQUIRE(url.is_ipv4());

    BOOST_REQUIRE(parse_url("tcp://localhost:2345", url));
    BOOST_REQUIRE_EQUAL(TCP,         url.proto);
    BOOST_REQUIRE_EQUAL("localhost", url.addr);
    BOOST_REQUIRE_EQUAL("2345",      url.port);
    BOOST_REQUIRE_EQUAL("",          url.path);
    BOOST_REQUIRE(!url.is_ipv4());

    BOOST_REQUIRE(parse_url("http://127.0.0.1", url));
    BOOST_REQUIRE_EQUAL(TCP,         url.proto);
    BOOST_REQUIRE_EQUAL("127.0.0.1", url.addr);
    BOOST_REQUIRE_EQUAL("80",        url.port);
    BOOST_REQUIRE_EQUAL("",          url.path);
    BOOST_REQUIRE(url.is_ipv4());

    BOOST_REQUIRE(parse_url("tcp://233.37.0.10@127.0.0.1;eth1:1024/temp", url));
    BOOST_REQUIRE_EQUAL(TCP,         url.proto);
    BOOST_REQUIRE_EQUAL("233.37.0.10@127.0.0.1;eth1", url.addr);
    BOOST_REQUIRE_EQUAL("1024",      url.port);
    BOOST_REQUIRE_EQUAL("/temp",     url.path);
    BOOST_REQUIRE(url.is_ipv4());

    BOOST_REQUIRE(parse_url("udp://myhome.com:1234", url));
    BOOST_REQUIRE_EQUAL(UDP,         url.proto);
    BOOST_REQUIRE_EQUAL("myhome.com",url.addr);
    BOOST_REQUIRE_EQUAL("1234",      url.port);
    BOOST_REQUIRE_EQUAL("",          url.path);
    BOOST_REQUIRE(!url.is_ipv4());

    BOOST_REQUIRE(parse_url("http://localhost", url));
    BOOST_REQUIRE_EQUAL(TCP,         url.proto);
    BOOST_REQUIRE_EQUAL("localhost", url.addr);
    BOOST_REQUIRE_EQUAL("80",        url.port);
    BOOST_REQUIRE_EQUAL("",          url.path);
    BOOST_REQUIRE(!url.is_ipv4());

    BOOST_REQUIRE(parse_url("https://localhost", url));
    BOOST_REQUIRE_EQUAL(TCP,         url.proto);
    BOOST_REQUIRE_EQUAL("https",     url.proto_str());
    BOOST_REQUIRE_EQUAL("localhost", url.addr);
    BOOST_REQUIRE_EQUAL("443",       url.port);
    BOOST_REQUIRE_EQUAL("",          url.path);
    BOOST_REQUIRE(!url.is_ipv4());

    BOOST_REQUIRE(parse_url("http://google.com:8000/a/b/d?a=3", url));
    BOOST_REQUIRE_EQUAL(TCP,         url.proto);
    BOOST_REQUIRE_EQUAL("google.com",url.addr);
    BOOST_REQUIRE_EQUAL("8000",      url.port);
    BOOST_REQUIRE_EQUAL("/a/b/d?a=3",url.path);
    BOOST_REQUIRE(!url.is_ipv4());

    BOOST_REQUIRE(parse_url("uds:///tmp/path", url));
    BOOST_REQUIRE_EQUAL(UDS,         url.proto);
    BOOST_REQUIRE_EQUAL("",          url.addr);
    BOOST_REQUIRE_EQUAL("",          url.port);
    BOOST_REQUIRE_EQUAL("/tmp/path", url.path);
    BOOST_REQUIRE(!url.is_ipv4());

    BOOST_REQUIRE(parse_url("file:///tmp/path", url));
    BOOST_REQUIRE_EQUAL(FILENAME,    url.proto);
    BOOST_REQUIRE_EQUAL("",          url.addr);
    BOOST_REQUIRE_EQUAL("",          url.port);
    BOOST_REQUIRE_EQUAL("/tmp/path", url.path);
    BOOST_REQUIRE(!url.is_ipv4());

    BOOST_REQUIRE(parse_url("cmd://7z -so x temp.7z", url));
    BOOST_REQUIRE_EQUAL(CMD,         url.proto);
    BOOST_REQUIRE_EQUAL("",          url.addr);
    BOOST_REQUIRE_EQUAL("",          url.port);
    BOOST_REQUIRE_EQUAL("cmd://7z -so x temp.7z", url.url);
    BOOST_REQUIRE_EQUAL("7z -so x temp.7z",       url.path);
    BOOST_REQUIRE(!url.is_ipv4());

    string ip = "123.45.67.89";
    BOOST_REQUIRE(make_pair(ip, 10) == split_addr("123.45.67.89:10"));
    BOOST_REQUIRE(make_pair(ip, -1) == split_addr("123.45.67.89"));
    BOOST_REQUIRE(make_pair(ip, -1) == split_addr("123.45.67.89:99999"));
    BOOST_CHECK_THROW(split_addr(ip, true), runtime_error);
    BOOST_CHECK_THROW(split_addr("123.45.67.89:99999", true), runtime_error);

    BOOST_CHECK(!url.assign(TCP,       "google.com", 1234));
    BOOST_CHECK(!url.assign(UNDEFINED, "google.com", 1234));
    BOOST_CHECK( url.assign(TCP,       "127.1.2.3",  1234));
    BOOST_CHECK_EQUAL("tcp://127.1.2.3:1234", url.url);

    BOOST_CHECK( url.assign(TCP,       "127.1.2.3",  1234, "home"));
    BOOST_CHECK_EQUAL("tcp://127.1.2.3:1234/home", url.url);

    BOOST_CHECK( url.assign(TCP,       "127.1.2.3",  1234, "/home"));
    BOOST_CHECK_EQUAL("tcp://127.1.2.3:1234/home", url.url);

    BOOST_CHECK( url.assign(TCP,       "127.1.2.3",  1234, "/home", "eth0"));
    BOOST_CHECK_EQUAL("tcp://127.1.2.3;eth0:1234/home", url.url);

    auto url_copy = url;
    BOOST_CHECK( url_copy == url );
}