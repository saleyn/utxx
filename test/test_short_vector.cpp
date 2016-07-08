//----------------------------------------------------------------------------
/// \file  test_short_vector.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the short_vector.hpp.
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

#include <utxx/short_vector.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace utxx;

template <class Char = char>
struct Alloc {
    using value_type = Char;
    Char* allocate  (size_t n)          { ++s_ac; ++s_count; return std::allocator<Char>().allocate(n); }
    void  deallocate(Char* p, size_t n) { --s_count; std::allocator<Char>().deallocate(p, n); }

    static long  allocations()     { return s_count; }
    static long  tot_allocations() { return s_ac;    }
private:
    static long s_count;
    static long s_ac;
};

template <class Char> long Alloc<Char>::s_count;
template <class Char> long Alloc<Char>::s_ac;

struct X {
    X(int i = 0) : m_i(i) {}

    int m_i;
};

BOOST_AUTO_TEST_CASE( test_short_vector )
{
    using Allocator = Alloc<int>;
    using ss = basic_short_vector<int, 16, Allocator, 1>;

    Allocator salloc;

    { ss s; BOOST_CHECK_EQUAL(0, s.size()); }
    {
        ss s(salloc);
        BOOST_CHECK_EQUAL(0,   s.size());
        BOOST_CHECK(!s.null());
        BOOST_CHECK(!s.allocated());
        BOOST_CHECK(s.begin() == s.end());
        s.push_back(5);
        BOOST_CHECK_EQUAL(1,   s.size());
        BOOST_CHECK_EQUAL(5,   s[0]);
        s[0] = 20;
        BOOST_CHECK_EQUAL(20,  s[0]);
        s.resize(10);
        BOOST_CHECK_EQUAL(10,  s.size());
        BOOST_CHECK(s.begin()+10 == s.end());
        BOOST_CHECK(!s.null());
        BOOST_CHECK(!s.allocated());

        BOOST_CHECK_EQUAL(0, Allocator::tot_allocations());

        s.reset();
        BOOST_CHECK_EQUAL(0,   s.size());
        BOOST_CHECK(!s.null());
        BOOST_CHECK(!s.allocated());
        s.set_null();
        BOOST_CHECK(s.null());
        BOOST_CHECK_EQUAL(-1, s.size());
        BOOST_CHECK(s.begin()  == s.end());
        BOOST_CHECK(s.cbegin() == s.cend());

        int dummy[] = {1, 2, 3, 4};
        s.append(dummy, 4);
        BOOST_CHECK(!s.null());
        BOOST_CHECK_EQUAL(4, s.size());
        BOOST_CHECK(s.begin()+4  == s.end());
        BOOST_CHECK(s.cbegin()+4 == s.cend());

        BOOST_CHECK(!s.allocated());

        s.set(nullptr, -1);
        BOOST_CHECK(s.null());
        BOOST_CHECK_EQUAL(-1, s.size());
        s.push_back(5);
        BOOST_CHECK(!s.null());
        BOOST_CHECK_EQUAL(1, s.size());
        BOOST_CHECK_EQUAL(5, s[0]);
        BOOST_CHECK_EQUAL(5, s[0]);
        BOOST_CHECK(s.begin()+1  == s.end());
        BOOST_CHECK(s.cbegin()+1 == s.cend());

        BOOST_CHECK_EQUAL(0, Allocator::tot_allocations());

        const std::vector<int> test(80, 100);

        s = test;                                   // <-- allocation 1

        BOOST_CHECK(s.allocated());
        BOOST_CHECK_EQUAL(1, Allocator::tot_allocations());
        BOOST_CHECK(s == test);

        BOOST_CHECK_EQUAL(1, Allocator::tot_allocations());
        s.reset();
        BOOST_CHECK(!s.allocated());
        BOOST_CHECK_EQUAL(0, s.size());

        BOOST_CHECK_EQUAL(1, Allocator::tot_allocations());
        BOOST_CHECK_EQUAL(0, Allocator::allocations());

        s.reserve(1000);

        BOOST_CHECK(s.allocated());
        BOOST_CHECK_EQUAL(0, s.size());
        BOOST_CHECK_EQUAL(2, Allocator::tot_allocations());
        BOOST_CHECK_EQUAL(1, Allocator::allocations());

        s.resize(1100);

        BOOST_CHECK(s.allocated());
        BOOST_CHECK_EQUAL(1100, s.size());
        BOOST_CHECK_EQUAL(3, Allocator::tot_allocations());
        BOOST_CHECK_EQUAL(1, Allocator::allocations());

        s.size(20);
        BOOST_CHECK(s.allocated());
        BOOST_CHECK_EQUAL(20, s.size());

        s.reset();
        BOOST_CHECK(!s.allocated());
        BOOST_CHECK_EQUAL(0, s.size());
        BOOST_CHECK_EQUAL(3, Allocator::tot_allocations());
        BOOST_CHECK_EQUAL(0, Allocator::allocations());

        ss s2({{1,2,3}}, salloc);
        ss s1{{1, 2, 3}};

        BOOST_CHECK_EQUAL(3, s1.size());
        BOOST_CHECK_EQUAL(3, s2.size());
        BOOST_CHECK(!s1.allocated());
        BOOST_CHECK(!s2.allocated());
    }

    {
        basic_short_vector<X, 10> v;
    }
}