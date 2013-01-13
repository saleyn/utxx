//----------------------------------------------------------------------------
/// \file  test_pidfile.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for the pid_file class.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-02-20
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
#include <utxx/pidfile.hpp>
#include <utxx/path.hpp>
#include <utxx/verbosity.hpp>
#include <fstream>

using namespace utxx;

BOOST_AUTO_TEST_CASE( test_pid_file )
{
    std::string filename("/tmp/test_pidfile.pid");
    {
        utxx::pid_file l_pidfile(filename.c_str());

        std::string expected_pid = utxx::int_to_string(::getpid());
        std::string got_pid;

        std::ifstream fs(filename.c_str());
        fs >> got_pid;

        BOOST_REQUIRE_EQUAL(expected_pid, got_pid);
    }
}
