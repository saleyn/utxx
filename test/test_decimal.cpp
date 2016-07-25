//----------------------------------------------------------------------------
/// \file  test_raw_char.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for raw_char class.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-30
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the REPLOG project.

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

#if __SIZEOF_LONG__ >= 8

#include <boost/test/unit_test.hpp>
#include <utxx/decimal.hpp>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_decimal )
{
    BOOST_CHECK_EQUAL(1e-1,     decimal::pow10(-1));
    BOOST_CHECK_EQUAL(0.1,      decimal::pow10(-1));
    BOOST_CHECK_EQUAL(0.01,     decimal::pow10(-2));
    BOOST_CHECK_EQUAL(1e-10,    decimal::pow10(-10));
    BOOST_CHECK_EQUAL(1e-11,    decimal::pow10(-11));
    BOOST_CHECK_EQUAL(1e-12,    decimal::pow10(-12));
    BOOST_CHECK_EQUAL(1.0,      decimal::pow10(0));
    BOOST_CHECK_EQUAL(1e+1,     decimal::pow10(1));
    BOOST_CHECK_EQUAL(1e+5,     decimal::pow10(5));
    BOOST_CHECK_EQUAL(1e+10,    decimal::pow10(10));
    BOOST_CHECK_EQUAL(1e+11,    decimal::pow10(11));
    BOOST_CHECK_EQUAL(1e+12,    decimal::pow10(12));

    { decimal d; BOOST_CHECK(!d.is_null()); BOOST_CHECK_EQUAL(0.0, (double)d); }
    { decimal d(nullptr);       BOOST_CHECK(d.is_null()); }
    { decimal d(decimal::nan());BOOST_CHECK(d.is_null()); }
    { BOOST_CHECK_EQUAL(decimal(1,1),  decimal(1,1)); }
    { BOOST_CHECK_NE(   decimal(-1,1), decimal(1,1)); }
    { BOOST_CHECK_NE(   decimal(2,1),  decimal(1,1)); }
    { BOOST_CHECK_NE(   decimal(-2,1), decimal(1,1)); }
    { BOOST_CHECK_EQUAL(decimal(127,0),decimal::null_value()); }
    { decimal d( 2,  1);        BOOST_CHECK_EQUAL(100.0, (double)d); }
    { decimal d(-2,  1);        BOOST_CHECK_EQUAL(0.01,  (double)d); }
    { decimal d(-2, -125);      BOOST_CHECK_EQUAL(-1.25, (double)d); }
    { decimal d(-11,-125678901234);
                                BOOST_CHECK_EQUAL("-1.25678901234", d.to_string()); }
    { decimal d(-11, 125678901234);
                                BOOST_CHECK_EQUAL("1.25678901234", d.to_string()); }
    { decimal d(-15, 1256789012345678);
                                BOOST_CHECK_EQUAL("1.256789012345678",d.to_string());}
}

#endif
