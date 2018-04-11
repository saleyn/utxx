//----------------------------------------------------------------------------
/// \file  test_base64.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for classes in the string.hpp.
//----------------------------------------------------------------------------
// Copyright (c) 2018 Serge Aleynikov <saleyn@gmail.com>
// Created: 2018-04-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects

Copyright (C) 2018 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/base64.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace utxx;

// trim from end (in place)
static inline std::string rtrim(std::string s, char c = '=') {
    s.erase(std::find_if(s.rbegin(), s.rend(), [c](int ch) {
        return ch != c;
    }).base(), s.end());
    return s;
}

BOOST_AUTO_TEST_CASE( test_base64 )
{
	const std::string s_sources[] = {
        ""
      , "B"
      , "Ba"
      , "Bas"
      , "Base"
      , "Base6"
      , "Base64"
      , "Base64."
      , "Base64+"
      , "Base64+/"
      , "Base64+/ "
	};
	const std::string s_encoded[] = {
        ""
      , "Qg=="
      , "QmE="
      , "QmFz"
      , "QmFzZQ=="
      , "QmFzZTY="
      , "QmFzZTY0"
      , "QmFzZTY0Lg=="
      , "QmFzZTY0Kw=="
      , "QmFzZTY0Ky8="
      , "QmFzZTY0Ky8g"
	};

	for(auto i=0u; i < std::extent<decltype(s_encoded)>::value; ++i) {
		auto enc = base64::encode(s_sources[i]);
		BOOST_CHECK_EQUAL(s_encoded[i], enc);
        enc = base64::encode(s_sources[i], base64::STANDARD, false);
		BOOST_CHECK_EQUAL(rtrim(s_encoded[i]), enc);

        auto dec = base64::decode(s_encoded[i]);
        auto res = std::string(&*dec.begin(), dec.size());
		BOOST_CHECK_EQUAL(s_sources[i], res);

        dec = base64::decode(rtrim(s_encoded[i]));
        res = std::string(&*dec.begin(), dec.size());
		BOOST_CHECK_EQUAL(s_sources[i], res);
    }
}

