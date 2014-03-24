//----------------------------------------------------------------------------
/// \file   variant_tree_path.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief This file contains a tree class that can hold variant values.
//----------------------------------------------------------------------------
// Created: 2010-07-10
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

#ifndef _UTXX_VARIANT_TREE_PATH_HPP_
#define _UTXX_VARIANT_TREE_PATH_HPP_

#include <boost/property_tree/string_path.hpp>
#include <utxx/variant_translator.hpp>

namespace utxx {

template <class Ch>
using basic_tree_path = boost::property_tree::string_path<
        std::basic_string<Ch>,
        boost::property_tree::id_translator<std::basic_string<Ch>>
      >;

typedef basic_tree_path<char> tree_path;


tree_path  operator/ (const tree_path& a, const std::string& s);
tree_path& operator/ (tree_path& a,       const std::string& s);
tree_path  operator/ (const std::string& a, const tree_path& s);

tree_path  operator/ (const tree_path& a,
    const std::pair<std::string,std::string>& a_option_with_value);

tree_path  operator/ (const tree_path& a,
    const std::pair<const char*,const char*>& a_option_with_value);

tree_path& operator/ (tree_path& a,
    const std::pair<std::string,std::string>& a_option_with_value);

tree_path& operator/ (tree_path& a,
    const std::pair<const char*,const char*>& a_option_with_value);

} // namespace utxx

#endif // _UTXX_VARIANT_TREE_PATH_HPP_

#include <utxx/variant_tree_path.ipp>