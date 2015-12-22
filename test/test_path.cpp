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
#include <utxx/path.hpp>
#include <utxx/verbosity.hpp>
#include <iostream>
#include <fstream>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_path_slash )
{
#if defined(__windows__) || defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)
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

    s = path::replace_env_vars(path::temp_path() + "$HOME/path/to/exe");
    BOOST_REQUIRE_EQUAL(
        path::temp_path() + home + "/path/to/exe", s);

    s = path::replace_env_vars("~/path/to/exe");
    BOOST_REQUIRE_EQUAL(home+"/path/to/exe", s);

    s = path::replace_env_vars(path::temp_path() + "/file%Y-%m-%d::%T.txt");
    BOOST_REQUIRE_EQUAL(path::temp_path() + "/file%Y-%m-%d::%T.txt", s);

    struct tm tm;
    tm.tm_year = 100;
    tm.tm_mon  = 0;
    tm.tm_mday = 2;
    tm.tm_hour = 5;
    tm.tm_min  = 4;
    tm.tm_sec  = 3;
    s = path::replace_env_vars(path::temp_path() + "/file%Y-%m-%d::%T.txt", &tm);
    BOOST_REQUIRE_EQUAL(path::temp_path() + "/file2000-01-02::05:04:03.txt", s);

    std::string str = "${env}/${instance}";
    std::map<std::string, std::string> bindings
    {
        {"env",      "one"},
        {"instance", "two"}
    };

    s = path::replace_env_vars(str, time_val(), false, &bindings);
    BOOST_CHECK_EQUAL("one/two", s);

    s = path::replace_macros("abc {{env}}-{{instance}}", bindings);
    BOOST_CHECK_EQUAL("abc one-two", s);
}

BOOST_AUTO_TEST_CASE( test_path_symlink )
{
    auto tp = path::temp_path();
    BOOST_CHECK(path::is_dir(tp));

    auto fn = "xxx-file-name.test.txt";
    auto p  = path::temp_path(fn);
    auto s  = path::temp_path("xxx-file-link.test.link");
    if (path::file_exists(p))
        BOOST_CHECK(path::file_unlink(p));
    if (path::file_exists(s))
        BOOST_CHECK(path::file_unlink(s));

    BOOST_CHECK_EQUAL(tp, path::dirname(p));
    BOOST_CHECK_EQUAL(fn, path::basename(p));
    BOOST_CHECK_EQUAL("xxx-file-name.test", path::basename(p, ".txt"));

    BOOST_CHECK(!path::file_exists(p));
    BOOST_CHECK(!path::file_exists(s));

    BOOST_CHECK(path::write_file(p, "test"));
    BOOST_CHECK_EQUAL("test", path::read_file(p));

    BOOST_CHECK(path::is_regular(p));

    BOOST_CHECK(path::file_rename(p, s));
    BOOST_CHECK(!path::file_exists(p));
    BOOST_CHECK(path::file_exists(s));
    BOOST_CHECK(path::file_rename(s, p));
    BOOST_CHECK(path::file_exists(p));
    BOOST_CHECK(!path::file_exists(s));

    BOOST_CHECK(path::file_symlink(p, s));
    BOOST_CHECK(path::is_symlink(s));
    BOOST_CHECK_EQUAL(p, path::file_readlink(s));

    BOOST_CHECK(path::file_unlink(p));
    BOOST_CHECK(path::file_unlink(s));

    BOOST_CHECK(path::write_file(p, "test"));
    BOOST_CHECK(path::write_file(p+"1", "test"));

    // s -> "*test.txt1"
    BOOST_CHECK(path::file_symlink(p+"1", s));
    BOOST_CHECK_EQUAL(p+"1", path::file_readlink(s));
    // s -> "*test.txt"
    BOOST_CHECK(path::file_symlink(p, s, true));
    BOOST_CHECK_EQUAL(p, path::file_readlink(s));
    BOOST_CHECK(path::file_unlink(s));
    BOOST_CHECK(path::file_unlink(p));
    // create a conflicting file equal to the name of the link "*test.link"
    path::file_unlink(s+".tmp");
    BOOST_CHECK(!path::is_regular(s+".tmp"));
    BOOST_CHECK(path::write_file(s, "test"));
    BOOST_CHECK(path::write_file(p, "test"));
    BOOST_CHECK(!path::is_regular(s+".tmp"));
    BOOST_CHECK(path::file_symlink(p, s, true));
    BOOST_CHECK_EQUAL(p, path::file_readlink(s));
    BOOST_CHECK(path::is_regular(s+".tmp"));

    BOOST_CHECK(path::file_unlink(p));
    BOOST_CHECK(path::file_unlink(p+"1"));
    BOOST_CHECK(path::file_unlink(s+".tmp"));
    BOOST_CHECK(path::file_unlink(s));

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

BOOST_AUTO_TEST_CASE( test_path_file_exists )
{
    auto s_filename = path::temp_path() + "/test_file_123.qqq";

    BOOST_REQUIRE(!path::file_exists(s_filename));

    std::fstream file;
    file.open(s_filename, std::ios_base::out); // create the file
    BOOST_REQUIRE(file.is_open());
    file.close();

    BOOST_REQUIRE(path::file_exists(s_filename));

    path::file_unlink(s_filename);

    file.open(s_filename, std::ios_base::in); // won't create the file
    BOOST_REQUIRE(!file.is_open());
    file.close();
}

BOOST_AUTO_TEST_CASE( test_path_split_join )
{
    auto res = path::split(path::temp_path() + path::slash_str() + "abc.txt");
    BOOST_CHECK_EQUAL(path::temp_path(), res.first);
    BOOST_CHECK_EQUAL("abc.txt",   res.second);
    res = path::split("abc.txt");
    BOOST_CHECK_EQUAL("",          res.first);
    BOOST_CHECK_EQUAL("abc.txt",   res.second);

    auto s   = path::join(path::temp_path(), "abc.txt");
    auto exp = path::temp_path() + path::slash_str() + "abc.txt";
    BOOST_CHECK_EQUAL(exp, s);
    BOOST_CHECK_EQUAL("abc.txt", path::join("", "abc.txt"));

    // Pass as rvalue
    s   = path::join(std::vector<std::string>{"a", "b", "c"});
    exp = std::string("a") + path::slash_str() + "b" + path::slash_str() + "c";
    BOOST_CHECK_EQUAL(exp, s);

    // Pass as lvalue
    std::vector<std::string> v = std::vector<std::string>{"a", "b", "c"};
    s   = path::join(v);
    exp = std::string("a") + path::slash_str() + "b" + path::slash_str() + "c";
    BOOST_CHECK_EQUAL(exp, s);
}

BOOST_AUTO_TEST_CASE( test_path_list_files )
{
    auto create_file = [](const char* name) {
        auto file = path::temp_path() + path::slash_str() + name;

        std::ofstream f(file, std::ios::trunc);
        BOOST_REQUIRE(f.is_open());
    };

    create_file("test_file_1.bin");
    create_file("test_file_2.bin");
    create_file("test_file_3.bin");

    auto res = path::list_files(path::temp_path(), "test_file_[1-3]\\.bin", FileMatchT::REGEX);
    BOOST_CHECK(res.first);
    BOOST_CHECK_EQUAL(3u, res.second.size());

    res = path::list_files(path::temp_path(), "test_file_?.bin");
    BOOST_CHECK(res.first);
    BOOST_CHECK_EQUAL(3u, res.second.size());

    res = path::list_files(path::temp_path(), "test_file_?.b*", FileMatchT::WILDCARD);
    BOOST_CHECK(res.first);
    BOOST_CHECK_EQUAL(3u, res.second.size());

    res = path::list_files(path::temp_path() + path::slash_str() + "test_file_?.b*", FileMatchT::WILDCARD);
    BOOST_CHECK(res.first);
    BOOST_CHECK_EQUAL(3u, res.second.size());

    res = path::list_files(path::temp_path(), "test_file_", FileMatchT::PREFIX, true);
    BOOST_CHECK(res.first);
    BOOST_REQUIRE_EQUAL(3u, res.second.size());
    auto fn = path::join(path::temp_path(), "test_file_3.bin");
    BOOST_CHECK_EQUAL(fn, *res.second.begin());

    res = path::list_files(path::temp_path(), "test_file_", FileMatchT::PREFIX);
    BOOST_CHECK(res.first);
    BOOST_REQUIRE_EQUAL(3u, res.second.size());
    BOOST_CHECK_EQUAL("test_file_3.bin", *res.second.begin());

    for (auto& f : res.second) {
        path::file_unlink(path::temp_path() + f);
    }

    BOOST_CHECK(!path::file_exists("test_file_1.bin"));
    BOOST_CHECK(!path::file_exists("test_file_2.bin"));
    BOOST_CHECK(!path::file_exists("test_file_3.bin"));
}