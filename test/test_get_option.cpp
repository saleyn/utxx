//----------------------------------------------------------------------------
/// \file  test_get_option.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes and functions in the url.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-09-19
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects

Copyright (C) 2015 Serge Aleynikov <saleyn@gmail.com>

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
#include <utxx/get_option.hpp>
#include <iostream>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_get_option )
{
    const char* argv[] = {"test", "-a", "10", "--out=file", "-t", "true",
                          "-f", "-", "-x", "--", "-y", "/temp"};
    int         argc   = std::extent< decltype(argv) >::value;

    int         a;
    std::string out;
    bool        t;

    BOOST_CHECK(get_opt(argc, argv, &a, "-a"));
    BOOST_CHECK_EQUAL(10, a);
    BOOST_CHECK(get_opt(argc, argv, &out, "", "--out"));
    BOOST_CHECK_EQUAL("file", out);
    BOOST_CHECK(get_opt(argc, argv, &t, "-t"));
    BOOST_CHECK(t);

    opts_parser opts(argc, argv);

    while (opts.next()) {
        if (opts.match("-a", "", &a)) {
            BOOST_CHECK_EQUAL(10, a); continue;
        }
        if (opts.match("", "--out", &out)) {
            BOOST_CHECK_EQUAL("file", out); continue;
        }
        if (opts.match("-t", "", &t)) {
            BOOST_CHECK(t); continue;
        }
        if (opts.match("-f", "", &out)) {
            BOOST_CHECK_EQUAL("-", out); continue;
        }
        if (opts.match("-x", ""))
            continue;
        if (opts() == std::string("--"))
            continue;
        if (opts.match("-y","", &out)) {
            BOOST_CHECK_EQUAL("/temp", out); continue;
        }
        BOOST_CHECK(false);
    }

    BOOST_CHECK(opts.find("-a", "", &a));
    BOOST_CHECK_EQUAL(10, a);
}
