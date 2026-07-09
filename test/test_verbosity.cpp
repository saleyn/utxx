//----------------------------------------------------------------------------
/// \file  test_verbosity.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating verbosity.hpp functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2004-07-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of utxx open-source project.

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
//#define BOOST_TEST_MODULE bitmap_test 

#include <utxx/test_helper.hpp>
#include <utxx/verbosity.hpp>
#include <utxx/scope_exit.hpp>

using namespace utxx;

TEST_CASE( test_verbosity )
{
    verbose_type old = verbosity::level();
    {
        // Restore verbosity on exiting scope
        scope_exit g([&old]() { verbosity::level(old); });

        verbosity::level(VERBOSE_DEBUG);

        REQUIRE_EQUAL(VERBOSE_DEBUG, verbosity::level());
        REQUIRE_EQUAL("debug", verbosity::c_str());

        REQUIRE( verbosity::enabled(VERBOSE_DEBUG));
        REQUIRE( verbosity::enabled(VERBOSE_TEST));
        REQUIRE(!verbosity::enabled(VERBOSE_INFO));

        {
            int n = 0;
            verbosity::if_enabled(VERBOSE_INFO,  [&n]() { n = 1; });
            REQUIRE(!n);
            verbosity::if_enabled(VERBOSE_DEBUG, [&n]() { n = 1; });
            REQUIRE(n);
        }

    }
    REQUIRE_EQUAL(old, verbosity::level());

    REQUIRE_EQUAL(VERBOSE_TEST,    verbosity::parse("test"));
    REQUIRE_EQUAL(VERBOSE_DEBUG,   verbosity::parse("debug"));
    REQUIRE_EQUAL(VERBOSE_DEBUG,   verbosity::parse("DEBUG"));
    REQUIRE_EQUAL(VERBOSE_INFO,    verbosity::parse("info"));
    REQUIRE_EQUAL(VERBOSE_MESSAGE, verbosity::parse("message"));
    REQUIRE_EQUAL(VERBOSE_WIRE,    verbosity::parse("wire"));
    REQUIRE_EQUAL(VERBOSE_TRACE,   verbosity::parse("trace"));
    REQUIRE_EQUAL(VERBOSE_NONE,    verbosity::parse("other"));
    REQUIRE_EQUAL(VERBOSE_INVALID, verbosity::parse("other", NULL, true));
    REQUIRE_EQUAL(VERBOSE_WIRE,    verbosity::parse(NULL, "wire"));
    REQUIRE_EQUAL(VERBOSE_WIRE,    verbosity::parse("",   "wire"));
} 
