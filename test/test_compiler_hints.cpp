//----------------------------------------------------------------------------
/// \file   test_compiler_hints.cpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Test cases for compiler_hints macros.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-09-31
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

#include <utxx/test_helper.hpp>
#include <utxx/compiler_hints.hpp>
#include <iostream>

TEST_CASE( test_compiler_hints )
{
    CHECK(LIKELY(1));
    CHECK(UNLIKELY(1));
    CHECK(utxx::likely(1));
    CHECK(utxx::unlikely(1));
    bool res;
    res = UTXX_CHECK(1, true); CHECK(res);
    CHECK( UTXX_CHECK(0, true));
    CHECK(!UTXX_CHECK(1, false));
    CHECK(!UTXX_CHECK(0, false));
}