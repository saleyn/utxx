//----------------------------------------------------------------------------
/// \file  bit_count.hpp
//----------------------------------------------------------------------------
/// \brief This file contains implementation of a function that
/// counts number of bits in an unsigned integer.
//----------------------------------------------------------------------------
// Copyright (c) 2009 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-17
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

#pragma once

#include <cstdint>

namespace utxx {

/**
 * Count number of "on" bits in an integer.  Works for 4 and 8 byte
 * unsigned integers.
 */
static inline int bitcount(uint32_t n) { return __builtin_popcount(n);   }
static inline int bitcount(uint64_t n) { return __builtin_popcountll(n); }

} // namespace utxx
