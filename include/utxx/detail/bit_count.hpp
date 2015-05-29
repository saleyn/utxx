//----------------------------------------------------------------------------
/// \file  bit_count.hpp
//----------------------------------------------------------------------------
/// \brief This file contains implementation of a function that
/// counts number of bits in an unsigned integer.
//----------------------------------------------------------------------------
// Windows version of the algorithm is taken from open source MIT HAKMEM.
// Copiright (c) 2009 Serge Aleynikov <saleyn@gmail.com>
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

#ifndef _UTXX_BIT_COUNT_HPP_
#define _UTXX_BIT_COUNT_HPP_

#include <boost/cstdint.hpp>

namespace utxx {

    namespace detail {
        #ifdef __GNUC__
        static inline int bitcount32(uint32_t n) {
            return __builtin_popcount(n);
        }
        static inline int bitcount64(uint64_t n) {
            return __builtin_popcount(n >> 32)
                 + __builtin_popcount(n & 0xFFFFFFFF);
        }
        #elif defined(_WIN32) || defined(_WIN64)
        static inline int bitcount32(uint32_t n) {
            // From MIT HAKMEM source
            register uint32_t tmp = n
                - ((n >> 1) & 033333333333)
                - ((n >> 2) & 011111111111);
            return ((tmp + (tmp >> 3)) & 030707070707) % 63;
        }
        static inline int bitcount64(uint64_t n) {
            register uint64_t tmp = n
                - ((n >> 1) & 0x7777777777777777)
                - ((n >> 2) & 0x3333333333333333)
                - ((n >> 3) & 0x1111111111111111);
            return ((tmp + (tmp >> 4)) & 0x0F0F0F0F0F0F0F0F) % 255;
        }
        #else
        #  error Platform not supported!
        #endif

        template <typename T, int N>
        struct bitcount_helper;

        template <typename T>
        struct bitcount_helper<T, 4> {
            static int count(T n) { return bitcount32(n); }
        };

        template <typename T>
        struct bitcount_helper<T, 8> {
            static int count(T n) { return bitcount64(n); }
        };

    } // namespace detail

/**
 * Count number of "on" bits in an integer.  Works for 4 and 8 byte
 * unsigned integers.
 */
template <typename T>
int bitcount(T n) {
    return detail::bitcount_helper<T, sizeof(T)>::count(n);
}

} // namespace utxx

#endif // _UTXX_GET_TICK_COUNT
