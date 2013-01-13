//----------------------------------------------------------------------------
/// \file  test_helper.hpp
//----------------------------------------------------------------------------
/// \brief Unit test helping routines.
//----------------------------------------------------------------------------
// Author:  Serge Aleynikov
// Created: 2010-01-06
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

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

#ifndef _UTXX_TEST_HELPER_HPP_
#define _UTXX_TEST_HELPER_HPP_

#include <boost/test/unit_test.hpp>

#define BOOST_CURRENT_TEST_NAME \
  boost::unit_test::framework::current_test_case().p_name->c_str()

#endif // _UTXX_TEST_HELPER_HPP_

