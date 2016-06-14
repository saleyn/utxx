//----------------------------------------------------------------------------
/// \file  test_nchar.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for nchar class.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-30
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
#include <utxx/nchar.hpp>
#include <iostream>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_nchar )
{
    {
        nchar<4> rc("abcd", 4);
        std::string rcs = rc.to_string();
        BOOST_CHECK_EQUAL(4u, rcs.size());
        BOOST_CHECK_EQUAL("abcd", rcs);
    }
    {
        nchar<4> rc = std::string("ff");
        std::string rcs = rc.to_string();
        BOOST_CHECK_EQUAL(2u, rcs.size());
        BOOST_CHECK_EQUAL("ff", rcs.c_str());
    }
    {
        nchar<4> rc("ff");
        const uint8_t expect[] = {'f','f',0,0};
        BOOST_CHECK_EQUAL(0, memcmp(expect, (char*)rc, sizeof(expect)));
        rc = std::string("");
        std::string rcs = rc.to_string();
        BOOST_CHECK_EQUAL(0u, rcs.size());
        BOOST_CHECK_EQUAL("", rcs);
    }
    {
        const char s[] = "abcd";
        nchar<4> rc(s);
        BOOST_CHECK_EQUAL(rc.to_string(), "abcd");
        std::stringstream str;
        rc.dump(str);
        BOOST_CHECK_EQUAL(str.str(), "abcd");
    }
    {
        nchar<4> rc(1);
        BOOST_CHECK_EQUAL(1, rc.to_binary<int>());
        const uint8_t expect[] = {0,0,0,1};
        BOOST_CHECK_EQUAL(0, memcmp(expect, (char*)rc, sizeof(expect)));
        std::stringstream str;
        rc.dump(str);
        BOOST_CHECK_EQUAL(str.str(), "0,0,0,1");
        rc.fill(' ');
        BOOST_CHECK_EQUAL("    ", rc.to_string());
        BOOST_CHECK_EQUAL(0, rc.to_integer<int>());
        rc.fill('0');
        BOOST_CHECK_EQUAL("0000", rc.to_string());
        BOOST_CHECK_EQUAL(0, rc.to_integer<int>());
    }

    {
        nchar<8> rc(" abc   ");
        BOOST_CHECK_EQUAL(" abc   ", rc.to_string());
        BOOST_CHECK_EQUAL(" abc",    rc.to_string(' '));
    }

    {
        nchar<6> rc("abc\n  ", 6);
        BOOST_CHECK_EQUAL(3u, rc.len('\n'));
        BOOST_CHECK_EQUAL(6u, rc.len('\0'));
        BOOST_CHECK_EQUAL(6u, rc.len('X'));

        char buf1[20];
        nchar<6> rx("abc\nxx", 6);
        char* p = rx.copy_to(buf1, sizeof(buf1), '\n');
        BOOST_CHECK_EQUAL("abc",  buf1);
        BOOST_CHECK_EQUAL(buf1+3, p);

        p = rx.copy_to(buf1, sizeof(buf1), 'x');
        BOOST_CHECK_EQUAL("abc\n",  buf1);
        BOOST_CHECK_EQUAL(buf1+4, p);

        p = rx.copy_to(buf1, sizeof(buf1), '\0');
        BOOST_CHECK_EQUAL("abc\nxx", buf1);
        BOOST_CHECK_EQUAL(buf1+6, p);

        char buf2[3];
        p = rx.copy_to(buf2, sizeof(buf2), '\0');
        BOOST_CHECK_EQUAL("ab", buf2);
        BOOST_CHECK_EQUAL(buf2+2, p);
    }
}

BOOST_AUTO_TEST_CASE( test_nchar_to_binary )
{
    {
        const uint8_t expect[] = {0,1};
        nchar<2> rc((uint16_t)1);
        BOOST_CHECK_EQUAL(1, rc.to_binary<uint16_t>());
        BOOST_CHECK_EQUAL(0, memcmp(expect, (char*)rc, sizeof(expect)));
    }
    {
        const uint8_t expect[] = {255, 246};
        nchar<2> rc((int16_t)-10);
        BOOST_CHECK_EQUAL((int16_t)-10, rc.to_binary<int16_t>());
        BOOST_CHECK_EQUAL(0, memcmp(expect, (char*)rc, sizeof(expect)));
    }
    {
        const uint8_t expect[] = {0,0,0,10};
        nchar<4> rc((uint32_t)10);
        BOOST_CHECK_EQUAL((uint32_t)10, rc.to_binary<uint32_t>());
        BOOST_CHECK_EQUAL(0, memcmp(expect, (char*)rc, sizeof(expect)));
    }
    {
        const uint8_t expect[] = {255, 255, 255, 246};
        nchar<4> rc((int32_t)-10);
        BOOST_CHECK_EQUAL((int32_t)-10, rc.to_binary<int32_t>());
        BOOST_CHECK_EQUAL(0, memcmp(expect, (char*)rc, sizeof(expect)));
    }
    {
        const uint8_t expect[] = {255, 255, 255, 255, 255, 255, 255, 246};
        nchar<8> rc((int64_t)-10);
        BOOST_CHECK_EQUAL((int64_t)-10, rc.to_binary<int64_t>());
        BOOST_CHECK_EQUAL(0, memcmp(expect, (char*)rc, sizeof(expect)));
    }
    {
        const uint8_t expect[] = {0, 0, 0, 0, 0, 0, 0, 10};
        nchar<8> rc((uint64_t)10);
        BOOST_CHECK_EQUAL((uint64_t)10, rc.to_binary<uint64_t>());
        BOOST_CHECK_EQUAL(0, memcmp(expect, (char*)rc, sizeof(expect)));
    }
    {
        const uint8_t expect[] = {63,240,0,0,0,0,0,0};
        nchar<8> rc(1.0);
        BOOST_CHECK_EQUAL(1.0, rc.to_binary<double>());
        BOOST_CHECK_EQUAL(0, memcmp(expect, (char*)rc, sizeof(expect)));
    }
}

BOOST_AUTO_TEST_CASE( test_nchar_to_integer )
{
    {
        nchar<8> rc1("12345678");
        BOOST_CHECK_EQUAL(12345678, rc1.to_integer<int>());
        BOOST_CHECK_EQUAL(12345678, rc1.to_integer<int>(' '));
        BOOST_CHECK_EQUAL(2345678,  rc1.to_integer<int>('1'));
        BOOST_CHECK_EQUAL(12345678, rc1.to_integer<int>('2'));
        nchar<16> rc2("-123456789012345");
        BOOST_CHECK_EQUAL(-123456789012345ll, rc2.to_integer<int64_t>());
        const char* p = "  12";
        nchar<4> rc3(p, 4);
        BOOST_CHECK_EQUAL(12, rc3.to_integer<int>());
        nchar<4> rc4(p, 4);
        BOOST_CHECK_EQUAL(12, rc4.to_integer<int>(' '));
        {
            const char* p = "   -";
            nchar<4> rc(p, 4);
            BOOST_CHECK_EQUAL(0, rc.to_integer<int>(' '));
        }
        {
            const char* p = "-123";
            nchar<4> rc(p, 4);
            BOOST_CHECK_EQUAL(-123, rc.to_integer<int>(' '));
            BOOST_CHECK_EQUAL(123,  rc.to_integer<int>('-'));
        }
    }
}

BOOST_AUTO_TEST_CASE( test_nchar_from_integer )
{
    {
        nchar<8> rc;
        rc.from_integer(12345678);
        BOOST_CHECK_EQUAL("12345678", rc.to_string());
    }
    {
        nchar<16> rc;
        rc.from_integer(-123456789012345);
        BOOST_CHECK_EQUAL("-123456789012345", rc.to_string());
    }
    {
        nchar<4> rc;
        rc.from_integer(12);
        BOOST_CHECK_EQUAL("12", rc.to_string());
        rc.fill(' ');
        rc.from_integer(-12);
        BOOST_CHECK_EQUAL("-12", rc.to_string());
        rc.fill(' ');
        rc.from_integer(12, ' ');
        BOOST_CHECK_EQUAL("12  ", rc.to_string());
        rc.fill(' ');
        rc.from_integer(-12, ' ');
        BOOST_CHECK_EQUAL("-12 ", rc.to_string());
    }
    {
        nchar<4> rc;
        rc.from_integer(0, ' ');
        BOOST_CHECK_EQUAL("0   ", rc.to_string());
    }
}

BOOST_AUTO_TEST_CASE( test_nchar_to_double )
{
    {
        nchar<12> rc1("12345678.123");
        BOOST_CHECK_EQUAL(12345678.123, rc1.to_double());
        BOOST_CHECK_EQUAL(12345678.123, rc1.to_double(' '));
        BOOST_CHECK_EQUAL(2345678.123,  rc1.to_double('1'));
        BOOST_CHECK_EQUAL(12345678.123, rc1.to_double('2'));
        nchar<16> rc2("-1234567890.567");
        BOOST_CHECK_EQUAL(-1234567890.567, rc2.to_double());
        const char* p = " 12.34  ";
        nchar<8> rc3(p, 7);
        BOOST_CHECK_EQUAL(12.34, rc3.to_double()); // Spaces are skipped by default
        nchar<8> rc4(p+1, 6);
        BOOST_CHECK_EQUAL(12.34, rc4.to_double());
        nchar<8> rc5(p, 7);
        BOOST_CHECK_EQUAL(12.34, rc5.to_double(' '));
        {
            const char* p = "  1.2";
            nchar<6> rc(p, 5);
            BOOST_CHECK_EQUAL(1.2, rc.to_double(' '));
        }
        {
            const char* p = "-123.1";
            nchar<6> rc(p, 6);
            BOOST_CHECK_EQUAL(-123.1, rc.to_double(' '));
            BOOST_CHECK_EQUAL(123.1,  rc.to_double('-'));
        }
    }
}

BOOST_AUTO_TEST_CASE( test_nchar_from_double )
{
    {
        nchar<9> rc;
        BOOST_CHECK_EQUAL(9, rc.from_double(12345.67, 2, ' '));
        BOOST_CHECK_EQUAL(8, rc.from_double(12345.67, 2, false));
        BOOST_CHECK_EQUAL("12345.67", rc.to_string());
    }
    {
        nchar<17> rc;
        BOOST_CHECK_EQUAL(16, rc.from_double(-12345678901.235, 6, true));
        BOOST_CHECK_EQUAL("-12345678901.235", rc.to_string());

        BOOST_CHECK_EQUAL(16, rc.from_double(-12345678901.234, 3, true));
        BOOST_CHECK_EQUAL("-12345678901.234", rc.to_string());
    }
    {
        nchar<6> rc;
        rc.from_double(12.1, 1, false);
        BOOST_CHECK_EQUAL("12.1",   rc.to_string());
        rc.from_double(12.1, 1, ' ');
        BOOST_CHECK_EQUAL("  12.1", rc.to_string());
        rc.fill(' ');
        rc.from_double(-12.1, 1, false);
        BOOST_CHECK_EQUAL("-12.1", rc.to_string());
        rc.from_double(12, 1, true, ' ');
        BOOST_CHECK_EQUAL("12.0  ", rc.to_string());
        rc.from_double(-12, 1, true, ' ');
        BOOST_CHECK_EQUAL("-12.0 ", rc.to_string());
    }
    {
        nchar<5> rc;
        BOOST_CHECK_EQUAL(3,       rc.from_double(0, 3, true, ' '));
        BOOST_CHECK_EQUAL("0.0  ", rc.to_string());
        BOOST_CHECK_EQUAL(3,       rc.from_double(0, 2, true, ' '));
        BOOST_CHECK_EQUAL("0.0  ", rc.to_string());
        BOOST_CHECK_EQUAL(4,       rc.from_double(0, 2, false, ' '));
        BOOST_CHECK_EQUAL("0.00 ", rc.to_string());

        BOOST_CHECK_EQUAL(5,       rc.from_double(0, 3, ' '));
        BOOST_CHECK_EQUAL(-1,      rc.from_double(-1,3, ' '));
        BOOST_CHECK_EQUAL(5,       rc.from_double(0, 2, ' '));
        BOOST_CHECK_EQUAL(" 0.00", rc.to_string());
        BOOST_CHECK_EQUAL(4,       rc.from_double(-1.2, 2, false)); // No space for '\0'
        BOOST_CHECK_EQUAL("-1.2",  rc.to_string());
        BOOST_CHECK_EQUAL(4,       rc.from_double(-1.2, 1, false));
        BOOST_CHECK_EQUAL(5,       rc.from_double(-1.2, 2, ' '));
        BOOST_CHECK_EQUAL("-1.20", rc.to_string());
    }
}

BOOST_AUTO_TEST_CASE( test_nchar_bad_cases )
{
    {
        const char* p = " -12";
        nchar<4> rc(p, 4);
        BOOST_CHECK_EQUAL(-12, rc.to_integer<int>(' '));
        BOOST_CHECK_EQUAL(-12, rc.to_integer<int>());
        BOOST_CHECK_EQUAL(0,   rc.to_integer<int>('-'));
    }
    {
        const char* p = "  12";
        nchar<4> rc(p, 4);
        BOOST_CHECK_EQUAL(12, rc.to_integer<int>());
        BOOST_CHECK_EQUAL(12, rc.to_integer<int>(' '));
    }
    {
        const char* p = "  12";
        nchar<5> rc(p, 4);
        BOOST_CHECK_EQUAL(120,rc.to_integer<int>()); // Empty spaces are treated as zeros
        BOOST_CHECK_EQUAL(12, rc.to_integer<int>(' '));
    }
    {
        const char* p = "--12";
        nchar<4> rc(p, 4);
        BOOST_CHECK_EQUAL(12, rc.to_integer<int>('-'));
    }
    {
        const char* p = " -1 ";
        nchar<4> rc(p, 4);
        BOOST_CHECK_EQUAL(-10,rc.to_integer<int>()); // Empty spaces are treated as zeros
        BOOST_CHECK_EQUAL(-1, rc.to_integer<int>(' '));
    }
}
