//----------------------------------------------------------------------------
/// \brief Test file for stream_io.hpp
//----------------------------------------------------------------------------
// Copyright (c) 2016 Serge Aleynikov <saleyn@gmail.com>
// Created: 2016-02-26
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

Copyright (C) 2016 Serge Aleynikov <saleyn@gmail.com>

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
#include <utxx/stream_io.hpp>
#include <utxx/path.hpp>
#include <utxx/string.hpp>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_stream_io_read_values )
{
    auto tp = path::temp_path();
    BOOST_REQUIRE(path::is_dir(tp));
    auto fn = path::join(tp, "utxx-stream-io.test.txt");
    path::file_unlink(fn);

    path::write_file(fn, "1 2 3\n4 5 6\n7 8 9\n");

    {
        std::ifstream in(fn);
        BOOST_REQUIRE(in.is_open());

        int output[3];
        bool res;
        const int fld_count = 3;

        auto convert = [](const char* a, const char* e, int& output) {
            //utxx::atof(p, e, *a_output)
            return fast_atoi(a, e, output);
        };

        res = read_values(in, output, nullptr, fld_count, convert);

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(1, output[0]);
        BOOST_CHECK_EQUAL(2, output[1]);
        BOOST_CHECK_EQUAL(3, output[2]);

        res = read_values(in, output, nullptr, fld_count, convert);

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(4, output[0]);
        BOOST_CHECK_EQUAL(5, output[1]);
        BOOST_CHECK_EQUAL(6, output[2]);

        res = read_values(in, output, nullptr, 3, convert);

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(7, output[0]);
        BOOST_CHECK_EQUAL(8, output[1]);
        BOOST_CHECK_EQUAL(9, output[2]);

        res = read_values(in, output, nullptr, fld_count, convert);

        BOOST_CHECK(!res);
    }
    {
        std::ifstream in(fn);
        BOOST_REQUIRE(in.is_open());

        int fields[] = {1, 2};
        const int fld_count = length(fields);
        int output[2];
        bool res;

        auto convert = [](const char* a, const char* e, int& output) {
            //utxx::atof(p, e, *a_output)
            return fast_atoi<int, false>(a, e, output);
        };

        res = read_values(in, output, fields, fld_count, convert);

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(1, output[0]);
        BOOST_CHECK_EQUAL(2, output[1]);

        res = read_values(in, output, fields, fld_count, convert);

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(4, output[0]);
        BOOST_CHECK_EQUAL(5, output[1]);

        res = read_values(in, output, fields, fld_count, convert);

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(7, output[0]);
        BOOST_CHECK_EQUAL(8, output[1]);

        res = read_values(in, output, fields, fld_count, convert);

        BOOST_CHECK(!res);
    }
    {
        std::ifstream in(fn);
        BOOST_REQUIRE(in.is_open());

        int fields[] = {1, 2};
        const int fld_count = length(fields);
        int output[2];
        bool res;

        auto convert = [](const char* a, const char* e, int& output) {
            return fast_atoi<int, false>(a, e, output);
        };

        res = read_values(in, output, fields, fld_count, convert);

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(1, output[0]);
        BOOST_CHECK_EQUAL(2, output[1]);

        res = read_values(in, output, fields, fld_count, convert);

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(4, output[0]);
        BOOST_CHECK_EQUAL(5, output[1]);

        res = read_values(in, output, fields, fld_count, convert);

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(7, output[0]);
        BOOST_CHECK_EQUAL(8, output[1]);

        res = read_values(in, output, fields, fld_count, convert);

        BOOST_CHECK(!res);
    }

    path::write_file(fn, "1.0 2.0 abc 10.0\n4.0 5.0 xyz 6\n7.0 8.0 xxx 9\n");

    {
        std::ifstream in(fn);
        BOOST_REQUIRE(in.is_open());

        int fields[] = {1, 2};
        const int fld_count = length(fields);
        double output[2];
        bool res;

        auto convert = [](const char* a, const char* e, double& output) {
            auto   p =  utxx::atof(a, e, output);
            return p == a ? nullptr : p;
        };

        res = read_values(in, output, fields, fld_count, convert);

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(1, output[0]);
        BOOST_CHECK_EQUAL(2, output[1]);

        res = read_values(in, output, fields, fld_count, convert);

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(4, output[0]);
        BOOST_CHECK_EQUAL(5, output[1]);

        res = read_values(in, output, fields, fld_count, convert);

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(7, output[0]);
        BOOST_CHECK_EQUAL(8, output[1]);

        res = read_values(in, output, fields, fld_count, convert);

        BOOST_CHECK(!res);
    }

    path::write_file(fn, "1.0 | 2.0 | 3.0\n4.0|5.0 | 6.0\n");
    {
        std::ifstream in(fn);
        BOOST_REQUIRE(in.is_open());

        int fields[] = {1, 2};
        const int fld_count = length(fields);
        double output[2];
        bool res;

        auto convert = [](const char* a, const char* e, double& output) {
            auto   p =  utxx::atof(a, e, output);
            return p == a ? nullptr : p;
        };

        res = read_values(in, output, fields, fld_count, convert, " |");

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(1, output[0]);
        BOOST_CHECK_EQUAL(2, output[1]);

        res = read_values(in, output, fields, fld_count, convert, " |");

        BOOST_CHECK(res);
        BOOST_CHECK_EQUAL(4, output[0]);
        BOOST_CHECK_EQUAL(5, output[1]);

        res = read_values(in, output, fields, fld_count, convert, " |");

        BOOST_CHECK(!res);
    }

    path::file_unlink(fn);
}