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