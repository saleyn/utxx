//----------------------------------------------------------------------------
/// \file  test_name.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes and functions in the name.hpp file.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-10-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source projects

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
#include <utxx/name.hpp>
#include <utxx/string.hpp>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_name )
{
    {
        char expect[] = "ABC";
        name_t s(expect);
        const std::string& result = s.to_string();
        BOOST_REQUIRE_EQUAL(expect, result);
    }
    {
        name_t s1("ABC");
        name_t s2("ABC");
        BOOST_CHECK_EQUAL(s1, s2);
        BOOST_CHECK(s1 == s2);
    }
    {
        name_t s1("ABCD.EFGH1");
        name_t s2("ABCD.EFGH1");
        BOOST_CHECK_EQUAL(s1, s2);
        BOOST_CHECK(s1 == s2);
        BOOST_CHECK(s1 != name_t("ABCD0EFGH1"));
    }
    {
        char expect[] = "ABC_EF";
        name_t s("aBc_Ef", 7, true);
        const std::string& result = s.to_string();
        BOOST_REQUIRE_EQUAL(expect, result);
    }
    {
        char value[]  = "C \177";
        char expect[] = "C";
        BOOST_REQUIRE_THROW(name_t s(value), badarg_error);
        name_t s1;
        int rc;
        s1.set(value, rc);
        const std::string& result = s1.to_string();
        BOOST_REQUIRE_EQUAL(expect, result);
        BOOST_REQUIRE_EQUAL(-1, rc);
    }
    {
        char expect[] = "A1C";
        name_t s(expect);
        char r[5];
        s.write(r);
        BOOST_REQUIRE_EQUAL(expect, r);
    }
    {
        char expect[5];
        strncpy(expect, "ABC  ", 5);
        { BOOST_REQUIRE_THROW(name_t s(expect), badarg_error); }
        name_t s;
        int rc;
        s.set(expect, rc);
        const std::string& result = s.to_string();
        BOOST_REQUIRE_EQUAL("ABC", result);
        BOOST_REQUIRE_EQUAL(-3, rc);
    }
    {
        char expect[6];
        snprintf(expect, sizeof(expect), "ABC  ");
        name_t s(expect, 3);
        char result[7];
        s.write(result, ' ');
        result[5] = '\0';
        BOOST_REQUIRE_EQUAL(std::string(expect), result);
        strncpy(expect, "ABCDE", 5);
        s.set(expect, 5);
        std::string exp(expect, 5);
        std::string res = s.to_string();
        BOOST_REQUIRE_EQUAL(exp, res);
    }

    // Test truncating string

    {
        char expect[11];
        snprintf(expect, sizeof(expect), "0123456789");
        name_t a("0123456789ABC");
        char result[11];
        a.write(result);
        BOOST_REQUIRE_EQUAL(std::string(expect), result);
    }

    // Test length

    {
        const char* s_test[] = {"0", "01", "012", "0123", "01234", "012345",
                                "0123456", "01234567", "012345678", "0123456789"};
        for (size_t i = 0; i < length(s_test); i++) {
            name_t x(s_test[i]);
            BOOST_REQUIRE_EQUAL(x.length(), i+1);
        }
    }

    // Test encoding

    {   name_t a("A");
        uint64_t i=a;
        name_t b(i);
        std::string x = a.to_string(), y = b.to_string();
        BOOST_REQUIRE_EQUAL(x, y);
    } { 
        name_t a("+-./@{|}~^");
        uint64_t i=a;
        name_t b(i);
        std::string x = a.to_string(), y = b.to_string();
        BOOST_REQUIRE_EQUAL(x, y);
    }
    { name_t a("0123456789"); uint64_t i=a; name_t b(i); BOOST_REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("ABCDEFGHIJ"); uint64_t i=a; name_t b(i); BOOST_REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("KLMNOPQRST"); uint64_t i=a; name_t b(i); BOOST_REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("UVWXYZ[]_:"); uint64_t i=a; name_t b(i); BOOST_REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a(";<=>?#$%&'"); uint64_t i=a; name_t b(i); BOOST_REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("()*");        uint64_t i=a; name_t b(i); BOOST_REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("A.B[123]=0"); uint64_t i=a; name_t b(i); BOOST_REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("~@#$%^&*()"); uint64_t i=a; name_t b(i); BOOST_REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("{}[]|:;'<>"); uint64_t i=a; name_t b(i); BOOST_REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("Z09?");       uint64_t i=a; name_t b(i); BOOST_REQUIRE_EQUAL(a.to_string(), b.to_string()); }

    // Test comparison

    { name_t a("0"),      b("A");     BOOST_REQUIRE(a < b); }
    { name_t a("9"),      b("A");     BOOST_REQUIRE(a < b); }
    { name_t a("A"),      b("Z");     BOOST_REQUIRE(a < b); }
    { name_t a("AB"),     b("AC");    BOOST_REQUIRE(a < b); }
    { name_t a("+"),      b("0");     BOOST_REQUIRE(a < b); }
    { name_t a("AB"),     b("[]");    BOOST_REQUIRE(a < b); }
    { name_t a("ADN"),    b("ALLZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("ALLZZ"),  b("APAZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("APAZZ"),  b("APB");   BOOST_REQUIRE(a < b); }
    { name_t a("APB"),    b("AZZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("AZZZZ"),  b("B");     BOOST_REQUIRE(a < b); }
    { name_t a("B"),      b("BGZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("BGZZZ"),  b("BH");    BOOST_REQUIRE(a < b); }
    { name_t a("BH"),     b("BRCZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("BRCZZ"),  b("BRD");   BOOST_REQUIRE(a < b); }
    { name_t a("BRD"),    b("CCKZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("CCKZZ"),  b("CCL");   BOOST_REQUIRE(a < b); }
    { name_t a("CCL"),    b("CMAZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("CMAZZ"),  b("CMB");   BOOST_REQUIRE(a < b); }
    { name_t a("CMB"),    b("CORZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("CORZZ"),  b("COS");   BOOST_REQUIRE(a < b); }
    { name_t a("COS"),    b("CVSZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("CVSZZ"),  b("CVT");   BOOST_REQUIRE(a < b); }
    { name_t a("CVT"),    b("DHZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("DI"),     b("DOAZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("DOB"),    b("EEMZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("EEN"),    b("ESMZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("ESN"),    b("FASZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("FAT"),    b("FSZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("FT"),     b("GIKZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("GIL"),    b("GPZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("GQ"),     b("HNZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("HO"),     b("ICZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("ID"),     b("IVZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("IW"),     b("IYSZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("IYT"),    b("JZZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("K"),      b("LLZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("LM"),     b("MCDZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("MCE"),    b("MMMZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("MMN"),    b("MSZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("MT"),     b("NDXZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("NDY"),    b("NVKZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("NVL"),    b("PABZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("PAC"),    b("PIZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("PJ"),     b("PXBZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("PXC"),    b("QQQZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("QQR"),    b("RRBZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("RRC"),    b("SBUZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("SBV"),    b("SKMZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("SKN"),    b("SPXZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("SPY"),    b("SPYZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("SPZ"),    b("SWJZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("SWK"),    b("TISZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("TIT"),    b("TVZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("TW"),     b("UPKZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("UPL"),    b("UYLZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("UYM"),    b("VYZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("VZ"),     b("WLSZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("WLT"),    b("XHZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("XI"),     b("XLZZZ"); BOOST_REQUIRE(a < b); }
    { name_t a("XM"),     b("ZZZZZ"); BOOST_REQUIRE(a < b); }
}


