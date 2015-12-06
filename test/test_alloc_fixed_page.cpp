//----------------------------------------------------------------------------
/// \file  test_alloc_fixed_page.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes and functions in the path.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects

Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>

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

//#define _ALLOCATOR_MEM_DEBUG 1

#include <boost/test/unit_test.hpp>
#include <utxx/alloc_fixed_page.hpp>
#include <utxx/verbosity.hpp>
#include <vector>
#include <iostream>

using namespace utxx;

namespace {
    struct test { char buf[44]; };
}

BOOST_AUTO_TEST_CASE( test_alloc_fixed_page )
{
    {
        void* p;
        BOOST_REQUIRE(posix_memalign(&p, 128, 32) >= 0);
        //BOOST_REQUIRE((p = memory::aligned_malloc(32, 128)) >= 0);
        //memory::aligned_free(p);
        free(p);
        BOOST_REQUIRE_EQUAL(0, (static_cast<char*>(p) - static_cast<char*>(0)) % 128);
    }

    memory::aligned_page_allocator<test, 128> alloc;
    std::vector<test*> v;
    for (int i = 0; i < 5; i++)
        v.push_back(alloc.allocate(1));
    for (int i = 0; i < 5; i++)
        alloc.deallocate(v[i], 1);

}

