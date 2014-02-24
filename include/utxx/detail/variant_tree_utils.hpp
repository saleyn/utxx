//----------------------------------------------------------------------------
/// \file  variant_tree_utils.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Various utils for varian_tree parsing
//----------------------------------------------------------------------------
// Created: 2014-02-22
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

#ifndef _UTXX_VARIANT_TREE_UTILS_HPP_
#define _UTXX_VARIANT_TREE_UTILS_HPP_

#include <utxx/variant_tree.hpp>
#include <boost/property_tree/info_parser.hpp>

namespace utxx {

    namespace detail {

        template<class Ch>
        std::basic_string<Ch> create_escapes(const std::basic_string<Ch>& s) {
            return boost::property_tree::info_parser::create_escapes(s);
        }

        template<class Ch>
        bool is_simple_key(const std::basic_string<Ch> &key) {
            return boost::property_tree::info_parser::is_simple_key(key);
        }
    }

} // namespace utxx

#endif // _UTXX_VARIANT_TREE_UTILS_HPP_