//----------------------------------------------------------------------------
/// \file  test_buffer.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating buffer.hpp functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2012 Serge Aleynikov <saleyn@gmail.com>
// Created: 2012-05-06
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

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

#include <boost/test/unit_test.hpp>

#include <utxx/endian.hpp>

namespace utxx {

BOOST_AUTO_TEST_CASE( test_endian )
{
    {
        const char test[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        int64_t    resb  = cast_be<int64_t>(test);
        BOOST_CHECK_EQUAL(72623859790382856, resb);
        int64_t    resl  = cast_le<int64_t>(test);
        BOOST_CHECK_EQUAL(57843769575230720, resl);

        const char* p = test;
        get_be(p, resb);
        BOOST_CHECK_EQUAL(72623859790382856, resb);
        BOOST_CHECK_EQUAL(uint64_t(test+8), uint64_t(p));
        p = test;
        get_le(p, resl);
        BOOST_CHECK_EQUAL(57843769575230720, resl);
        BOOST_CHECK_EQUAL(uint64_t(test+8), uint64_t(p));
    }
    {
        char test[9];
        test[8] = '\0';
        store_be(test, 72623859790382856);
        BOOST_CHECK_EQUAL("\x1\x2\x3\x4\x5\x6\x7\x8", test);
        store_le(test, 57843769575230720);
        BOOST_CHECK_EQUAL("\x1\x2\x3\x4\x5\x6\x7\x8", test);
    }
}

} // namespace utxx
