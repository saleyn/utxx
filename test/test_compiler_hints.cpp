//----------------------------------------------------------------------------
/// \file   test_compiler_hints.cpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Test cases for compiler_hints macros.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-09-31
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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
#include <utxx/compiler_hints.hpp>
#include <iostream>

BOOST_AUTO_TEST_CASE( test_compiler_hints )
{
    BOOST_CHECK(LIKELY(1));
    BOOST_CHECK(UNLIKELY(1));
    BOOST_CHECK(utxx::likely(1));
    BOOST_CHECK(utxx::unlikely(1));
    bool res;
    res = UTXX_CHECK(1, true); BOOST_CHECK(res);
    BOOST_CHECK( UTXX_CHECK(0, true));
    BOOST_CHECK(!UTXX_CHECK(1, false));
    BOOST_CHECK(!UTXX_CHECK(0, false));
}