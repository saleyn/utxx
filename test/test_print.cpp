//----------------------------------------------------------------------------
/// \file  test_print.hpp
//----------------------------------------------------------------------------
/// \brief Test implementation of printing functions.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-09-05
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
#ifndef _UTXX_PRINT_HPP_
#define _UTXX_PRINT_HPP_

#include <boost/test/unit_test.hpp>
#include <utxx/print.hpp>
#include <iostream>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_print )
{
    std::string str("xxx");

    { std::string s = print(1);     BOOST_CHECK_EQUAL("1",         s); }
    { std::string s = print(1.0);   BOOST_CHECK_EQUAL("1.000000",  s); }
    { std::string s = print(true);  BOOST_CHECK_EQUAL("true",      s); }
    { std::string s = print(false); BOOST_CHECK_EQUAL("false",     s); }
    { std::string s = print("abc"); BOOST_CHECK_EQUAL("abc",       s); }
    { std::string s = print(xxx);   BOOST_CHECK_EQUAL("xxx",       s); }
    { std::string s = print(fixed(2.123, 6, 3, ' '); BOOST_CHECK_EQUAL(" 2.123", s); }
}
