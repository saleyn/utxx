//----------------------------------------------------------------------------
/// \brief Test file for stack_container.hpp
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2016-02-24
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

Copyright (C) 2016 Serge Aleynikov <saleyn@gmail.com>

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
#include <utxx/container/stack_container.hpp>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_stack_container )
{
    stack_allocator<char, 10>::storage storage;
    stack_allocator<char, 10> alloc(&storage);
    auto p = alloc.allocate(10);
    BOOST_CHECK(alloc.used_stack());
    alloc.deallocate(p, 0);
    BOOST_CHECK(!alloc.used_stack());
    p = alloc.allocate(1);
    BOOST_CHECK(alloc.used_stack());
    alloc.deallocate(p, 0);
    BOOST_CHECK(!alloc.used_stack());
    p = alloc.allocate(11);
    BOOST_CHECK(!alloc.used_stack());
    alloc.deallocate(p, 0);

    basic_stack_string<15> str;
    BOOST_CHECK_EQUAL(15, str.container().capacity());
    str.container() = "01";
    auto* p1 = (const char*)&str + sizeof(str)-16;
    auto* p2 = &str.container()[0];
    BOOST_CHECK(p1 == p2);
    str.container() = "012345678";
    p1 = (const char*)&str + sizeof(str)-16;
    p2 = &str.container()[0];
    BOOST_CHECK(p1 == p2);
    str.container() = "0123";
    p1 = (const char*)&str + sizeof(str)-16;
    p2 = &str.container()[0];
    BOOST_CHECK(p1 == p2);
    str.container() = "012345678912345678";
    p1 = (const char*)&str + sizeof(str)-16;
    p2 = &str.container()[0];
    BOOST_CHECK(p1 != p2);
}