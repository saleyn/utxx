//----------------------------------------------------------------------------
/// \file  test_config_validator.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for config_validator.
//----------------------------------------------------------------------------
// Copyright (c) 2012 Serge Aleynikov <saleyn@gmail.com>
// Created: 2012-01-14
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

Copyright (C) 2012 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/config_validator.hpp>
#include <utxx/variant_tree_parser.hpp>
// The file below is auto-generated by running
// config_validator.xsl on test_config_validator.xml
#include "generated/test_config_validator.generated.hpp"

#ifndef NO_TEST_FRAMEWORK
#include <boost/test/unit_test.hpp>
#else
#include <boost/assert.hpp>
#define BOOST_AUTO_TEST_CASE(f) void f()
#define BOOST_REQUIRE(x) BOOST_ASSERT((x))
void BOOST_REQUIRE_EQUAL(const char* x, const char* y) {
    if (strcmp(x, y)) {
        std::cerr << "Left:  '" << (x) << "'\nRight: '" << (y) << "'" << std::endl;
        BOOST_ASSERT(false);
    }
}
void BOOST_REQUIRE_EQUAL(const char* x, const std::string& y) {
    if (y != x) {
        std::cerr << "Left:  '" << (x) << "'\nRight: '" << (y) << "'" << std::endl;
        BOOST_ASSERT(false);
    }
}

void test_config_validator1();
void test_config_validator2();
void test_config_validator3();
void test_config_validator4();
void test_config_validator5();
void test_config_validator6();
void test_config_validator7();
void test_config_validator8();

int main() {
    test_config_validator1();
    test_config_validator2();
    test_config_validator3();
    test_config_validator4();
    test_config_validator5();
    test_config_validator6();
    test_config_validator7();
    test_config_validator8();
    return 0;
}

#endif


using namespace utxx;

BOOST_AUTO_TEST_CASE( test_config_validator0 )
{
    const test::cfg_validator& l_validator = test::cfg_validator::instance();
    std::stringstream s;
    s << l_validator.usage("");
    BOOST_REQUIRE_EQUAL(
        "address: string\n"
        "  Description: Sample string entry\n"
        "      Default: \"123.124.125.012\"\n"
        "\n"
        "cost: float\n"
        "  Description: Sample float entry\n"
        "      Default: 1.5\n"
        "          Min: 0.0\n"
        "country: string\n"
        "  Description: Sample choice required entry\n"
        "       Unique: true\n"
        "     Required: true\n"
        "\n"
        "  connection (anonymous): string\n"
        "    Description: Server connection\n"
        "        Default: \"\"\n"
        "\n"
        "      address: string\n"
        "        Description: Server address\n"
        "           Required: true\n"
        "\n"
        "duration: int\n"
        "  Description: Sample required int entry\n"
        "     Required: true\n"
        "          Min: 10 Max: 60\n"
        "enabled: bool\n"
        "  Description: Sample bool entry\n"
        "      Default: true\n"
        "\n"
        "section: string\n"
        "     Required: true\n"
        "\n"
        "  location: int\n"
        "       Required: true\n"
        "\n"
        "section2: string\n"
        "\n"
        "  abc: string\n"
        "        Default: \"x\"\n"
        "\n"
        "tmp_str: string\n"
        "      Default: \"$TMP\"\n"
        "\n",
        s.str());
}

BOOST_AUTO_TEST_CASE( test_config_validator1 )
{
    variant_tree l_config;
    std::stringstream l_stream;
    l_stream
        << "address \"yahoo\"\n"
        << "enabled false\n"
        << "duration 20\n"
        << "cost     2.0\n"
        << "country \"US\"\n"
        << "  {\n"
        << "    ARCA\n"
        << "    { address \"1.2.3.4\" }\n"
        << "  }\n"
        << "section {\n"
        << "  location 10\n"
        << "}\n"
        << "country \"CA\"\n"
        << "  {\n"
        << "    ARCA exchange\n" /* This is an example of an anonymous
                                    node with value 'exchange' */
        << "    { address \"1.2.3.4\" }\n"
        << "    NSDQ\n"          /* Another anonymous node */
        << "    { address \"2.3.4.5\" }\n"
        << "  }\n";

    read_info(l_stream, l_config);

    try {
        const test::cfg_validator& l_validator = test::cfg_validator::instance();
        l_validator.validate(l_config, true);
        BOOST_REQUIRE(true); // Just to avoid a warning that there are no tests
    } catch (config_error& e) {
        std::cerr << e.str() << std::endl;
        BOOST_REQUIRE(false);
    }
}

BOOST_AUTO_TEST_CASE( test_config_validator2 )
{
    variant_tree l_config;
    std::stringstream l_stream;
    l_stream << "address \"yahoo\"\n";

    read_info(l_stream, l_config);

    const test::cfg_validator& l_validator = test::cfg_validator::instance();
    try {
        l_validator.validate(l_config, true);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("country", e.path());
        BOOST_REQUIRE_EQUAL(
            "Config error [country]: Missing a required child option connection.address",
            e.what());
    }
}

BOOST_AUTO_TEST_CASE( test_config_validator3 )
{
    variant_tree l_config;
    std::stringstream l_stream;
    l_stream << "duration 10\n";

    read_info(l_stream, l_config);

    const test::cfg_validator& l_validator = test::cfg_validator::instance();
    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("country", e.path());
        BOOST_REQUIRE_EQUAL(
            "Config error [country]: Missing a required child option connection.address",
            e.what());
    }

    l_config.clear();
    l_stream.clear();
    l_stream << "country US { ARCA connection { address abc } }\nduration 10\n"
             << "section { location 10 }\n";

    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
    } catch (utxx::config_error& e) {
        std::cerr << e.str() << std::endl;
        BOOST_REQUIRE(false);
    }

    l_config.clear();
    l_stream.clear();
    l_stream << "country US { }\nduration 10\n";

    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(true); // TODO: throw error because country.US.connection.address is undefined
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("country[US]", e.path());
        BOOST_REQUIRE_EQUAL(
            "Config error [country[US]]: Option is missing required child option connection.address",
            e.what());
    }
}

BOOST_AUTO_TEST_CASE( test_config_validator4 )
{
    variant_tree l_config;
    std::stringstream l_stream;
    l_stream << "country US { ARCA connection { address abc } }\nduration 5\n"
             << "section { location 10 }\n";

    read_info(l_stream, l_config);

    const test::cfg_validator& l_validator = test::cfg_validator::instance();
    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("duration[5]", e.path());
        BOOST_REQUIRE_EQUAL("Config error [duration[5]]: Value too small!", e.what());
    }
}

BOOST_AUTO_TEST_CASE( test_config_validator5 )
{
    variant_tree l_config;
    std::stringstream l_stream;
    l_stream << "country US { ARCA connection { address abc } }\nduration 61\n"
             << "section { location 10 }\n";

    read_info(l_stream, l_config);

    const test::cfg_validator& l_validator = test::cfg_validator::instance();
    try {
        l_validator.validate(l_config, true, "root");
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("root.duration[61]", e.path());
        BOOST_REQUIRE_EQUAL("Config error [root.duration[61]]: Value too large!", e.what());
    }
}

BOOST_AUTO_TEST_CASE( test_config_validator6 )
{
    variant_tree l_config;
    std::stringstream l_stream;
    l_stream << "duration 10\n"
             << "country \"ER\" { ARCA connection { address abc } }\n"
             << "section { location 10 }\n";

    read_info(l_stream, l_config);

    const test::cfg_validator& l_validator = test::cfg_validator::instance();
    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("country[ER]", e.path());
        BOOST_REQUIRE_EQUAL(
            "Config error [country[ER]]: Value is not allowed for option!",
            e.what());
    }
}

BOOST_AUTO_TEST_CASE( test_config_validator7 )
{
    variant_tree l_config;
    std::stringstream l_stream;
    const test::cfg_validator& l_validator = test::cfg_validator::instance();

    l_stream << "duration 10\n"
             << "country \"US\"\n"
             << "section { location 10 }\n";
    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("country[US]", e.path());
        BOOST_REQUIRE_EQUAL(
            "Config error [country[US]]: Option is missing required child option connection.address",
            e.what());
    }

    l_config.clear();
    l_stream.clear();
    l_stream << "duration 10\n"
             << "country \"US\"\n"
             << "{\"ARCA\" example }\n"
             << "section { location 10 }\n";
    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("country[US].connection.ARCA[example].address", e.path());
        BOOST_REQUIRE_EQUAL("Config error [country[US].connection.ARCA[example].address]: "
            "Missing required option with no default!", e.what());
    }

    l_config.clear();
    l_stream.clear();
    l_stream << "duration 10\n"
             << "country \"ER\" { ARCA connection { address abc } }\n"
             << "abc test\n"
             << "section { location 10 }\n";
    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("country[ER]", e.path());
        BOOST_REQUIRE_EQUAL("Config error [country[ER]]: Value is not allowed for option!", e.what());
    }

    l_config.clear();
    l_stream.clear();
    l_stream << "duration 10\n"
             << "country \"US\" { ARCA connection { address abc } }\n"
             << "abc test\n"
             << "section { location 10 }\n";
    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("abc", e.path());
        BOOST_REQUIRE_EQUAL("Config error [abc]: Unsupported config option!", e.what());
    }

    l_config.clear();
    l_stream.clear();
    l_stream << "duration 10\n country \"US\"\n { \"\" example }\n"
             << "section { location 10 }\n";
    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("country[US].connection[example].address", e.path());
        BOOST_REQUIRE_EQUAL("Config error [country[US].connection[example].address]: "
            "Missing required option with no default!", e.what());
    }

    l_config.clear();
    l_stream.clear();
    l_stream << "address abc\n address bcd\n"
             << "section { location 10 }\n";
    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("address[bcd]", e.path());
        BOOST_REQUIRE_EQUAL("Config error [address[bcd]]: "
            "Non-unique config option found!", e.what());
    }
}

BOOST_AUTO_TEST_CASE( test_config_validator8 )
{
    variant_tree l_config;
    std::stringstream l_stream;
    const test::cfg_validator& l_validator = test::cfg_validator::instance();

    l_stream << "address 10\nduration 15\n"
             << "country \"US\" { ARCA connection { address abc } }\n"
             << "section { location 10 }\n";
    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("address[10]", e.path());
        BOOST_REQUIRE_EQUAL("Config error [address[10]]: "
            "Wrong type - expected string!", e.what());
    }

    l_config.clear();
    l_stream.clear();
    l_stream << "duration abc\n"
             << "country \"US\" { ARCA connection { address abc } }\n"
             << "section { location 10 }\n";
    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("duration[abc]", e.path());
        BOOST_REQUIRE_EQUAL("Config error [duration[abc]]: Wrong type - expected integer!", e.what());
    }

    l_config.clear();
    l_stream.clear();
    l_stream << "enabled 1\n"
             << "duration 10\n"
             << "country \"US\" { ARCA connection { address abc } }\n"
             << "section { location 10 }\n";
    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("enabled[1]", e.path());
        BOOST_REQUIRE_EQUAL("Config error [enabled[1]]: "
            "Wrong type - expected boolean true/false!", e.what());
    }

    l_config.clear();
    l_stream.clear();
    l_stream << "cost 1\n"
             << "duration 10\n"
             << "country \"US\" { ARCA connection { address abc } }\n"
             << "section { location 10 }\n";
    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("cost[1]", e.path());
        BOOST_REQUIRE_EQUAL("Config error [cost[1]]: Wrong type - expected float!", e.what());
    }

    l_config.clear();
    l_stream.clear();
    l_stream << "cost 1\n"
             << "duration 10\n"
             << "country \"US\" { ARCA connection { address abc } }\n";
    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
        BOOST_REQUIRE(false);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("section", e.path());
        std::string s = e.what();
        BOOST_REQUIRE_EQUAL("Config error [section]: "
            "Missing a required child option location", s);
    }

    l_config.clear();
    l_stream.clear();
    l_stream << "cost 1\n"
             << "duration 10\n"
             << "country \"US\" { ARCA connection { address abc } }\n"
             << "section { location 10 }\n";
    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
    } catch (utxx::config_error& e) {
        BOOST_REQUIRE_EQUAL("cost[1]", e.path());
        BOOST_REQUIRE_EQUAL("Config error [cost[1]]: Wrong type - expected float!", e.what());
    }

    l_config.clear();
    l_stream.clear();
    l_stream << "cost 1.0\n"
             << "duration 10\n"
             << "country \"US\" { ARCA connection { address abc } }\n"
             << "section { location 10 }\n";
    read_info(l_stream, l_config);

    try {
        l_validator.validate(l_config);
    } catch (utxx::config_error& e) {
        std::cerr << e.str() << std::endl;
        BOOST_REQUIRE(false);
    }

}

BOOST_AUTO_TEST_CASE( test_config_validator_def )
{
    const test::cfg_validator& l_validator = test::cfg_validator::instance();
    BOOST_REQUIRE_EQUAL("test", l_validator.root().dump());
    BOOST_REQUIRE_EQUAL(variant("123.124.125.012"), l_validator.default_value("test.address"));
    BOOST_REQUIRE_EQUAL(variant(true), l_validator.default_value("test.enabled"));
    BOOST_REQUIRE_EQUAL(variant(1.5), l_validator.default_value("test.cost"));
    BOOST_REQUIRE_EQUAL(variant("x"), l_validator.default_value("test.section2.abc"));
    BOOST_REQUIRE_THROW(l_validator.default_value("a.b.c"), utxx::config_error);

    config_tree l_config;
    bool b = l_validator.get<bool>("test.enabled", l_config);
    BOOST_REQUIRE(b);

    const config::option* l_opt = l_validator.find("enabled", "test");
    BOOST_REQUIRE(l_opt);
    BOOST_REQUIRE_EQUAL("enabled", l_opt->name);
    BOOST_REQUIRE_EQUAL(true, l_opt->default_value.to_bool());
    try {
        l_validator.default_value("name", "test.country");
        BOOST_REQUIRE(false);
    } catch (config_error& e) {
        BOOST_REQUIRE_EQUAL("test.country.name", e.path());
    }
    try {
        l_validator.default_value("", "test.country.name");
    } catch (config_error& e) {
        BOOST_REQUIRE_EQUAL("test.country.name", e.path());
    }

    std::string l_tmp_str = l_validator.get<std::string>("test.tmp_str", l_config);
    BOOST_REQUIRE(l_tmp_str.find('$') == std::string::npos);
}
