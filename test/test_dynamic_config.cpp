//----------------------------------------------------------------------------
/// \file  test_dynamic_config.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for dynamic configuration
//----------------------------------------------------------------------------
// Copyright (c) 2016 Serge Aleynikov <saleyn@gmail.com>
// Created: 2016-08-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects

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

#include <utxx/config.h>
#ifdef UTXX_HAVE_BOOST
#include <utxx/test_helper.hpp>
#include <utxx/dynamic_config.hpp>
#include <utxx/path.hpp>

using namespace utxx;

TEST_CASE( test_dynamic_config )
{
    static const char* s_file = "/tmp/dynconfig.bin";

    dynamic_config<64> dc;

    path::file_unlink(s_file);

    bool existing = dc.init(s_file);

    CHECK(existing);

    long&   p1 = dc.bind<long>("param1");
    bool&   p2 = dc.bind<bool>("param2");
    double& p3 = dc.bind<double>("param3");
    auto&   p4 = dc.bind<dparam_str_t>("param4");

    p1 = 10;
    p2 = true;
    p3 = 1.234;
    p4.copy_from("abcd");

    CHECK_EQUAL(4, dc.count());

    CHECK_EQUAL("param1", dc.name(&p1));
    CHECK_EQUAL("param2", dc.name(&p2));
    CHECK_EQUAL("param3", dc.name(&p3));
    CHECK_EQUAL("param4", dc.name(&p4));
    CHECK(dc.name("some value") == nullptr);

    dc.close();

    {
        bool existing = dc.init(s_file);

        CHECK(!existing);
        CHECK_EQUAL(4, dc.count());

        long&   p1 = dc.bind<long>("param1");
        bool&   p2 = dc.bind<bool>("param2");
        double& p3 = dc.bind<double>("param3");
        auto&   p4 = dc.bind<dparam_str_t>("param4");

        CHECK_EQUAL(10, p1);
        CHECK(p2);
        CHECK_EQUAL(1.234,  p3);
        CHECK_EQUAL("abcd", p4.data());

        CHECK_EQUAL(4, dc.count());
    }

    path::file_unlink(s_file);
}
#endif // UTXX_HAVE_BOOST
