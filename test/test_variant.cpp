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
#include <utxx/variant.hpp>
#include <utxx/config_tree.hpp>
#include <utxx/verbosity.hpp>
#include <utxx/variant_tree_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <iostream>

using namespace utxx;

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

    { variant v;            BOOST_REQUIRE(!v.is_type<bool>()); }
    { variant v;            BOOST_REQUIRE(!v.is_type<int>()); }
    { variant v;            BOOST_REQUIRE(!v.is_type<long>()); }
    { variant v;            BOOST_REQUIRE(!v.is_type<double>()); }
    { variant v;            BOOST_REQUIRE(!v.is_type<std::string>()); }

    { variant v(true);      BOOST_REQUIRE( v.is_type<bool>()); }
    { variant v(true);      BOOST_REQUIRE(!v.is_type<int>()); }
    { variant v(true);      BOOST_REQUIRE(!v.is_type<long>()); }
    { variant v(true);      BOOST_REQUIRE(!v.is_type<double>()); }
    { variant v(true);      BOOST_REQUIRE(!v.is_type<std::string>()); }

    { variant v(1);         BOOST_REQUIRE(!v.is_type<bool>()); }
    { variant v(1);         BOOST_REQUIRE( v.is_type<int>()); }
    { variant v(1);         BOOST_REQUIRE( v.is_type<long>()); }
    { variant v(1);         BOOST_REQUIRE(!v.is_type<double>()); }
    { variant v(1);         BOOST_REQUIRE(!v.is_type<std::string>()); }

    { variant v(1.0);       BOOST_REQUIRE(!v.is_type<bool>()); }
    { variant v(1.0);       BOOST_REQUIRE(!v.is_type<int>()); }
    { variant v(1.0);       BOOST_REQUIRE(!v.is_type<long>()); }
    { variant v(1.0);       BOOST_REQUIRE( v.is_type<double>()); }
    { variant v(1.0);       BOOST_REQUIRE(!v.is_type<std::string>()); }

    { variant v("a");       BOOST_REQUIRE(!v.is_type<bool>()); }
    { variant v("a");       BOOST_REQUIRE(!v.is_type<int>()); }
    { variant v("a");       BOOST_REQUIRE(!v.is_type<long>()); }
    { variant v("a");       BOOST_REQUIRE(!v.is_type<double>()); }
    { variant v("a");       BOOST_REQUIRE( v.is_type<std::string>()); }


    { variant v(1.0);       BOOST_REQUIRE_EQUAL(variant::TYPE_DOUBLE, v.type()); }
    { variant v("test");    BOOST_REQUIRE_EQUAL(variant::TYPE_STRING, v.type()); }
    { std::string s("test");
      variant v(s);         BOOST_REQUIRE_EQUAL(variant::TYPE_STRING, v.type()); }

    { variant v(true);      BOOST_REQUIRE_EQUAL(variant::TYPE_BOOL,   v.type()); }
    { variant v(1.0);       BOOST_REQUIRE_EQUAL(variant::TYPE_DOUBLE, v.type()); }
    { variant v("test");    BOOST_REQUIRE_EQUAL(variant::TYPE_STRING, v.type()); }
    { std::string s("test");
      variant v(s);         BOOST_REQUIRE_EQUAL(variant::TYPE_STRING, v.type()); }

    {   variant v = variant(true);
        BOOST_REQUIRE_EQUAL(variant::TYPE_BOOL, v.type());
        BOOST_REQUIRE(v.to_bool());
    } {
        variant v = variant(false);
        BOOST_REQUIRE_EQUAL(variant::TYPE_BOOL, v.type());
        BOOST_REQUIRE(!v.to_bool());
    }

    {
        variant v  = variant(1.0);
        BOOST_REQUIRE_EQUAL(1.0, v.to_float());
        variant&& v1 = variant(1.0);
        BOOST_REQUIRE_EQUAL(1.0, v1.to_float());
    }

    {   variant v = variant(1.0);
        BOOST_REQUIRE_EQUAL(variant::TYPE_DOUBLE, v.type());
        BOOST_REQUIRE_EQUAL(1.0, v.to_float()); }

    {   variant v = variant("test");
        BOOST_REQUIRE_EQUAL(variant::TYPE_STRING, v.type());
        BOOST_REQUIRE_EQUAL("test", v.to_str());
        v = variant(std::string("xyz"));
        BOOST_REQUIRE_EQUAL(variant::TYPE_STRING, v.type());
        BOOST_REQUIRE_EQUAL("xyz", v.to_str());
    }

    {
        auto v = variant(true);
        BOOST_CHECK_EQUAL(true,   v.to<bool>());
        BOOST_CHECK_EQUAL(1.0,    v.to<double>());
        BOOST_CHECK_EQUAL(1,      v.to<int>());
        BOOST_CHECK_EQUAL(1L,     v.to<int64_t>());
        BOOST_CHECK_EQUAL(1ul,    v.to<uint64_t>());
        BOOST_CHECK_EQUAL("true", v.to<std::string>());
    }

    {
        auto v = variant(123);
        BOOST_CHECK_EQUAL(true,   v.to<bool>());
        BOOST_CHECK_EQUAL(123.0,  v.to<double>());
        BOOST_CHECK_EQUAL(123,    v.to<int>());
        BOOST_CHECK_EQUAL(123L,   v.to<int64_t>());
        BOOST_CHECK_EQUAL(123ul,  v.to<uint64_t>());
        BOOST_CHECK_EQUAL("123",  v.to<std::string>());
    }

    {
        auto v = variant(456.789);
        BOOST_CHECK_EQUAL(true,   v.to<bool>());
        BOOST_CHECK_EQUAL(456.789,v.to<double>());
        BOOST_CHECK_EQUAL(456,    v.to<int>());
        BOOST_CHECK_EQUAL(456L,   v.to<int64_t>());
        BOOST_CHECK_EQUAL(456ul,  v.to<uint64_t>());
        BOOST_CHECK_EQUAL("456.789000", v.to<std::string>());
    }

    {
        auto v = variant("1234");
        BOOST_CHECK_EQUAL(true,   v.to<bool>());
        BOOST_CHECK_EQUAL(1234.0, v.to<double>());
        BOOST_CHECK_EQUAL(1234,   v.to<int>());
        BOOST_CHECK_EQUAL(1234L,  v.to<int64_t>());
        BOOST_CHECK_EQUAL(1234ul, v.to<uint64_t>());
        BOOST_CHECK_EQUAL("1234", v.to<std::string>());
    }
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
        "    octal     0660\n"
        "    hex       0xFA16\n"
        "    address \"229.1.0.1:2000 Line1\"\n"
        "    address \"229.1.0.2:2001 Line2\"\n"
        "}";
    std::stringstream s; s << s_data;
    variant_tree tree;
    detail::read_scon(s, tree);
    if (verbosity::level() > VERBOSE_NONE)
        tree.dump(std::cout);
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
        int n = tree.get<int>("test.octal");
        BOOST_REQUIRE_EQUAL(0660, n);
    }
    {
        int n = tree.get<int>("test.hex");
        BOOST_REQUIRE_EQUAL(0xFA16, n);
    }
    {
        const variant_tree_base& vt = tree.get_child("test");
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
    // Sample data 2
    const char *data2 =
        "key1 1\n"
        "key2 true\n"
        "key3 10.0\n"
        "key4 test\n"
        "key4 \"str\"\n"
        "key5 1K\n"
        "key6 1M\n"
        "key7 1G\n";

    {
        variant_tree tree;
        std::stringstream s; s << data2;
        boost::property_tree::ptree pt;
        //boost::property_tree::info_parser::read_info_internal(
        //    s, tree, std::string(), 0);
        //boost::property_tree::ptree pt;
        detail::read_scon(s, tree);
        BOOST_REQUIRE_EQUAL(1, tree.get<int>("key1"));
        BOOST_REQUIRE_EQUAL(true, tree.get<bool>("key2"));
        BOOST_REQUIRE_EQUAL(10.0, tree.get<double>("key3"));
        BOOST_REQUIRE_EQUAL(2u, tree.count("key4"));
        BOOST_REQUIRE_EQUAL(1024,       tree.get<int>("key5"));
        BOOST_REQUIRE_EQUAL(1048576,    tree.get<int>("key6"));
        BOOST_REQUIRE_EQUAL(1073741824, tree.get<int>("key7"));

        //tree.swap(pt);
        //boost::property_tree::info_parser::read_info_intenal<variant_tree, char>(
        //    s.str(), tree, std::string(), 0);
    }
}

namespace {
    static const variant& merge(const variant_tree::path_type& s, const variant& d) {
        return d;
    }
    static void update(const variant_tree::path_type& s, variant& d) {
        if (d.is_null()) return;
        else if (d.is_string()) d = d.to_str() + "x";
        else d = d.to_int()+1;
    }
}

BOOST_AUTO_TEST_CASE( test_variant_tree_merge )
{
    variant_tree tree, tree2;
    tree.put("first.n",  1);
    tree.put("second.n", 2);
    tree.put("third",    3);

    tree2.put("third", variant("abc"));
    tree2.put("fourth.b", 12);
    tree2.put("first.n",  10);

    {
        tree.merge(tree2, &merge);
        std::stringstream out;

        tree.dump(out, 2, true, false);
        const char expect[] =
            "first::null()\n"
            "  n::int() = 10\n"
            "second::null()\n"
            "  n::int() = 2\n"
            "third::string() = \"abc\"\n"
            "fourth::null()\n"
            "  b::int() = 12\n";
        BOOST_REQUIRE_EQUAL(expect, out.str());
    }
    {
        tree2.update(&update);
        std::stringstream out;
        tree2.dump(out, 2, true, false);
        const char expect[] =
            "third::string() = \"abcx\"\n"
            "fourth::null()\n"
            "  b::int() = 13\n"
            "first::null()\n"
            "  n::int() = 11\n";
        BOOST_REQUIRE_EQUAL(expect, out.str());
    }
}

BOOST_AUTO_TEST_CASE( test_variant_tree_path )
{
    const char s_path[] = "one.two.three";
    tree_path a(s_path);
    BOOST_REQUIRE(!a.single());
    BOOST_REQUIRE_EQUAL("one", a.reduce());
    BOOST_REQUIRE_EQUAL(s_path, a.dump());
    BOOST_REQUIRE_EQUAL(std::string(s_path) + ".four", (a / "four").dump());

    {
        variant_tree tree;
        tree.put("one.xxxx", 1);
        BOOST_CHECK_THROW(tree.get<bool>("one.xxxx"), std::runtime_error);
    }
    {
        config_path s1;
        config_path s2 = s1 / "a.b.c";
        BOOST_REQUIRE_EQUAL("a.b.c", s2.dump());
        std::string k = s2.reduce();
        BOOST_REQUIRE_EQUAL("a", k);
        BOOST_REQUIRE_EQUAL("a.b.c", s2.dump());
        s2 /= "d.e";
        BOOST_REQUIRE_EQUAL("a.b.c.d.e", s2.dump());
        config_path s3 = make_tree_path_pair("a", "b");
        BOOST_REQUIRE_EQUAL("a[b]", s3.dump());
    }

    {
        config_path p("/a/b/c", '/');
        std::string k = p.reduce();
        BOOST_REQUIRE(k.empty());
        BOOST_REQUIRE_EQUAL("/a/b/c", p.dump());
        k = p.reduce();
        BOOST_REQUIRE_EQUAL("/a/b/c", p.dump());
        BOOST_REQUIRE_EQUAL("a", k);
    }

    try {
        throw config_error(config_path(s_path));
    } catch (config_error& e) {
        BOOST_REQUIRE_EQUAL(s_path, e.path());
    }

    {
        config_path s(s_path);
        s = s / std::make_pair("four", "ABC");
        std::string exp = std::string(s_path) + ".four[ABC]";
        std::string res = s.dump();
        BOOST_REQUIRE_EQUAL(exp, res);
    }

    {
        std::stringstream s; s <<
            "k1 a001\n"
            "k1 a002 {\n"
            "  k2 a011 {\n"
            "    k3 a111\n"
            "    k4 a3110\n"
            "    k4 a3111\n"
            "    k5 true\n"
            "    k6 1.23\n"
            "    k7 10\n"
            "  }\n"
            "}\n";
        variant_tree tree;
        detail::read_scon(s, tree);

        boost::optional<variant_tree_base&> r = tree.get_child_optional("k1[a001]");
        BOOST_REQUIRE(r);
        BOOST_REQUIRE(r->empty());

        r = tree.get_child_optional(tree_path("k1[a002]/k2[a011]", '/'));
        BOOST_REQUIRE(r);
        BOOST_REQUIRE(!r->empty());
        BOOST_REQUIRE(r->find("k3") != r->not_found());

        r = tree.get_child_optional(tree_path("[a002]/k2[a011]", '/'));
        BOOST_REQUIRE(r);
        BOOST_REQUIRE(!r->empty());
        BOOST_REQUIRE(r->find("k3") != r->not_found());

        r = tree.get_child_optional(tree_path("k1[a002]/k2/k4[a3111]", '/'));
        BOOST_REQUIRE(r);
        BOOST_REQUIRE(r->empty());

        BOOST_REQUIRE_EQUAL("a3110", tree.get<std::string>(tree_path("k1[a002]/k2/k4[a3110]", '/')));
        BOOST_REQUIRE_EQUAL(true, tree.get<bool>(tree_path("k1[a002]/k2/k5", '/')));
        BOOST_REQUIRE_EQUAL(1.23, tree.get<double>(tree_path("k1[a002]/k2[a011]/k6", '/')));
        BOOST_REQUIRE_EQUAL(10,   tree.get<int>(tree_path("k1[a002]/k2/k7", '/')));

        variant_tree cfg(tree, "k1[a002]");

        BOOST_REQUIRE_EQUAL("k1[a002]", cfg.root_path().dump());
        BOOST_REQUIRE_EQUAL("a011",     cfg.get<std::string>("k2"));
    }
}

template <class Stream, class ReadFun>
void gen_test_case(Stream& stream, ReadFun read_fun, const char* a_test_name)
{
    variant_tree tree;
    read_fun(stream, tree, 0);

    BOOST_REQUIRE_EQUAL("debug", tree.get<std::string>("one.verbose"));
    BOOST_REQUIRE_EQUAL("test1", tree.get<std::string>("one.test"));
    BOOST_REQUIRE_EQUAL(5,       tree.get<int>("one.interval"));
    BOOST_REQUIRE_EQUAL(2.012,   tree.get<double>("one.threshold"));
    BOOST_REQUIRE_EQUAL(true,    tree.get<bool>("one.overwrite"));
    BOOST_REQUIRE_EQUAL("29xx",  tree.get<std::string>("one.address1"));
    BOOST_REQUIRE_EQUAL("29 xx", tree.get<std::string>("one.address2"));
    BOOST_REQUIRE_THROW(tree.get<std::string>("one.interval"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE( test_variant_tree_xml )
{
    std::stringstream s; s <<
        "<one>\n"
        "<verbose>debug</verbose>\n"
        "<test>test1</test>\n"
        "<interval>5</interval>\n"
        "<threshold>2.012</threshold>\n"
        "<overwrite>true</overwrite>\n"
        "<address1>29xx</address1>\n"
        "<address2>29 xx</address2>\n"
        "</one>\n";
    gen_test_case(s, &detail::read_xml<std::stringstream, char>, "test_variant_tree_xml");
}

BOOST_AUTO_TEST_CASE( test_variant_tree_ini )
{
    std::stringstream s; s <<
        "[one]\n"
        "verbose    = debug\n"
        "test       = test1\n"
        "interval   = 5\n"
        "threshold  = 2.012\n"
        "overwrite  = true\n"
        "address1   = 29xx\n"
        "address2   = 29 xx\n"
        "\n";
    gen_test_case(s, &detail::read_ini<std::stringstream, char>, "test_variant_tree_ini");
}
