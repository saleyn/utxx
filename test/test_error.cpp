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
#include <regex>

using namespace utxx;

const src_info& sample_src() { static const auto s_src = UTXX_SRC; return s_src; }

namespace abc { namespace d {
    template <class T>
    struct A {
        template <class U, class V>
        struct B {
            static const src_info& my_fun() { static const auto s_src = UTXX_SRC; return s_src; }
        };
    };
}}

BOOST_AUTO_TEST_CASE( test_error )
{
    BOOST_REQUIRE_EQUAL("a",   utxx::runtime_error("a").str());
    BOOST_REQUIRE_EQUAL("ab",  utxx::runtime_error("a", "b").str());
    BOOST_REQUIRE_EQUAL("abc", utxx::runtime_error("a", "b", "c").str());

    { auto e = utxx::runtime_error("a");               BOOST_REQUIRE_EQUAL("a",   e.str()); }
    { auto e = utxx::runtime_error("a") << "b";        BOOST_REQUIRE_EQUAL("ab",  e.str()); }
    { auto e = utxx::runtime_error("a") << "b" << "c"; BOOST_REQUIRE_EQUAL("abc", e.str()); }

    BOOST_REQUIRE_EQUAL("a: Success",   utxx::io_error(0, "a").str());
    BOOST_REQUIRE_EQUAL("ab: Success",  utxx::io_error(0, "a", "b").str());
    BOOST_REQUIRE_EQUAL("abc: Success", utxx::io_error(0, "a", "b", "c").str());

    { auto e = utxx::io_error(0, "a");                 BOOST_REQUIRE_EQUAL("a: Success",     e.str()); }
    { auto e = utxx::io_error(0, "a") << ". b";        BOOST_REQUIRE_EQUAL("a: Success. b",  e.str()); }
    { auto e = utxx::io_error(0, "a") << ". b" << "c"; BOOST_REQUIRE_EQUAL("a: Success. bc", e.str()); }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    BOOST_REQUIRE_EQUAL("test: Success", utxx::sock_error(fd, "test").str());

    try {
        UTXX_THROW(utxx::runtime_error, "A ", 123);
    } catch (utxx::runtime_error& e) {
        BOOST_CHECK_EQUAL("A 123", e.str());
        std::regex re("\\[test_error.cpp:\\d+ test_error::test_method\\] A 123");
        if (!std::regex_search(std::string(e.what()), re)) {
            std::cout << "Error what: " << e.what() << std::endl;
            std::cout << "Error src:  " << e.src()  << std::endl;
            BOOST_CHECK(false);
        }
        BOOST_CHECK(!e.src().empty());
    }

    try {
        UTXX_THROW_RUNTIME_ERROR("A ", 123);
    } catch (utxx::runtime_error& e) {
        BOOST_CHECK_EQUAL("A 123", e.str());
        std::regex re("\\[test_error.cpp:\\d+ test_error::test_method\\] A 123");
        BOOST_REQUIRE(std::regex_search(std::string(e.what()), re));
        BOOST_REQUIRE(!e.src().empty());
    }

    try {
        UTXX_THROW_BADARG_ERROR("A ", 123);
    } catch (utxx::badarg_error& e) {
        BOOST_CHECK_EQUAL("A 123", e.str());
        std::regex re("\\[test_error.cpp:\\d+ test_error::test_method\\] A 123");
        BOOST_REQUIRE(std::regex_search(std::string(e.what()), re));
        BOOST_REQUIRE(!e.src().empty());
    }
}

BOOST_AUTO_TEST_CASE( test_error_srcloc )
{
    utxx::src_info s("A", "B");
    auto s1(s);
    BOOST_CHECK_EQUAL("A", s1.srcloc());
    BOOST_CHECK_EQUAL("B", s1.fun());

    try {
        UTXX_SRC_THROW(utxx::runtime_error, sample_src(), "B ", 111);
    } catch (utxx::runtime_error& e) {
        BOOST_CHECK_EQUAL("B 111", e.str());
        std::regex re("\\[test_error.cpp:\\d+ sample_src\\] B 111");
        BOOST_REQUIRE(std::regex_search(std::string(e.what()), re));
        BOOST_REQUIRE(!e.src().empty());
    }

    {
        auto& src = abc::d::A<int>::B<bool,double>::my_fun();
        {
            auto  str = src.to_string();
            std::regex re("test_error.cpp:\\d+ A::B::my_fun$");
            BOOST_CHECK(std::regex_search(str, re));
        }
        {
            auto  str = src.to_string("","",3);
            std::regex re("test_error.cpp:\\d+ A::B::my_fun$");
            BOOST_CHECK(std::regex_search(str, re));
        }
        {
            auto  str = src.to_string("","",10);
            std::regex re("test_error.cpp:\\d+ abc::d::A::B::my_fun$");
            BOOST_CHECK(std::regex_search(str, re));
        }
        {
            auto  str = src.to_string("","",0);
            std::regex re("^test_error.cpp:\\d+$");
            BOOST_CHECK(std::regex_search(str, re));
        }
        {
            auto  str = src.to_string("","",1);
            std::regex re("^test_error.cpp:\\d+ my_fun$");
            BOOST_CHECK(std::regex_search(str, re));
        }
    }

    {
        auto info1 = utxx::src_info("A", "BB");
        utxx::src_info&& info2 = std::move(info1);

        BOOST_CHECK_EQUAL("A",  info2.srcloc());
        BOOST_CHECK_EQUAL(1,    info2.srcloc_len());
        BOOST_CHECK_EQUAL("BB", info2.fun());
        BOOST_CHECK_EQUAL(2,    info2.fun_len());
    }
}
