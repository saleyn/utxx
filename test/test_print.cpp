//----------------------------------------------------------------------------
/// \file  test_print.hpp
//----------------------------------------------------------------------------
/// \brief Test implementation of printing functions.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-09-05
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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
#include <utxx/print.hpp>
#include <utxx/print_opts.hpp>
#include <utxx/time_val.hpp>
#include <utxx/string.hpp>
#include <iostream>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_print_opts )
{
    {
        std::stringstream ss;
        const char s[] = "abcd";
        output(ss, s, s+length(s), print_opts::hex);
        BOOST_CHECK_EQUAL("61,62,63,64", ss.str());
        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::hex, "|");
        BOOST_CHECK_EQUAL("61|62|63|64", ss.str());
        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::hex, "|", "0x");
        BOOST_CHECK_EQUAL("0x61|0x62|0x63|0x64", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::dec);
        BOOST_CHECK_EQUAL("97,98,99,100", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_string);
        BOOST_CHECK_EQUAL("abcd", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_or_hex);
        BOOST_CHECK_EQUAL("abcd", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_or_dec);
        BOOST_CHECK_EQUAL("abcd", ss.str());
    }
    {
        std::stringstream ss;
        const char s[] = "a\0cd";
        output(ss, s, s+length(s), print_opts::hex);
        BOOST_CHECK_EQUAL("61,00,63,64", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::dec);
        BOOST_CHECK_EQUAL("97,0,99,100", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_string);
        BOOST_CHECK_EQUAL("a.cd", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_or_hex);
        BOOST_CHECK_EQUAL("61,00,63,64", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_or_dec);
        BOOST_CHECK_EQUAL("97,0,99,100", ss.str());
    }
    {
        std::stringstream ss;
        const char s[] = "a\t\nb";
        output(ss, s, s+length(s), print_opts::hex);
        BOOST_CHECK_EQUAL("61,09,0a,62", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::dec);
        BOOST_CHECK_EQUAL("97,9,10,98", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_string);
        BOOST_CHECK_EQUAL("a\\t\\nb", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_or_hex);
        BOOST_CHECK_EQUAL("a\t\nb", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_or_dec);
        BOOST_CHECK_EQUAL("a\t\nb", ss.str());
    }
    {
        std::stringstream ss;
        const char s[] = "";
        output(ss, s, s+length(s), print_opts::hex);
        BOOST_CHECK_EQUAL("", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::dec);
        BOOST_CHECK_EQUAL("", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_string);
        BOOST_CHECK_EQUAL("", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_or_hex);
        BOOST_CHECK_EQUAL("", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_or_dec);
        BOOST_CHECK_EQUAL("", ss.str());
    }
    {
        std::stringstream ss;
        const char s[] = "abc";
        const char e[] = "";
        output(ss, e, e, print_opts::dec, ",", "", "\"", "<<", ">>");
        BOOST_CHECK_EQUAL("<<>>", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::hex, ",", "", "\"", "<<", ">>");
        BOOST_CHECK_EQUAL("<<61,62,63>>", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::dec, ",", "", "\"", "<<", ">>");
        BOOST_CHECK_EQUAL("<<97,98,99>>", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_string, ",", "", "\"", "<<", ">>");
        BOOST_CHECK_EQUAL("<<\"abc\">>", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_or_hex, ",", "", "\"", "<<", ">>");
        BOOST_CHECK_EQUAL("<<\"abc\">>", ss.str());

        ss.str(std::string());
        output(ss, s, s+length(s), print_opts::printable_or_dec, ",", "", "\"", "<<", ">>");
        BOOST_CHECK_EQUAL("<<\"abc\">>", ss.str());
    }
}

BOOST_AUTO_TEST_CASE( test_print )
{
    using sstrm = std::stringstream;

    { sstrm s; s << width<7, LEFT, double>(2.123, 2); BOOST_CHECK_EQUAL("2.12   ", s.str()); }
    { sstrm s; s << width<7, RIGHT,double>(2.123, 2); BOOST_CHECK_EQUAL("   2.12", s.str()); }
    { sstrm s; s << fixed(2.123, 6, 3, ' '); BOOST_CHECK_EQUAL(" 2.123", s.str()); }
    { sstrm s; s << fixed(2.123, 2);         BOOST_CHECK_EQUAL("2.12",   s.str()); }
    { sstrm s; s << fixed(2.100, 3, false);  BOOST_CHECK_EQUAL("2.100",  s.str()); }
    { sstrm s; s << fixed(2.100, 3);         BOOST_CHECK_EQUAL("2.1",    s.str()); }
    { sstrm s; s << fixed(2,     5);         BOOST_CHECK_EQUAL("    2",  s.str()); }
    { sstrm s; s << fixed(12345, 3);         BOOST_CHECK_EQUAL("123",    s.str()); }
    { sstrm s; s << fixed(12345, 3, RIGHT);  BOOST_CHECK_EQUAL("123",    s.str()); }

    std::string str("xxx");

    { std::string s = print(1);      BOOST_CHECK_EQUAL("1",     s); }
    { std::string s = print(1.0);    BOOST_CHECK_EQUAL("1.0",   s); }
    { std::string s = print(true);   BOOST_CHECK_EQUAL("true",  s); }
    { std::string s = print('c');    BOOST_CHECK_EQUAL("c",     s); }
    { std::string s = print(false);  BOOST_CHECK_EQUAL("false", s); }
    { std::string s = print("abc");  BOOST_CHECK_EQUAL("abc",   s); }
    { std::string s = print(str);    BOOST_CHECK_EQUAL("xxx",   s); }
    { std::string s = print(fixed(2.123, 6, 3, ' ')); BOOST_CHECK_EQUAL(" 2.123", s); }
    { std::string s = print(fixed(2.123, 2));         BOOST_CHECK_EQUAL("2.12", s);   }
    { std::string s = print(width<7, LEFT, double>(2.123, 2     )); BOOST_CHECK_EQUAL("2.12   ", s); }
    { std::string s = print(width<7, RIGHT,double>(2.123, 2     )); BOOST_CHECK_EQUAL("   2.12", s); }
    { std::string s = print(width<7, LEFT, double>(2.123, 2, '0')); BOOST_CHECK_EQUAL("2.12000", s); }
    { std::string s = print(width<7, RIGHT,double>(2.123, 2, '0')); BOOST_CHECK_EQUAL("0002.12", s); }
    { std::string s = print(width<7, LEFT, int>   (123     ));      BOOST_CHECK_EQUAL("123    ", s); }
    { std::string s = print(width<7, RIGHT,int>   (123     ));      BOOST_CHECK_EQUAL("    123", s); }
    { std::string s = print(width<7, LEFT, int>   (123, '_'));      BOOST_CHECK_EQUAL("123____", s); }
    { std::string s = print(width<7, RIGHT,int>   (123, '0'));      BOOST_CHECK_EQUAL("0000123", s); }
    { std::string s = print(width<7, LEFT, bool>  (true     ));     BOOST_CHECK_EQUAL("true   ", s); }
    { std::string s = print(width<7, RIGHT,bool>  (true     ));     BOOST_CHECK_EQUAL("   true", s); }
    { std::string s = print(width<7, LEFT, bool>  (false    ));     BOOST_CHECK_EQUAL("false  ", s); }
    { std::string s = print(width<7, RIGHT,bool>  (false    ));     BOOST_CHECK_EQUAL("  false", s); }
    { std::string s = print(make_width<7, RIGHT>  ("abc"));         BOOST_CHECK_EQUAL("    abc", s); }
    { std::string s = print(make_width<7, LEFT>   ("abc"));         BOOST_CHECK_EQUAL("abc    ", s); }
    { std::string s = print(make_width<7, LEFT>   ('a'));           BOOST_CHECK_EQUAL("a      ", s); }
    { std::string s = print(width<7, RIGHT,const std::string&>(str)); BOOST_CHECK_EQUAL("    xxx", s); }
    { std::string s = print(width<7, LEFT, const std::string&>(str)); BOOST_CHECK_EQUAL("xxx    ", s); }
    { std::string s = print(make_width<7, LEFT>(str)); BOOST_CHECK_EQUAL("xxx    ", s); }
}


BOOST_AUTO_TEST_CASE( test_print_perf )
{
    static const int ITERATIONS = getenv("ITERATIONS")
                                ? atoi(getenv("ITERATIONS")) : 1000000;
    double elapsed1, elapsed2;
    {
        char buf[256];
        timer tm;
        for (int i=0; i < ITERATIONS; i++) {
            snprintf(buf, sizeof(buf)-1, "%d",    10000);
            snprintf(buf, sizeof(buf)-1, "%.6f",  12345.6789);
            snprintf(buf, sizeof(buf)-1, "%6.3f", 2.123);
            snprintf(buf, sizeof(buf)-1, "%s",    "this is a test string");
            snprintf(buf, sizeof(buf)-1, "%s",    i % 2 ? "true" : "false");
        }
        elapsed1 = tm.elapsed();
    }

    {
        basic_buffered_print<> b;
        timer tm;
        for (int i=0; i < ITERATIONS; i++) {
            b.print(10000);                     b.reset();
            b.print(12345.6789);                b.reset();
            b.print(fixed(2.123, 6, 3));        b.reset();
            b.print("this is a test string");   b.reset();
            b.print(i % 2);
        }
        elapsed2 = tm.elapsed();
    }

    BOOST_TEST_MESSAGE(" printf      speed: " << fixed(double(ITERATIONS)/elapsed1, 10, 0) << " calls/s");
    BOOST_TEST_MESSAGE(" utxx::print speed: " << fixed(double(ITERATIONS)/elapsed2, 10, 0) << " calls/s");
    BOOST_TEST_MESSAGE("    printf / print: " << fixed(elapsed1/elapsed2, 6, 4) << " times");

}

