//----------------------------------------------------------------------------
/// \file  test_string.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the string.hpp.
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
#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>
#include <map>
#include <set>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_string_conversion )
{
    {
        std::stringstream s; s << utxx::fixed(10.123, 8, 4);
        BOOST_CHECK_EQUAL(" 10.1230", s.str());
    }
    {
        std::stringstream s; s << utxx::fixed(10.123, 8, 4, '0');
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

    const char s1[] = "abcde";
    const char s2[] = "abc\0\0";
    BOOST_REQUIRE_EQUAL(5, strnlen(s1));
    BOOST_REQUIRE_EQUAL(3, strnlen(s2));
}

BOOST_AUTO_TEST_CASE( test_string_replace )
{
    BOOST_CHECK_EQUAL("abc cdNNN", utxx::replace    ("abNNN cdNNN", "NNN", "c"));
    BOOST_CHECK_EQUAL("abc cdc",   utxx::replace_all("abNNN cdNNN", "NNN", "c"));
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
    TEST_WILDCARD("some same crazy address address",          "*address",  true);
    TEST_WILDCARD("some same crazy address address",          "*address*", true);
    TEST_WILDCARD("some same crazy address address!",         "*address", false);
    TEST_WILDCARD("some same crazy address address\naddress", "*address",  true);
    TEST_WILDCARD("some same crazy address address\nAddress", "*address", false);
    TEST_WILDCARD("some same crazy address address\nAddress", "*address*", true);
    TEST_WILDCARD("some same crazy address address\nAddress", "*?ddress",  true);
    TEST_WILDCARD("heloo address address Address Address address", "*address", true);

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

template <class Char = char>
struct Alloc {
    using value_type = Char;
    Char* allocate  (size_t n)          { ++s_ac; ++s_count; return std::allocator<Char>().allocate(n); }
    void  deallocate(Char* p, size_t n) { --s_count; std::allocator<Char>().deallocate(p, n); }

    static long  allocations()     { return s_count; }
    static long  tot_allocations() { return s_ac;    }
private:
    static long s_count;
    static long s_ac;
};

template <class Char> long Alloc<Char>::s_count;
template <class Char> long Alloc<Char>::s_ac;

BOOST_AUTO_TEST_CASE( test_string_short_string )
{
    using Allocator = Alloc<char>;
    using ss = basic_short_string<char, 64-1-2*8, Allocator>;
    static_assert(sizeof(ss) == 64, "Invalid size");

    BOOST_REQUIRE(ss::null_value().is_null());
    BOOST_REQUIRE(!ss::null_value());
    BOOST_CHECK_EQUAL(47, ss::max_size());

    BOOST_CHECK_EQUAL(64, ss::round_size(45));
    BOOST_CHECK_EQUAL(64, ss::round_size(46));
    BOOST_CHECK_EQUAL(64, ss::round_size(47));
    BOOST_CHECK_EQUAL(72, ss::round_size(48));

    Allocator salloc;

    { ss s; BOOST_CHECK_EQUAL(0, s.size()); }
    {
        ss s0 = ss("abc");

        ss s("a", salloc);
        BOOST_CHECK_EQUAL(1,   s.size());
        BOOST_CHECK_EQUAL("a", s);
        BOOST_CHECK_EQUAL("a", s.c_str());
        BOOST_CHECK_EQUAL("a", s.str());
        BOOST_CHECK_EQUAL("a", s.begin());
        BOOST_CHECK(s.begin()+1 == s.end());
        BOOST_CHECK(!s.is_null());
        BOOST_CHECK(s);
        BOOST_CHECK(!s.allocated());

        s.reset();
        BOOST_CHECK_EQUAL(0,   s.size());
        BOOST_CHECK(!s.is_null());
        BOOST_CHECK(s);
        BOOST_CHECK(!s.allocated());

        s.set_null();
        BOOST_CHECK(s.is_null());
        BOOST_CHECK(!s);
        BOOST_CHECK_EQUAL(-1, s.size());
        BOOST_CHECK_EQUAL("", s.c_str());
        BOOST_CHECK_EQUAL("", s.str());
        BOOST_CHECK(s.begin()  == s.end());
        BOOST_CHECK(s.cbegin() == s.cend());
        s.set("b");
        BOOST_CHECK(!s.is_null());
        BOOST_CHECK_EQUAL(1, s.size());
        BOOST_CHECK(s.begin()+1  == s.end());
        BOOST_CHECK(s.cbegin()+1 == s.cend());

        s.set("abc");
        BOOST_CHECK_EQUAL(3, s.size());
        s.resize(1);
        BOOST_CHECK_EQUAL(1, s.size());
        BOOST_CHECK(!s.is_null());
        BOOST_CHECK(!s.allocated());

        s.set(nullptr, -1);
        BOOST_CHECK(s.is_null());
        BOOST_CHECK_EQUAL(-1, s.size());
        s.append("y");
        BOOST_CHECK(!s.is_null());
        BOOST_CHECK_EQUAL(1, s.size());
        BOOST_CHECK_EQUAL("y", s.c_str());
        BOOST_CHECK_EQUAL("y", s.str());
        BOOST_CHECK(s.begin()+1  == s.end());
        BOOST_CHECK(s.cbegin()+1 == s.cend());

        const std::string test(80, 'x');
        s = test;                                   // <-- allocation 1
        BOOST_CHECK(s.allocated());
        BOOST_CHECK(s == test);
        BOOST_CHECK_EQUAL(test, s.c_str());
        s.reset();
        BOOST_CHECK(!s.allocated());
        BOOST_CHECK_EQUAL(0, s.size());

        BOOST_CHECK_EQUAL(1, Allocator::tot_allocations());
        BOOST_CHECK_EQUAL(0, Allocator::allocations());

        std::string test2(test);
        s = test2;                                  // <-- allocation 2
        BOOST_CHECK(s.allocated());
        BOOST_CHECK(s == test2);
        BOOST_CHECK_EQUAL(test2, s.c_str());

        std::string test3(s);
        BOOST_CHECK(s == test3);
        BOOST_CHECK_EQUAL(test3, s.c_str());

        BOOST_CHECK_EQUAL(2, Allocator::tot_allocations());
        BOOST_CHECK_EQUAL(1, Allocator::allocations());

        {
            std::string t1(30, 'a');
            std::string a1(5,  'b');
            std::string a2(50, 'c');
            ss s(t1);
            BOOST_CHECK_EQUAL(t1, s.c_str());
            BOOST_CHECK_EQUAL(t1.size(), s.size());
            BOOST_CHECK_EQUAL(64-1-2*8,  s.capacity());
            BOOST_CHECK(!s.allocated());
            s.append(a1);
            BOOST_CHECK_EQUAL(t1+a1, s.c_str());
            BOOST_CHECK_EQUAL(t1.size()+a1.size(), s.size());
            BOOST_CHECK_EQUAL(64-1-2*8, s.capacity());
            BOOST_CHECK(!s.allocated());

            s.append(a2);                           // <-- allocation 3
            BOOST_CHECK_EQUAL(t1+a1+a2, s.c_str());
            BOOST_CHECK_EQUAL(t1.size()+a1.size()+a2.size(), s.size());
            BOOST_CHECK_EQUAL(87, s.capacity());    // allocations are rounded by 8 minus 1 (for '\0')
            BOOST_CHECK(s.allocated());

            s.reserve(60);                          // <-- allocation 4
            BOOST_CHECK_EQUAL(t1+a1+a2, s.c_str());
            BOOST_CHECK_EQUAL(t1.size()+a1.size()+a2.size(), s.size());
            BOOST_CHECK_EQUAL(87, s.capacity());    // allocations are rounded by 8 minus 1 (for '\0')
            BOOST_CHECK(s.allocated());

            s.reserve(90);
            BOOST_CHECK_EQUAL(t1+a1+a2, s.c_str());
            BOOST_CHECK_EQUAL(t1.size()+a1.size()+a2.size(), s.size());
            BOOST_CHECK_EQUAL(95, s.capacity()); // allocations are rounded by 8 minus 1 (for '\0')
            BOOST_CHECK(s.allocated());

            s.clear();
            BOOST_CHECK_EQUAL(0, s.size());
            BOOST_CHECK_EQUAL(95, s.capacity());
            BOOST_CHECK(s.allocated());

            s.reset();
            BOOST_CHECK_EQUAL(0, s.size());
            BOOST_CHECK_EQUAL(64-1-2*8, s.capacity());
            BOOST_CHECK(!s.allocated());
        }
    }
    BOOST_CHECK_EQUAL(4, Allocator::tot_allocations());
    BOOST_CHECK_EQUAL(0, Allocator::allocations());

    using ssu = basic_short_string<unsigned char>;
    { ssu s; BOOST_CHECK_EQUAL(0, s.size()); }
    {
        const unsigned char* test = (const unsigned char*)"a";
        ssu s(test);
        BOOST_CHECK_EQUAL(1, s.size());
        BOOST_CHECK_EQUAL(s, test);
        BOOST_CHECK(!strncmp((const char*)s.c_str(), (const char*)test, 1));
        BOOST_CHECK(!strncmp((const char*)s.str(),   (const char*)test, 1));
        BOOST_CHECK(!s.allocated());

        BOOST_CHECK_EQUAL(4, Allocator::tot_allocations());
        BOOST_CHECK_EQUAL(0, Allocator::allocations());
    }
}

BOOST_AUTO_TEST_CASE( test_string_fixed_string )
{
    using str = basic_fixed_string<8>;
    using map = std::unordered_map<str, int>;
    using set = std::set<str>;
    {
        auto s = str("abc");
        BOOST_CHECK_EQUAL(3, s.size());
        BOOST_CHECK(!s.empty());
        BOOST_CHECK(s == "abc");
        BOOST_CHECK(s == std::string("abc"));
        s.set("123");
        BOOST_CHECK(s == "123");
        s.set(std::string("12345678"));
        BOOST_CHECK(s == "123456");
    }
}

BOOST_AUTO_TEST_CASE( test_string_hex )
{
    const char* src = "KLMN0123";

    auto  res1 = hex(src, strlen(src));
    auto  res2 = unhex_string(res1);
    auto  res3 = unhex_string(boost::to_lower_copy(res1));

    const char* expect = "4B4C4D4E30313233";

    BOOST_CHECK_EQUAL(expect, res1);
    BOOST_CHECK_EQUAL(src,    res2);
    BOOST_CHECK_EQUAL(src,    res3);

    BOOST_CHECK_EQUAL("313233",   hex(123));
    BOOST_CHECK_EQUAL("4b4c4d4e", hex(std::string("KLMN"), true));

    auto res4 = to_hex_string(std::string(src));
    BOOST_CHECK_EQUAL(expect, res4);
    auto res5 = hex(std::string(src), true);
    BOOST_CHECK_EQUAL("4b4c4d4e30313233", res5);
}

