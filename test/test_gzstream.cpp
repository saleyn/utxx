//----------------------------------------------------------------------------
/// \file  test_gzstream.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes and functions in the gzstream.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Copyright (c) 2001 Deepak Bandyopandhyay, Lutz Kettner
// Created: 2015-09-06
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
#include <utxx/path.hpp>
#include <utxx/gzstream.hpp>
#include <utxx/scope_exit.hpp>
#include <utxx/string.hpp>
#include <iostream>
#include <fstream>

using namespace utxx;

static std::string temp_path(const std::string& a_add_str = "") {
    #if defined(__windows__) || defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
    auto    p = getenv("TEMP");
    if (!p) p = "";
    #else
    auto    p = P_tmpdir;
    #endif
    auto r = std::string(p);
    return (a_add_str.empty()) ? r : r + path::slash_str() + a_add_str;
}

#ifdef UTXX_HAVE_LIBZ

BOOST_AUTO_TEST_CASE( test_gzstream_gzip )
{
    auto ss = temp_path("xxxx");
    auto dd = temp_path("xxxx.gz");

    BOOST_REQUIRE(path::write_file(ss, "this is a test1\nthis is a test2\n"));

    UTXX_SCOPE_EXIT([&]() { path::file_unlink(ss); });
    UTXX_SCOPE_EXIT([&]() { path::file_unlink(dd); });

    // check alternate way of opening file
    {
        ogzstream out;
        out.open(dd);
        BOOST_REQUIRE(out.good());
        out.close();
        BOOST_REQUIRE(out.good());
    }

    // now use the shorter way with the constructor to open the same file
    ogzstream out(dd);
    BOOST_REQUIRE(out.good());

    std::ifstream in(ss);
    BOOST_REQUIRE(in.good());

    auto res = gzsetparams(out.rdbuf()->native_handle(), 9, Z_DEFAULT_STRATEGY);
    BOOST_REQUIRE_EQUAL(0, res);

    char c;
    while (in.get(c))
        out << c;

    in.close();
    out.close();

    BOOST_REQUIRE(in.eof());
    BOOST_REQUIRE(out.good());

    BOOST_REQUIRE_EQUAL(39, utxx::path::file_size(dd));

    {
        igzstream in(dd);
        BOOST_REQUIRE(in.good());

        for (int i=1; i <= 3; ++i) {
            std::string s;
            std::getline(in, s);
            BOOST_REQUIRE((i == 3) ^ in.good());
            if (i != 3)
                BOOST_REQUIRE_EQUAL(utxx::to_string("this is a test", i), s);
        }
    }
}

#endif // UTXX_HAVE_LIBZ
