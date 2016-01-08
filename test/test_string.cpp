//----------------------------------------------------------------------------
/// \file  test_string.cpp
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

#include <utxx/string.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_string_conversion )
{
    {
        std::stringstream s; s << fixed(10.123, 8, 4);
        BOOST_CHECK_EQUAL(" 10.1230", s.str());
    }
    {
        std::stringstream s; s << fixed(10.123, 8, 4, '0');
        BOOST_CHECK_EQUAL("010.1230", s.str());
    }
    {
        auto pair = split("", ",");
        BOOST_CHECK_EQUAL("", pair.first);
        BOOST_CHECK_EQUAL("", pair.second);

        pair = split("abc,efg", ",");
        BOOST_CHECK_EQUAL("abc", pair.first);
        BOOST_CHECK_EQUAL("efg", pair.second);

        pair = split("abc||efg", "||");
        BOOST_CHECK_EQUAL("abc", pair.first);
        BOOST_CHECK_EQUAL("efg", pair.second);

        pair = split("abc|efg|xyz", "|");
        BOOST_CHECK_EQUAL("abc",     pair.first);
        BOOST_CHECK_EQUAL("efg|xyz", pair.second);

        pair = split("abc", ",");
        BOOST_CHECK_EQUAL("abc", pair.first);
        BOOST_CHECK_EQUAL("",    pair.second);

        pair = split<RIGHT>("", ",");
        BOOST_CHECK_EQUAL("", pair.first);
        BOOST_CHECK_EQUAL("", pair.second);

        pair = split<RIGHT>("abc,efg,xyz", ",");
        BOOST_CHECK_EQUAL("abc,efg", pair.first);
        BOOST_CHECK_EQUAL("xyz",     pair.second);

        pair = split<RIGHT>("abc||efg", "||");
        BOOST_CHECK_EQUAL("abc", pair.first);
        BOOST_CHECK_EQUAL("efg", pair.second);

        pair = split<RIGHT>("abc", "||");
        BOOST_CHECK_EQUAL("",    pair.first);
        BOOST_CHECK_EQUAL("abc", pair.second);
    }
    {
        std::vector<std::string> v{"a","b","c"};
        auto s1 = join(v, ",");
        BOOST_CHECK_EQUAL("a,b,c", s1);
        auto s2 = join(v.begin(), v.end());
        BOOST_CHECK_EQUAL("a,b,c", s2);
        auto s3 = join(v.begin(), v.end(), std::string(":"));
        BOOST_CHECK_EQUAL("a:b:c", s3);

        BOOST_CHECK_EQUAL("",    strjoin("",  "", "/"));
        BOOST_CHECK_EQUAL("a",   strjoin("a", "", "/"));
        BOOST_CHECK_EQUAL("b",   strjoin("",  "b", "/"));
        BOOST_CHECK_EQUAL("a/b", strjoin("a", "b", "/"));
        BOOST_CHECK_EQUAL("a//b",strjoin("a", "b", "//"));
    }
}

BOOST_AUTO_TEST_CASE( test_string_length )
{
    char const* s_values[] = {"One", "Two", "Three"};
    static_assert(length(s_values) == 3, "Wrong length");

    const char s[] = "abc";
    static_assert(length(s) == 3, "Wrong length");
    BOOST_REQUIRE_EQUAL(3u, length(s));

    static const char* s_ops[] = {"a", "b", "c"};
    BOOST_REQUIRE_EQUAL(3u, length(s_ops));

    struct t { int a; t(int a) : a(a) {} };

    const t arr[] = { t(1), t(2) };
    BOOST_REQUIRE_EQUAL(2u, length(arr));

    size_t b[3];
    BOOST_REQUIRE_EQUAL(3u, length<decltype(b)>());
}

enum op_type {OP_UNDEFINED = -1, OP_ADD, OP_REMOVE, OP_REPLACE, OP_UPDATE};

BOOST_AUTO_TEST_CASE( test_string_find_pos )
{
    static const char* s_test   = "abc\n   ";
    static const char* s_end    = s_test + 7;
    static const char* s_expect = s_test + 3;
    BOOST_REQUIRE_EQUAL(s_expect, utxx::find_pos(s_test, s_end, '\n'));
    BOOST_REQUIRE_EQUAL(s_end,    utxx::find_pos(s_test, s_end, 'X'));
    BOOST_REQUIRE_EQUAL(s_end,    utxx::find_pos(s_test, s_end, '\0'));
}

BOOST_AUTO_TEST_CASE( test_string_find_index )
{
    static const char* s_ops[] = {"add", "remove", "replace", "update"};
    BOOST_CHECK(OP_REMOVE    == utxx::find_index<op_type>(s_ops, "remove  ", 6));
    BOOST_CHECK(OP_UNDEFINED == utxx::find_index<op_type>(s_ops, "remove  ", 0));
    BOOST_CHECK(OP_REPLACE   == utxx::find_index<op_type>(s_ops, sizeof(s_ops), "replace"));
    BOOST_CHECK(OP_REPLACE   == utxx::find_index<op_type>(s_ops, sizeof(s_ops), "replace ", 7));
    BOOST_CHECK(OP_UNDEFINED == utxx::find_index<op_type>(s_ops, "xxx"));
    BOOST_CHECK(OP_ADD       == utxx::find_index<op_type>(s_ops, "Add",    OP_UNDEFINED, true));
    BOOST_CHECK(OP_ADD       == utxx::find_index<op_type>(s_ops, "ADD", 3, OP_UNDEFINED, true));
    BOOST_CHECK(OP_ADD       == utxx::find_index_or_throw<op_type>(s_ops, "ADD", OP_UNDEFINED, true));
}

BOOST_AUTO_TEST_CASE( test_string_to_int64 )
{
    BOOST_REQUIRE_EQUAL(1u,       to_int64("\1"));
    BOOST_REQUIRE_EQUAL(258u,     to_int64("\1\2"));
    BOOST_REQUIRE_EQUAL(66051u,   to_int64("\1\2\3"));
    BOOST_REQUIRE_EQUAL(4276803u, to_int64("ABC"));

    char buf[4];
    auto p = from_int64(4276803u, buf);
    BOOST_CHECK_EQUAL(buf, "ABC");
    BOOST_CHECK(p ==  buf+3);
}

#define TEST_WILDCARD(A, B, C) \
        BOOST_CHECK(wildcard_match((A),(B)) == C)

BOOST_AUTO_TEST_CASE( test_string_wildcard )
{
    BOOST_CHECK(wildcard_match("foo3h.txt", "foo?h.*"));
    BOOST_CHECK(wildcard_match("foo3h.txt", "foo*h.*"));
    BOOST_CHECK(!wildcard_match("foo3k",    "foo*h"));

    TEST_WILDCARD("abcccd", "*ccd", true);
    TEST_WILDCARD("mississipissippi", "*issip*ss*", true);
    TEST_WILDCARD("xxxx*zzzzzzzzy*f", "xxxx*zzy*fffff", false);
    TEST_WILDCARD("xxxx*zzzzzzzzy*f", "xxx*zzy*f", true);
    TEST_WILDCARD("xxxxzzzzzzzzyf", "xxxx*zzy*fffff", false);
    TEST_WILDCARD("xxxxzzzzzzzzyf", "xxxx*zzy*f", true);
    TEST_WILDCARD("xyxyxyzyxyz", "xy*z*xyz", true);
    TEST_WILDCARD("mississippi", "*sip*", true);
    TEST_WILDCARD("xyxyxyxyz", "xy*xyz", true);
    TEST_WILDCARD("mississippi", "mi*sip*", true);
    TEST_WILDCARD("ababac", "*abac*", true);
    TEST_WILDCARD("ababac", "*abac*", true);
    TEST_WILDCARD("aaazz", "a*zz*", true);
    TEST_WILDCARD("a12b12", "*12*23", false);
    TEST_WILDCARD("a12b12", "a12b", false);
    TEST_WILDCARD("a12b12", "*12*12*", true);

    // Additional cases where the '*' char appears in the tame string.
    TEST_WILDCARD("*", "*", true);
    TEST_WILDCARD("a*abab", "a*b", true);
    TEST_WILDCARD("a*r", "a*", true);
    TEST_WILDCARD("a*ar", "a*aar", false);

    // More double wildcard scenarios.
    TEST_WILDCARD("XYXYXYZYXYz", "XY*Z*XYz", true);
    TEST_WILDCARD("missisSIPpi", "*SIP*", true);
    TEST_WILDCARD("mississipPI", "*issip*PI", true);
    TEST_WILDCARD("xyxyxyxyz", "xy*xyz", true);
    TEST_WILDCARD("miSsissippi", "mi*sip*", true);
    TEST_WILDCARD("miSsissippi", "mi*Sip*", false);
    TEST_WILDCARD("abAbac", "*Abac*", true);
    TEST_WILDCARD("abAbac", "*Abac*", true);
    TEST_WILDCARD("aAazz", "a*zz*", true);
    TEST_WILDCARD("A12b12", "*12*23", false);
    TEST_WILDCARD("a12B12", "*12*12*", true);
    TEST_WILDCARD("oWn", "*oWn*", true);

    // Completely tame (no wildcards) cases.
    TEST_WILDCARD("bLah", "bLah", true);
    TEST_WILDCARD("bLah", "bLaH", false);

    // Simple mixed wildcard tests suggested by IBMer Marlin Deckert.
    TEST_WILDCARD("a", "*?", true);
    TEST_WILDCARD("ab", "*?", true);
    TEST_WILDCARD("abc", "*?", true);

    // More mixed wildcard tests including coverage for false positives.
    TEST_WILDCARD("a", "??", false);
    TEST_WILDCARD("ab", "?*?", true);
    TEST_WILDCARD("ab", "*?*?*", true);
    TEST_WILDCARD("abc", "?**?*?", true);
    TEST_WILDCARD("abc", "?**?*&?", false);
    TEST_WILDCARD("abcd", "?b*??", true);
    TEST_WILDCARD("abcd", "?a*??", false);
    TEST_WILDCARD("abcd", "?**?c?", true);
    TEST_WILDCARD("abcd", "?**?d?", false);
    TEST_WILDCARD("abcde", "?*b*?*d*?", true);

    // Single-character-match cases.
    TEST_WILDCARD("bLah", "bL?h", true);
    TEST_WILDCARD("bLaaa", "bLa?", false);
    TEST_WILDCARD("bLah", "bLa?", true);
    TEST_WILDCARD("bLaH", "?Lah", false);
    TEST_WILDCARD("bLaH", "?LaH", true);

    // Many-wildcard scenarios.
    TEST_WILDCARD("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab",
            "a*a*a*a*a*a*aa*aaa*a*a*b", true);
    TEST_WILDCARD("abababababababababababababababababababaacacacacaca\
cacadaeafagahaiajakalaaaaaaaaaaaaaaaaaffafagaagggagaaaaaaaab",
            "*a*b*ba*ca*a*aa*aaa*fa*ga*b*", true);
    TEST_WILDCARD("abababababababababababababababababababaacacacacaca\
cacadaeafagahaiajakalaaaaaaaaaaaaaaaaaffafagaagggagaaaaaaaab",
            "*a*b*ba*ca*a*x*aaa*fa*ga*b*", false);
    TEST_WILDCARD("abababababababababababababababababababaacacacacaca\
cacadaeafagahaiajakalaaaaaaaaaaaaaaaaaffafagaagggagaaaaaaaab",
            "*a*b*ba*ca*aaaa*fa*ga*gggg*b*", false);
    TEST_WILDCARD("abababababababababababababababababababaacacacacaca\
cacadaeafagahaiajakalaaaaaaaaaaaaaaaaaffafagaagggagaaaaaaaab",
            "*a*b*ba*ca*aaaa*fa*ga*ggg*b*", true);
    TEST_WILDCARD("aaabbaabbaab", "*aabbaa*a*", true);
    TEST_WILDCARD("a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*",
            "a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*", true);
    TEST_WILDCARD("aaaaaaaaaaaaaaaaa",
            "*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*", true);
    TEST_WILDCARD("aaaaaaaaaaaaaaaa",
            "*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*", false);
    TEST_WILDCARD("abc*abcd*abcde*abcdef*abcdefg*abcdefgh*abcdefghi*a\
bcdefghij*abcdefghijk*abcdefghijkl*abcdefghijklm*abcdefghijklmn",
            "abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*a\
            bc*", false);
    TEST_WILDCARD("abc*abcd*abcde*abcdef*abcdefg*abcdefgh*abcdefghi*a\
bcdefghij*abcdefghijk*abcdefghijkl*abcdefghijklm*abcdefghijklmn",
            "abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*", true);
    TEST_WILDCARD("abc*abcd*abcd*abc*abcd", "abc*abc*abc*abc*abc", false);
    TEST_WILDCARD(
            "abc*abcd*abcd*abc*abcd*abcd*abc*abcd*abc*abc*abcd",
            "abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abcd", true);
    TEST_WILDCARD("abc", "********a********b********c********", true);
    TEST_WILDCARD("********a********b********c********", "abc", false);
    TEST_WILDCARD("abc", "********a********b********b********", false);
    TEST_WILDCARD("*abc*", "***a*b*c***", true);
}

BOOST_AUTO_TEST_CASE( test_string_nocase )
{
    string_nocase s("AbcDe123");
    BOOST_REQUIRE_EQUAL(s, "abcde123");

    std::string s1("AbC");
    BOOST_REQUIRE_EQUAL("abc", to_lower(s1));
    BOOST_REQUIRE_EQUAL("ABC", to_upper(s1));
}

BOOST_AUTO_TEST_CASE( test_string_to_bin_string )
{
    {
        std::string s("abcdef01234");
        std::string b = to_bin_string(s.c_str(), s.size());
        std::string c("<<\"abcdef01234\">>");
        BOOST_REQUIRE_EQUAL(c, b);
    }
    {
        std::string s("8=FIX.4.2|9=71|35=A|34=93|49=CLIENT1|52=20120418-03:04:28.925|"
                      "56=EXECUTOR|98=0|108=10|10=151|");
        std::string b = to_bin_string(s.c_str(), s.size());
        std::string c("<<\"8=FIX.4.2|9=71|35=A|34=93|49=CLIENT1|52=20120418-03:04:28.925|"
                      "56=EXECUTOR|98=0|108=10|10=151|\">>");
        BOOST_REQUIRE_EQUAL(c, b);
        b = to_bin_string(s.c_str(), s.size(), false, false);
        std::string d("<<56,61,70,73,88,46,52,46,50,124,57,61,55,49,124,51,53,61,65,"
                    "124,51,52,61,57,51,124,52,57,61,67,76,73,69,78,84,49,124,53,50,"
                    "61,50,48,49,50,48,52,49,56,45,48,51,58,48,52,58,50,56,46,57,50,"
                    "53,124,53,54,61,69,88,69,67,85,84,79,82,124,57,56,61,48,124,49,"
                    "48,56,61,49,48,124,49,48,61,49,53,49,124>>");
        BOOST_REQUIRE_EQUAL(d, b);
    }
}



