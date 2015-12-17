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
#include <utxx/string.hpp>
#include <boost/test/unit_test.hpp>
#include <regex>
#include <type_traits>

using namespace utxx;

const src_info& sample_src() { static const auto s_src = UTXX_SRC; return s_src; }

namespace abc {
    namespace d   {
        struct EventType {
            enum etype { a,b,c };
        };

        template <class T>
        struct A {

            template <EventType::etype ET>
            struct ChannelEvent {};

            template <EventType::etype ET>
            static typename std::enable_if<ET == EventType::a ||
                                    ET == EventType::b ||
                                    ET == EventType::c, const src_info&>::type
            OnData(const ChannelEvent<ET>& a) {
                static const auto s_src = UTXX_SRC; return s_src;
            };

            template <class U, class V>
            struct B {
                static const src_info& my_fun() { static const auto s_src = UTXX_SRC; return s_src; }
                static const src_info& my_funx() {
                    UTXX_PRETTY_FUNCTION();
                    static const auto s_src = UTXX_SRCX; return s_src;
                }
            };
        };
    } // namespace d

    static const src_info* test_static(std::string const&) {
        static const auto s_src = UTXX_SRC;
        return &s_src;
    }

    static src_info lambda(utxx::src_info&& a_src) {
        return a_src;
    }

    static src_info do_lambda() {
        auto fun = [](){ return lambda(UTXX_SRC); };
        return fun();
    }
} // namespace abc

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

    try {
        UTXX_RETHROW(throw std::runtime_error("Test"));
    } catch (utxx::runtime_error const& e) {
        BOOST_REQUIRE_EQUAL("Test", e.str());
    }
}

BOOST_AUTO_TEST_CASE( test_error_srcloc )
{
    UTXX_PRETTY_FUNCTION(); // Cache pretty function name

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

    try {
        UTXX_THROWX_BADARG_ERROR("A ", 222);
    } catch (utxx::runtime_error& e) {
        BOOST_CHECK_EQUAL("A 222", e.str());
        std::regex re("\\[test_error.cpp:\\d+ test_error_srcloc::test_method\\] A 222$");
        BOOST_REQUIRE(!e.src().empty());
        if (!std::regex_search(e.what(), re)) {
            std::cout << e.what() << std::endl;
            BOOST_CHECK(false);
        }
    }

    {
        utxx::src_info src[] = {
            abc::d::A<int>::B<bool,double>::my_fun(),
            abc::d::A<int>::B<bool,double>::my_funx()
        };
        for (auto i=0u; i < length(src); ++i) {
            auto str = src[i].to_string();
            std::regex re("test_error.cpp:\\d+ A::B::my_fun[x]?$");
            if (!std::regex_search(str, re)) {
                std::cout << str << std::endl;
                BOOST_CHECK(false);
            }
        }
        for (auto i=0u; i < length(src); ++i) {
            auto str = src[i].to_string("", "", 3);
            std::regex re("test_error.cpp:\\d+ A::B::my_fun[x]?$");
            if (!std::regex_search(str, re)) {
                std::cout << str << std::endl;
                BOOST_CHECK(false);
            }
        }
        for (auto i=0u; i < length(src); ++i) {
            auto str = src[i].to_string("", "", 10);
            std::regex re(i==0 ? "test_error.cpp:\\d+ abc::d::A::B::my_fun$"
                               : "test_error.cpp:\\d+ A::B::my_funx$");
            if (!std::regex_search(str, re)) {
                std::cout << str << std::endl;
                BOOST_CHECK(false);
            }
        }
        for (auto i=0u; i < length(src); ++i) {
            auto str = src[i].to_string("", "", 0);
            std::regex re("^test_error.cpp:\\d+$");
            if (!std::regex_search(str, re)) {
                std::cout << '"' << str << '"' << std::endl;
                BOOST_CHECK(false);
            }
        }
        {
            auto str = src[0].to_string("", "", 1);
            std::regex re("^test_error.cpp:\\d+ my_fun[x]?$");
            if (!std::regex_search(str, re)) {
                std::cout << '"' << str << '"' << std::endl;
                BOOST_CHECK(false);
            }
            // my_funx() printing scope is controlled by
            // src_info_defaults::print_fun_scopes, and therefore scope argument
            // 1 is overriden by 3:
            str = src[1].to_string("", "", 1);
            re  = "^test_error.cpp:\\d+ A::B::my_fun[x]?$";
            if (!std::regex_search(str, re)) {
                std::cout << '"' << str << '"' << std::endl;
                BOOST_CHECK(false);
            }
        }

        {
            auto* si = abc::test_static("");
            auto str = si->to_string("", "", 3);
            std::regex re("^test_error.cpp:\\d+ abc::test_static$");
            if (!std::regex_search(str, re)) {
                std::cout << '"' << str << '"' << std::endl;
                BOOST_CHECK(false);
            }

            auto sii = abc::do_lambda();
            str = sii.to_string("", "", 3);
            re  = std::regex("^test_error.cpp:\\d+ abc::do_lambda$");
            if (!std::regex_search(str, re)) {
                std::cout << '"' << str << '"' << std::endl;
                BOOST_CHECK(false);
            }

            src_info ci
            (
                UTXX_FILE_SRC_LOCATION,
                "auto mqt::(anonymous class)::operator()(const std::string &) const"
            );
            str = ci.to_string("", "", 3);
            re  = std::regex("^test_error.cpp:\\d+ mqt::\\(anonymous class\\)::operator\\(\\)$");
            if (!std::regex_search(str, re)) {
                std::cout << '"' << str << '"' << std::endl;
                BOOST_CHECK(false);
            }
        }

        {
            src_info ci
            (
                UTXX_FILE_SRC_LOCATION,
                "auto main(int, char **)::(anonymous class)::operator()(io::FdInfo &, int, int) const"
            );
            auto str = ci.to_string("", "", 3);
            auto re  = std::regex("^test_error.cpp:\\d+ main$");
            if (!std::regex_search(str, re)) {
                std::cout << '"' << str << '"' << std::endl;
                BOOST_CHECK(false);
            }
        }

        {
            auto& s = abc::d::A<int>::OnData<abc::d::EventType::b>(abc::d::A<int>::ChannelEvent<abc::d::EventType::b>());
            //BOOST_TEST_MESSAGE(s.fun());
            auto str = s.to_string("","",3);
            //BOOST_TEST_MESSAGE(str);
            std::regex re("^test_error.cpp:\\d+ d::A::OnData$");
            if (!std::regex_search(str, re)) {
                std::cout << '"' << str << '"' << std::endl;
                BOOST_CHECK(false);
            }
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
    {
        utxx::src_info si("X:10",
            "void cme::Thread<cme::MDP<cme::MD<MDB, MyTraits>, Traits> >::Run() [Impl = MB]");
        char buf[80];
        auto str = si.to_string(buf, sizeof(buf));
        BOOST_CHECK_EQUAL("X:10 cme::Thread::Run", std::string(buf, str - buf));
    }
    {
        utxx::src_info si("X:10",
            "void cme::A<xx::C<U, V>>::B<U, V>::Run()");
        char buf[80];
        auto str = si.to_string(buf, sizeof(buf));
        BOOST_CHECK_EQUAL("X:10 A::B::Run", std::string(buf, str - buf));
    }
}
