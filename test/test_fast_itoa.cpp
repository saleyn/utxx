//----------------------------------------------------------------------------
/// \file  test_fast_itoa.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for fast_itoa conversion class.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-30
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx project.

Copyright (C) 2017 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/config.h>

#include <boost/test/unit_test.hpp>
#include <utxx/fast_itoa.hpp>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_fast_itoa )
{
    char buf[20];
    auto p = fast_itoa(12345,  buf);
    BOOST_CHECK_EQUAL("12345", buf);
    BOOST_CHECK_EQUAL(5, p - buf);

    p = fast_itoa(0,  buf);
    BOOST_CHECK_EQUAL("0", buf);
    BOOST_CHECK_EQUAL(1, p - buf);

    p = fast_itoa(-1,  buf);
    BOOST_CHECK_EQUAL("-1", buf);
    BOOST_CHECK_EQUAL(2, p - buf);

    p = fast_itoa(12345u,  buf);
    BOOST_CHECK_EQUAL("12345", buf);
    BOOST_CHECK_EQUAL(5, p - buf);

    p = fast_itoa(0u,  buf);
    BOOST_CHECK_EQUAL("0", buf);
    BOOST_CHECK_EQUAL(1, p - buf);

    p = fast_itoa(-123456781234545L,  buf);
    BOOST_CHECK_EQUAL("-123456781234545", buf);
    BOOST_CHECK_EQUAL(16, p - buf);
}


