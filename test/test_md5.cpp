//----------------------------------------------------------------------------
/// \file  test_md5.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the md5.hpp.
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
#include <utxx/md5.hpp>

using namespace utxx;

#if BOOST_VERSION >= 106600

BOOST_AUTO_TEST_CASE( test_md5 )
{
    auto res = md5("abc");
    BOOST_CHECK_EQUAL("900150983cd24fb0d6963f7d28e17f72", res);

    auto res1 = sha1("abc");
    BOOST_CHECK_EQUAL("a9993e364706816aba3e25717850c26c9cd0d89d", res1);
}

#endif
