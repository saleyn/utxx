//----------------------------------------------------------------------------
/// \file  hashmap.hpp
//----------------------------------------------------------------------------
/// \brief Abstraction of boost and std unordered_map.
//----------------------------------------------------------------------------
// Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-09-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>

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

#ifndef _UTXX_HASHMAP_HPP_
#define _UTXX_HASHMAP_HPP_

#ifdef __GXX_EXPERIMENTAL_CXX0X__
#  include <unordered_map>
#else
#  include <boost/unordered_map.hpp>
#endif

namespace utxx {
namespace detail {

    #ifdef __GXX_EXPERIMENTAL_CXX0X__
    namespace src = std;
    #else
    namespace src = boost;
    #endif

    /// Hash map class
    template <typename K, typename V, typename Hash = src::hash<K> >
    struct basic_hash_map : public src::unordered_map<K, V, Hash>
    {
        basic_hash_map() {}
        basic_hash_map(size_t n): src::unordered_map<K,V,Hash>(n)
        {}
        basic_hash_map(size_t n, const Hash& h): src::unordered_map<K,V,Hash>(n, h)
        {}
    };

} // namespace detail
} // namespace utxx

#endif // _UTXX_HASHMAP_HPP_
