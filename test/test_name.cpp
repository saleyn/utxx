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

#include <utxx/test_helper.hpp>
#include <utxx/name.hpp>
#include <utxx/string.hpp>

using namespace utxx;

TEST_CASE( test_name )
{
    {
        char expect[] = "ABC";
        name_t s(expect);
        const std::string& result = s.to_string();
        REQUIRE_EQUAL(expect, result);
    }
    {
        name_t s1("ABC");
        name_t s2("ABC");
        CHECK_EQUAL(s1, s2);
        CHECK(s1 == s2);
    }
    {
        name_t s1("ABCD.EFGH1");
        name_t s2("ABCD.EFGH1");
        CHECK_EQUAL(s1, s2);
        CHECK(s1 == s2);
        CHECK(s1 != name_t("ABCD0EFGH1"));
    }
    {
        char expect[] = "ABC_EF";
        name_t s("aBc_Ef", 7, true);
        const std::string& result = s.to_string();
        REQUIRE_EQUAL(expect, result);
    }
    {
        char value[]  = "C \177";
        char expect[] = "C";
        REQUIRE_THROW(name_t s(value), badarg_error);
        name_t s1;
        int rc;
        s1.set(value, rc);
        const std::string& result = s1.to_string();
        REQUIRE_EQUAL(expect, result);
        REQUIRE_EQUAL(-1, rc);
    }
    {
        char expect[] = "A1C";
        name_t s(expect);
        char r[5];
        s.write(r);
        REQUIRE_EQUAL(expect, r);
    }
    {
        char expect[5];
        memcpy(expect, "ABC  ", 5);
        { REQUIRE_THROW(name_t s(expect), badarg_error); }
        name_t s;
        int rc;
        s.set(expect, rc);
        const std::string& result = s.to_string();
        REQUIRE_EQUAL("ABC", result);
        REQUIRE_EQUAL(-3, rc);
    }
    {
        char expect[6];
        snprintf(expect, sizeof(expect), "ABC  ");
        name_t s(expect, 3);
        char result[7];
        s.write(result, ' ');
        result[5] = '\0';
        REQUIRE_EQUAL(std::string(expect), result);
        memcpy(expect, "ABCDE", 5);
        s.set(expect, 5);
        std::string exp(expect, 5);
        std::string res = s.to_string();
        REQUIRE_EQUAL(exp, res);
    }

    // Test truncating string

    {
        char expect[11];
        snprintf(expect, sizeof(expect), "0123456789");
        name_t a("0123456789ABC");
        char result[11];
        a.write(result);
        REQUIRE_EQUAL(std::string(expect), result);
    }

    // Test length

    {
        const char* s_test[] = {"0", "01", "012", "0123", "01234", "012345",
                                "0123456", "01234567", "012345678", "0123456789"};
        for (size_t i = 0; i < length(s_test); i++) {
            name_t x(s_test[i]);
            REQUIRE_EQUAL(x.length(), i+1);
        }
    }

    // Test encoding

    {   name_t a("A");
        uint64_t i=a;
        name_t b(i);
        std::string x = a.to_string(), y = b.to_string();
        REQUIRE_EQUAL(x, y);
    } { 
        name_t a("+-./@{|}~^");
        uint64_t i=a;
        name_t b(i);
        std::string x = a.to_string(), y = b.to_string();
        REQUIRE_EQUAL(x, y);
    }
    { name_t a("0123456789"); uint64_t i=a; name_t b(i); REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("ABCDEFGHIJ"); uint64_t i=a; name_t b(i); REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("KLMNOPQRST"); uint64_t i=a; name_t b(i); REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("UVWXYZ[]_:"); uint64_t i=a; name_t b(i); REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a(";<=>?#$%&'"); uint64_t i=a; name_t b(i); REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("()*");        uint64_t i=a; name_t b(i); REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("A.B[123]=0"); uint64_t i=a; name_t b(i); REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("~@#$%^&*()"); uint64_t i=a; name_t b(i); REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("{}[]|:;'<>"); uint64_t i=a; name_t b(i); REQUIRE_EQUAL(a.to_string(), b.to_string()); }
    { name_t a("Z09?");       uint64_t i=a; name_t b(i); REQUIRE_EQUAL(a.to_string(), b.to_string()); }

    // Test comparison

    { name_t a("0"),      b("A");     REQUIRE(a < b); }
    { name_t a("9"),      b("A");     REQUIRE(a < b); }
    { name_t a("A"),      b("Z");     REQUIRE(a < b); }
    { name_t a("AB"),     b("AC");    REQUIRE(a < b); }
    { name_t a("+"),      b("0");     REQUIRE(a < b); }
    { name_t a("AB"),     b("[]");    REQUIRE(a < b); }
    { name_t a("ADN"),    b("ALLZZ"); REQUIRE(a < b); }
    { name_t a("ALLZZ"),  b("APAZZ"); REQUIRE(a < b); }
    { name_t a("APAZZ"),  b("APB");   REQUIRE(a < b); }
    { name_t a("APB"),    b("AZZZZ"); REQUIRE(a < b); }
    { name_t a("AZZZZ"),  b("B");     REQUIRE(a < b); }
    { name_t a("B"),      b("BGZZZ"); REQUIRE(a < b); }
    { name_t a("BGZZZ"),  b("BH");    REQUIRE(a < b); }
    { name_t a("BH"),     b("BRCZZ"); REQUIRE(a < b); }
    { name_t a("BRCZZ"),  b("BRD");   REQUIRE(a < b); }
    { name_t a("BRD"),    b("CCKZZ"); REQUIRE(a < b); }
    { name_t a("CCKZZ"),  b("CCL");   REQUIRE(a < b); }
    { name_t a("CCL"),    b("CMAZZ"); REQUIRE(a < b); }
    { name_t a("CMAZZ"),  b("CMB");   REQUIRE(a < b); }
    { name_t a("CMB"),    b("CORZZ"); REQUIRE(a < b); }
    { name_t a("CORZZ"),  b("COS");   REQUIRE(a < b); }
    { name_t a("COS"),    b("CVSZZ"); REQUIRE(a < b); }
    { name_t a("CVSZZ"),  b("CVT");   REQUIRE(a < b); }
    { name_t a("CVT"),    b("DHZZZ"); REQUIRE(a < b); }
    { name_t a("DI"),     b("DOAZZ"); REQUIRE(a < b); }
    { name_t a("DOB"),    b("EEMZZ"); REQUIRE(a < b); }
    { name_t a("EEN"),    b("ESMZZ"); REQUIRE(a < b); }
    { name_t a("ESN"),    b("FASZZ"); REQUIRE(a < b); }
    { name_t a("FAT"),    b("FSZZZ"); REQUIRE(a < b); }
    { name_t a("FT"),     b("GIKZZ"); REQUIRE(a < b); }
    { name_t a("GIL"),    b("GPZZZ"); REQUIRE(a < b); }
    { name_t a("GQ"),     b("HNZZZ"); REQUIRE(a < b); }
    { name_t a("HO"),     b("ICZZZ"); REQUIRE(a < b); }
    { name_t a("ID"),     b("IVZZZ"); REQUIRE(a < b); }
    { name_t a("IW"),     b("IYSZZ"); REQUIRE(a < b); }
    { name_t a("IYT"),    b("JZZZZ"); REQUIRE(a < b); }
    { name_t a("K"),      b("LLZZZ"); REQUIRE(a < b); }
    { name_t a("LM"),     b("MCDZZ"); REQUIRE(a < b); }
    { name_t a("MCE"),    b("MMMZZ"); REQUIRE(a < b); }
    { name_t a("MMN"),    b("MSZZZ"); REQUIRE(a < b); }
    { name_t a("MT"),     b("NDXZZ"); REQUIRE(a < b); }
    { name_t a("NDY"),    b("NVKZZ"); REQUIRE(a < b); }
    { name_t a("NVL"),    b("PABZZ"); REQUIRE(a < b); }
    { name_t a("PAC"),    b("PIZZZ"); REQUIRE(a < b); }
    { name_t a("PJ"),     b("PXBZZ"); REQUIRE(a < b); }
    { name_t a("PXC"),    b("QQQZZ"); REQUIRE(a < b); }
    { name_t a("QQR"),    b("RRBZZ"); REQUIRE(a < b); }
    { name_t a("RRC"),    b("SBUZZ"); REQUIRE(a < b); }
    { name_t a("SBV"),    b("SKMZZ"); REQUIRE(a < b); }
    { name_t a("SKN"),    b("SPXZZ"); REQUIRE(a < b); }
    { name_t a("SPY"),    b("SPYZZ"); REQUIRE(a < b); }
    { name_t a("SPZ"),    b("SWJZZ"); REQUIRE(a < b); }
    { name_t a("SWK"),    b("TISZZ"); REQUIRE(a < b); }
    { name_t a("TIT"),    b("TVZZZ"); REQUIRE(a < b); }
    { name_t a("TW"),     b("UPKZZ"); REQUIRE(a < b); }
    { name_t a("UPL"),    b("UYLZZ"); REQUIRE(a < b); }
    { name_t a("UYM"),    b("VYZZZ"); REQUIRE(a < b); }
    { name_t a("VZ"),     b("WLSZZ"); REQUIRE(a < b); }
    { name_t a("WLT"),    b("XHZZZ"); REQUIRE(a < b); }
    { name_t a("XI"),     b("XLZZZ"); REQUIRE(a < b); }
    { name_t a("XM"),     b("ZZZZZ"); REQUIRE(a < b); }
}


