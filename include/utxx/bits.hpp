//----------------------------------------------------------------------------
/// \file   bits.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Functions for manipulating bits.
//----------------------------------------------------------------------------
// Copyright (c) 1992 Linus Torvalds
// Code inherited from linux kernel:  include/asm/bitops.h
// Created: 2011-1-16
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2011 Serge Aleynikov <saleyn@gmail.com>

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
#include <utxx/detail/bit_count.hpp>

#define FOREACH_SET_BIT(bit, addr) \
    for (unsigned int (bit) = utxx::bits::bit_scan_forward((addr)); \
         (bit) < 8*sizeof(addr); \
         (bit) = utxx::bits::bit_scan_next((addr), (bit) + 1))


namespace utxx {
namespace bits {

/// Set a given bit in a location pointed to by \a addr
static inline void set_bit(int n, volatile unsigned long* addr) {
    __atomic_fetch_or(addr, 1UL << n, __ATOMIC_SEQ_CST);
}

static inline void clear_bit(int n, volatile unsigned long* addr) {
    __atomic_fetch_and(addr, ~(1UL << n), __ATOMIC_SEQ_CST);
}

/// Toggle a bit in memory
static inline void change_bit(int n, volatile unsigned long* addr) {
    __atomic_fetch_xor(addr, 1UL << n, __ATOMIC_SEQ_CST);
}

/// Set a bit and return its old value
static inline int test_and_set_bit(int n, volatile unsigned long* addr) {
    unsigned long mask = 1UL << n;
    unsigned long old  = __atomic_fetch_or(addr, mask, __ATOMIC_SEQ_CST);
    return (old & mask) != 0;
}

/// Clear a bit and return its old value
static inline int test_and_clear_bit(int n, volatile unsigned long* addr) {
    unsigned long mask = 1UL << n;
    unsigned long old  = __atomic_fetch_and(addr, ~mask, __ATOMIC_SEQ_CST);
    return (old & mask) != 0;
}

/// Change a bit and return its old value
static inline int test_and_change_bit(int n, volatile unsigned long* addr) {
    unsigned long mask = 1UL << n;
    unsigned long old  = __atomic_fetch_xor(addr, mask, __ATOMIC_SEQ_CST);
    return (old & mask) != 0;
}

/// Find first set bit in word (index of least significant set bit).
/// Undefined if no set bit exists, so code should check against 0 first.
static inline unsigned long bit_scan_forward(unsigned long v) {
    return __builtin_ctzl(v);
}

/// Find next set bit in word counting from n'th bit.
/// Undefined if no set bit exists, so code should check against 0 first.
static inline unsigned long bit_scan_next(unsigned long v, unsigned int n) {
    static const unsigned int s_end = 8*sizeof(long);
    unsigned long val = v >> ++n;
    return val && n < s_end ? bit_scan_forward(val)+n : s_end;
}

/// Find last set bit in word (index of most significant set bit).
/// Undefined if no set bit exists, so code should check against 0 first.
static inline unsigned long bit_scan_reverse(unsigned long v) {
    return 8*sizeof(long) - 1 - __builtin_clzl(v);
}

/// Find first zero bit in word.
/// Undefined if no zero bit exists, so code should check against ~0UL first.
static inline unsigned long find_first_zero(unsigned long v) {
    return __builtin_ctzl(~v);
}

} // namespace bits
} // namespace utxx
