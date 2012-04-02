//----------------------------------------------------------------------------
/// \file  test_variant.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the variant.hpp and test_variant.hpp.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-07-10
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
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/integral_c.hpp>
#include <util/variant.hpp>
#include <util/variant_tree.hpp>
#include <util/verbosity.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/detail/info_parser_read.hpp>
#include <boost/property_tree/detail/info_parser_write.hpp>
#include <iostream>

using namespace util;

namespace {
    typedef boost::mpl::vector<
        int,int16_t,long,uint16_t,uint32_t,uint64_t> int_types;
    typedef boost::mpl::vector<
        bool,int,int16_t,long,uint16_t,uint32_t,uint64_t,double> valid_types;

    struct int_require {
        template <typename T>
        void operator() (T t) {
            typedef typename boost::mpl::find<int_types, T>::type iter;
            typedef typename boost::mpl::if_<
                boost::is_same<boost::mpl::end<int_types>::type, iter>,
                long,
                T
            >::type type;

            t = 1;
            T expect = static_cast<T>(1);
            variant v(t);
            T r = v.get<T>();
            BOOST_REQUIRE_EQUAL(expect, r);
        }
    };

    struct int_require_type {
        template <typename T>
        void operator() (T t) {
            variant v((T)1);
            BOOST_REQUIRE_EQUAL(variant::TYPE_INT, v.type());
        }
    };
}

BOOST_AUTO_TEST_CASE( test_variant )
{
    {
        variant v(true);
        BOOST_REQUIRE_EQUAL(variant::TYPE_BOOL, v.type());
        bool r = boost::get<bool>(v);
        r = v.get<bool>();
        //bool r = (bool)v;
        BOOST_REQUIRE_EQUAL(true, r);
    }

    {
        variant v(false);
        BOOST_REQUIRE_EQUAL(variant::TYPE_BOOL, v.type());
        bool r = v.get<bool>();
        BOOST_REQUIRE_EQUAL(false, r);
    }

    // Test all numeric types
    {
        int_require test;

        boost::mpl::for_each<int_types>(test);
        boost::mpl::for_each<valid_types>(test);

    }

    { variant v(true);      BOOST_REQUIRE_EQUAL(variant::TYPE_BOOL,   v.type()); }
    { variant v(1.0);       BOOST_REQUIRE_EQUAL(variant::TYPE_DOUBLE, v.type()); }
    { variant v("test");    BOOST_REQUIRE_EQUAL(variant::TYPE_STRING, v.type()); }
    { std::string s("test");
      variant v(s);         BOOST_REQUIRE_EQUAL(variant::TYPE_STRING, v.type()); }
}

BOOST_AUTO_TEST_CASE( test_variant_tree )
{
    variant_tree pt;

    {
        // Put/get int value
        pt.put("int value", 3);
        int int_value = pt.get<int>("int value");
        BOOST_REQUIRE_EQUAL(3, int_value);
    }

    {
        // Put/get int value
        pt.put("long value", 10l);
        long v = pt.get<long>("long value");
        BOOST_REQUIRE_EQUAL(10l, v);
    }

    {
        // Put/get string value
        pt.put<std::string>("string value", "foo bar");
        std::string string_value = pt.get<std::string>("string value");
        BOOST_REQUIRE_EQUAL("foo bar", string_value);
    }

    {
        // Put/get int value
        pt.put("bool value", true);
        bool b = pt.get<bool>("bool value");
        BOOST_REQUIRE_EQUAL(true, b);
    }
}

BOOST_AUTO_TEST_CASE( test_variant_tree_file )
{
    static const char s_data[] =
        "test\n"
        "{\n"
        "    verbose debug\n"
        "    test \"test1\"\n"
        "    report_interval 5\n"
        "    threshold 2.012\n"
        "    overwrite true\n"
        "    address \"229.1.0.1:2000 Line1\"\n"
        "    address \"229.1.0.2:2001 Line2\"\n"
        "}";
    std::stringstream s; s << s_data;
    variant_tree tree;
    boost::property_tree::info_parser::read_info(s, tree);
    if (verbosity::level() > VERBOSE_NONE)
        boost::property_tree::info_parser::write_info(std::cout, tree);
    {
        std::string str = tree.get<std::string>("test.verbose");
        BOOST_REQUIRE_EQUAL("debug", str);
    }
    {
        std::string str = tree.get<std::string>("test.test");
        BOOST_REQUIRE_EQUAL("test1", str);
    }
    {
        int n = tree.get<int>("test.report_interval");
        BOOST_REQUIRE_EQUAL(5, n);
    }
    {
        int n = tree.get("test.report", 10);
        BOOST_REQUIRE_EQUAL(10, n);
    }
    {
        bool n = tree.get<bool>("test.overwrite");
        BOOST_REQUIRE_EQUAL(true, n);
    }
    {
        bool n = tree.get("test.overwrite_it", true);
        BOOST_REQUIRE_EQUAL(true, n);
    }
    {
        double n = tree.get<double>("test.threshold");
        BOOST_REQUIRE_EQUAL(2.012, n);
    }
    {
        double n = tree.get("test.threshold_it", 4.5);
        BOOST_REQUIRE_EQUAL(4.5, n);
    }
    {
        variant_tree& vt = tree.get_child("test");
        int n = vt.count("address");
        BOOST_REQUIRE_EQUAL(2, n);
        std::string str = tree.get<std::string>("test.address");
        BOOST_REQUIRE_EQUAL("229.1.0.1:2000 Line1", str);
        n = tree.count("test.address");
        BOOST_REQUIRE_EQUAL(0, n);
    }
    {
        std::string n = tree.get("test.address_it", "test");
        BOOST_REQUIRE_EQUAL("test", n);
    }
}

BOOST_AUTO_TEST_CASE( test_variant_tree_parse )
{
    // Sample data 1
    const char *data1 = 
        "\n"
        "key1 3\n"
        "{\n"
        "\tkey \"test\"\n"
        "}\n"
        "key2 \"text\" {\n"
        "\tkey true\n"
        "}\n"
        "key3 \"data\"\n"
        "\t \"3\" {\n"
        "\tkey data\n"
        "}\n"
        "\n"
        "\"key4\" data4\n"
        "{\n"
        "\tkey data\n"
        "}\n"
        "\"key.5\" \"data.5\" { \n"
        "\tkey data \n"
        "}\n"
        "\"key6\" \"data\"\n"
        "\t   \"6\" {\n"
        "\tkey data\n"
        "}\n"
        "   \n"
        "key1 data1\n"
        "{\n"
        "\tkey data\n"
        "}\n"
        "key2 \"data2  \" {\n"
        "\tkey data\n"
        "}\n"
        "key3 \"data\"\n"
        "\t \"3\" {\n"
        "\tkey data\n"
        "}\n"
        "\n"
        "\"key4\" data4\n"
        "{\n"
        "\tkey data\n"
        "}\n"
        "\"key.5\" \"data.5\" {\n"
        "\tkey data\n"
        "}\n"
        "\"key6\" \"data\"\n"
        "\t   \"6\" {\n"
        "\tkey data\n"
        "}\n"
        "\\\\key\\t7 data7\\n\\\"data7\\\"\n"
        "{\n"
        "\tkey data\n"
        "}\n"
        "\"\\\\key\\t8\" \"data8\\n\\\"data8\\\"\"\n"
        "{\n"
        "\tkey data\n"
        "}\n"
        "\n";

    // Sample data 2
    const char *data2 = 
        "key1 1\n"
        "key2 true\n"
        "key3 10.0\n"
        "key4 test\n"
        "key4 \"str\"\n";

    {
        variant_tree tree;
        std::stringstream s; s << data1;
        //boost::property_tree::info_parser::read_info_intenal<variant_tree, char>(s.str(), tree, std::string(), 0);
    }
    {
        variant_tree tree;
        std::stringstream s; s << data2;
        boost::property_tree::ptree pt;
        boost::property_tree::info_parser::read_info_internal(
            s, tree, std::string(), 0);
        //boost::property_tree::ptree pt;
        //boost::property_tree::info_parser::read_info(s.str(), pt);
        //tree.swap(pt);
        //boost::property_tree::info_parser::read_info_intenal<variant_tree, char>(s.str(), tree, std::string(), 0);
    }
}
