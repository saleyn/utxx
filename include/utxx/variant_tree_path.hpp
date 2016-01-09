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

/// Constructs a path in the form "Node[Data]"
tree_path make_tree_path_pair(const char*,const char*, char a_sep = '.');
/// Constructs a path in the form "Node[Data]"
tree_path make_tree_path_pair(const std::string&, const std::string&, char a_sep = '.');

template <class Ch>
basic_tree_path<Ch> operator/
(
    const basic_tree_path<Ch>& a,
    const basic_tree_path<Ch>& b
);

template <class Ch>
basic_tree_path<Ch> operator/
(
    const basic_tree_path<Ch>& a,
    const char* b
);

template <class Ch>
basic_tree_path<Ch> operator/
(
    const basic_tree_path<Ch>& a,
    const std::string& b
);

template <class Ch>
basic_tree_path<Ch>& operator/=
(
    basic_tree_path<Ch>& a,
    const char* b
);

template <class Ch>
basic_tree_path<Ch>& operator/=
(
    basic_tree_path<Ch>& a,
    const std::string& b
);

template <class Ch>
basic_tree_path<Ch>
make_tree_path(basic_tree_path<Ch>&& a_path) { return std::move(a_path); }

template <class Ch, class T, class... Args>
basic_tree_path<Ch>
make_tree_path(basic_tree_path<Ch>&& a_path1, T&& a_path2, Args&&... args) {
    a_path1 /= a_path2;
    return make_tree_path<Ch>(std::move(a_path1), std::forward<Args>(args)...);
}

template <char Sep, class Ch, class T, class... Args>
basic_tree_path<Ch>
make_tree_path(const std::basic_string<Ch>& a_path1, T&& a_path2, Args&&... args) {
    auto path = basic_tree_path<Ch>(a_path1, Sep);
    path /= a_path2;
    return make_tree_path<Ch>(std::move(path), std::forward<Args>(args)...);
}

template <char Sep, class Ch, class T, class U, class... Args>
basic_tree_path<Ch>
make_tree_path(const Ch* a_path1, U&& a_path2, Args&&... args) {
    auto path = basic_tree_path<Ch>(a_path1, Sep);
    path /= a_path2;
    return make_tree_path<Ch>(std::move(path), std::forward<Args>(args)...);
}

tree_path  operator/ (const tree_path& a, const std::string& s);
tree_path  operator/ (const std::string& a, const tree_path& s);

tree_path  operator/ (const tree_path& a,
    const std::pair<std::string,std::string>& a_option_with_value);

tree_path  operator/ (const tree_path& a,
    const std::pair<const char*,const char*>& a_option_with_value);

tree_path& operator/=(tree_path& a,
    const std::pair<std::string,std::string>& a_option_with_value);

tree_path& operator/=(tree_path& a,
    const std::pair<const char*,const char*>& a_option_with_value);

} // namespace utxx

#endif // _UTXX_VARIANT_TREE_PATH_HPP_

#include <utxx/variant_tree_path.ipp>
