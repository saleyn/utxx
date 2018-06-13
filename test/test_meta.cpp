//----------------------------------------------------------------------------
/// \file  test_meta.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes and functions in the meta.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-05-06
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
#include <boost/static_assert.hpp>
#include <utxx/meta.hpp>
#include <utxx/compiler_hints.hpp>
#include <iostream>

using namespace utxx;

int& test_it (int& i) { return i; }

struct eval_tester {
    int a = 10;
    int operator()()     {return a;}
    int operator()(int n){return a+n;}
    int triple()         {return a*3;}
};

int add_one(int n) { return n+1; }

struct A     { const char* name() { return "A"; } };
struct B : A { const char* name() { return "B"; } };
struct C     { const char* name() { return "C"; } };

template <class T> bool check(if_base_of<A, T>     const& v) { return true;  }
template <class T> bool check(if_not_base_of<A, T> const& v) { return false; }

template <class T>
typename std::enable_if<std::is_base_of<A,T>::value, bool>::type
check2() { return true; }
template <class T>
typename std::enable_if<!std::is_base_of<A,T>::value, bool>::type
check2() { return false; }

template <class T> if_base_of<A,T,bool>
check3() { return true; }
template <class T> if_not_base_of<A,T,bool>
check3() { return false; }

BOOST_AUTO_TEST_CASE( test_meta )
{
    enum class E { B1 = 1, B2 = 2 };

    int t = 10;
    int j = test_it(out(t));

    BOOST_REQUIRE_EQUAL(10, j);

    BOOST_STATIC_ASSERT(1        == to_underlying(E::B1));
    BOOST_REQUIRE_EQUAL(1,          to_underlying(E::B1));

    BOOST_STATIC_ASSERT(0x00     == to_int<'\0'>::value());
    BOOST_STATIC_ASSERT(0x01     == to_int<'\1'>::value());
    BOOST_STATIC_ASSERT(0x0100   == to_int<'\1', '\0'>::value());
    BOOST_STATIC_ASSERT(0x0102   == to_int<'\1', '\2'>::value());
    BOOST_STATIC_ASSERT(0x010203 == to_int<'\1', '\2', '\3'>::value());

    BOOST_STATIC_ASSERT(0        == upper_power<0,  2>::value);
    BOOST_STATIC_ASSERT(1        == upper_power<1,  2>::value);
    BOOST_STATIC_ASSERT(2        == upper_power<2,  2>::value);
    BOOST_STATIC_ASSERT(4        == upper_power<3,  2>::value);
    BOOST_STATIC_ASSERT(16       == upper_power<15, 2>::value);
    BOOST_STATIC_ASSERT(32       == upper_power<32, 2>::value);

    {
        auto  lambda = [](int i, int j) { return long(i*10 + j); };
        using traits = function_traits<decltype(lambda)>;

        static_assert(std::is_same<long, traits::result_type>::value,  "err");
        static_assert(std::is_same<int,  traits::arg<0>::type>::value, "err");
    }

    {
        eval_tester ttt;

        // free function
        BOOST_CHECK_EQUAL(1, eval(add_one,0));

        // lambda function
        BOOST_CHECK_EQUAL(2, eval([](int n){return n+1;},1));

        // functor
        BOOST_CHECK_EQUAL(10, eval(ttt));
        BOOST_CHECK_EQUAL(14, eval(ttt,4));

        // member function
        BOOST_CHECK_EQUAL(30, eval(&eval_tester::triple,ttt));

        // member object
        //eval(&eval_tester::a, &ttt)++; // increment a by reference
        BOOST_CHECK_EQUAL(10, eval(&eval_tester::a, ttt));
    }

    {
        /*
        std::cout << std::boolalpha;
        std::cout << "a2b: " << std::is_base_of<A, B>::value << '\n';
        std::cout << "b2a: " << std::is_base_of<B, A>::value << '\n';
        std::cout << "c2b: " << std::is_base_of<C, B>::value << '\n';
        std::cout << "c2c: " << std::is_base_of<C, C>::value << '\n';
        */

        A a;
        B b;
        C c;
        BOOST_CHECK(( std::is_base_of<A,A>::value));
        BOOST_CHECK(( std::is_base_of<A,B>::value));
        BOOST_CHECK((!std::is_base_of<B,A>::value));
        BOOST_CHECK((!std::is_base_of<A,C>::value));

        BOOST_CHECK( check<A>(a));
        BOOST_CHECK( check<B>(b));
        BOOST_CHECK(!check<C>(c));

        BOOST_CHECK( check2<A>());
        BOOST_CHECK( check2<B>());
        BOOST_CHECK(!check2<C>());

        BOOST_CHECK( check3<A>());
        BOOST_CHECK( check3<B>());
        BOOST_CHECK(!check3<C>());
    }

    BOOST_CHECK(( has_type<int,        std::tuple<int, long, const char*>>::value));
    BOOST_CHECK(( has_type<int64_t,    std::tuple<int, long, const char*>>::value));
    BOOST_CHECK((!has_type<double,     std::tuple<int, long, const char*>>::value));
    BOOST_CHECK((!has_type<const int&, std::tuple<int, long, const char*>>::value));

    BOOST_CHECK(( has_type_nocvref<int,        std::tuple<int, long, const char*>>::value));
    BOOST_CHECK(( has_type_nocvref<int64_t,    std::tuple<int, long, const char*>>::value));
    BOOST_CHECK(( std::is_same<typename std::remove_reference<int&>::type, int>::value));
    BOOST_CHECK(( std::is_same<typename std::remove_const
                              <typename std::remove_reference<const int&>::type>::type, int>::value));
    BOOST_CHECK((!std::is_same<typename std::remove_reference
                              <typename std::remove_const<const int&>::type>::type, int>::value));
    BOOST_CHECK(( has_type_nocvref<const int&, std::tuple<int, long, const char*>>::value));
}
