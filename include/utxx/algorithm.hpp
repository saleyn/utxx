//------------------------------------------------------------------------------
/// \file   algorithm.hpp
/// \author Serge Aleynikov
//------------------------------------------------------------------------------
/// \brief This file defines various container-based algorithms
//------------------------------------------------------------------------------
// Copyright (c) 2020 Serge Aleynikov <saleyn@gmail.com>
// Created: 2020-12-10
//------------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2016 Serge Aleynikov <saleyn@gmail.com>

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
#pragma once

#include <iterator>

namespace utxx {

/// Search the container for an item which is closest (less/greater/equal) to the given value
/// @param s    - the container to search in
/// @param val  - the value to check
/// @code
///   std::set<int> set{ 3, 4, 8 };
///   auto x = find_closest(set, 1);  // Returns 3
///   auto y = find_closest(set, 4);  // Returns 4
///   auto z = find_closest(set, 5);  // Returns 4
/// @endcode
template <typename Set>
typename Set::const_iterator
find_closest(Set const& s, typename Set::value_type const& val)
{
    if (s.empty())
        return s.end();

    auto a = s.begin(), b = s.end(), it = s.lower_bound(val);

    if (it == b) {
        if (it != a) --it;
        return it;
    }

    auto nt = std::next(it);

    if (nt == b) return it;
    return val - *it < *nt - val ? it : nt;
}

/// Search the container for an item which is equal to or greater than the given value
/// @param s    - the container to search in
/// @param val  - the value to check
/// @code
///   std::set<int> set{ 3, 4, 8 };
///   auto x = find_ge(set, 1);  // Returns 3
///   auto y = find_ge(set, 4);  // Returns 4
///   auto z = find_ge(set, 5);  // Returns 8
/// @endcode
template <typename Set>
typename Set::const_iterator
find_ge(Set const& s, typename Set::value_type const& val)
{
    if (s.empty())
        return s.end();

    auto it = s.upper_bound(val);

    if (it == s.begin())
        return it;

    auto pr = std::prev(it);

    return *pr == val ? pr : it;
}

} // namespace utxx
