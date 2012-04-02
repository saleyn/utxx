//----------------------------------------------------------------------------
/// \file  test_path.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes and functions in the path.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-05-06
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects

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

#include <boost/test/unit_test.hpp>
#include <util/path.hpp>
#include <util/verbosity.hpp>
#include <iostream>

using namespace util;

BOOST_AUTO_TEST_CASE( test_path_slash )
{
#if defined(__windows__) || defined(_WIN32) || defined(_WIN64)
    BOOST_REQUIRE_EQUAL('\\', path::slash());
#else
    BOOST_REQUIRE_EQUAL('/', path::slash());
#endif
}

BOOST_AUTO_TEST_CASE( test_path_replace_env_vars )
{
    const char* h = getenv("HOME");
    BOOST_REQUIRE(h != NULL);

    std::string home(h);

    std::string s = path::replace_env_vars("${HOME}/path/to/exe");
    BOOST_REQUIRE_EQUAL(home+"/path/to/exe", s);

    s = path::replace_env_vars("${HOME}/path$HOME/exe");
    BOOST_REQUIRE_EQUAL(home+"/path"+home+"/exe", s);

    s = path::replace_env_vars("/tmp$HOME/path/to/exe");
    BOOST_REQUIRE_EQUAL(
        std::string("/tmp") + home + "/path/to/exe", s);

    s = path::replace_env_vars("~/path/to/exe");
    BOOST_REQUIRE_EQUAL(home+"/path/to/exe", s);
}


BOOST_AUTO_TEST_CASE( test_path_filename_with_backup )
{
    const char* h = getenv("HOME");
    BOOST_REQUIRE(h != NULL);

    std::string home(h);

    struct tm tm;
    tm.tm_year = 100;
    tm.tm_mon  = 0;
    tm.tm_mday = 2;
    tm.tm_hour = 5;
    tm.tm_min  = 4;
    tm.tm_sec  = 3;

    std::pair<std::string, std::string>
        pair = path::filename_with_backup(
            "~/file%Y-%m-%d::%T.txt", NULL, NULL, &tm);

    BOOST_REQUIRE_EQUAL(home+"/file2000-01-02::05:04:03.txt", pair.first);
    BOOST_REQUIRE_EQUAL(
        home+"/file2000-01-02::05:04:03@2000-01-02.050403.txt", pair.second);
}

BOOST_AUTO_TEST_CASE( test_path_program )
{
    const std::string& name     = path::program::name();
    const std::string& rel_path = path::program::rel_path();
    const std::string& abs_path = path::program::abs_path();

    BOOST_REQUIRE(!name.empty());
#ifdef __linux__
    BOOST_REQUIRE(abs_path.size() > 0 && abs_path.c_str()[0] == path::slash());
#endif
    if (verbosity::level() > VERBOSE_NONE) {
        std::cout << "  Program name : " << name << std::endl;
        std::cout << "  Relative path: " << rel_path << std::endl;
        std::cout << "  Absolute path: " << abs_path << std::endl;
    }
}
