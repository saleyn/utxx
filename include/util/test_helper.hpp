/**
 * Unit test helping routines.
 */
/*-----------------------------------------------------------------------------
 * Copyright (c) 2009 Serge Aleynikov <serge@aleynikov.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *-----------------------------------------------------------------------------
 * Author:  Serge Aleynikov
 * Created: 2010-01-06
 * $Id$
 */

#ifndef _UTIL_TEST_HELPER_HPP_
#define _UTIL_TEST_HELPER_HPP_

#include <boost/test/unit_test.hpp>

#define BOOST_CURRENT_TEST_NAME boost::unit_test::framework::current_test_case().p_name->c_str()

#endif // _UTIL_TEST_HELPER_HPP_

