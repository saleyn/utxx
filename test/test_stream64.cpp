//----------------------------------------------------------------------------
/// \file  test_stream64.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for stream64.hpp.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2013-12-05
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is a part of the utxx open-source project.

Copyright (C) 2013 Serge Aleynikov <saleyn@gmail.com>

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
#include <boost/filesystem.hpp>
#include <utxx/io/stream64.hpp>

BOOST_AUTO_TEST_CASE( test_stream64 ) {
    static const std::string file =
        (boost::filesystem::temp_directory_path() / "test_stream64").c_str();

    {
        std::ofstream64 out(file.c_str());

        //BOOST_REQUIRE(out.is_open());

        for (int i = 0; i < 1000; i++)
            out << "This is a test " << i << std::endl;
    }

    {
        std::ifstream64 in(file.c_str());

        //BOOST_REQUIRE(in.is_open());

        for (int i = 0; i < 1000; i++) {
            std::string s;
            std::getline(in, s, '\n');
            std::stringstream b;
            b << "This is a test " << i;
            BOOST_REQUIRE_EQUAL(b.str(), s);
        }
    }

    boost::filesystem::remove(file);
}
