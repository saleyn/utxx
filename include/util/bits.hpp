//----------------------------------------------------------------------------
/// \file  bits.cpp
//----------------------------------------------------------------------------
/// \brief Functions for manipulating bits.
//----------------------------------------------------------------------------
// Copyright (c) 1992 Linus Torvalds
// Author: Serge Aleynikov <saleyn@gmail.com>
// Code inherited from linux kernel:  include/asm/bitops.h
// Created: 2011-1-16
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

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

#ifndef _UTIL_BITS_HPP_
#define _UTIL_BITS_HPP_

#include <boost/cstdint.hpp>

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 1)
#   define UTIL_ADDR(x) "=m" (*(volatile long *) (x))
#else
#   define UTIL_ADDR(x) "+m" (*(volatile long *) (x))
#endif
#define UTIL_CONST_MASK_ADDR(n, addr) UTIL_ADDR((char*)(addr) + ((n)>>3))

#define FOREACH_SET_BIT(bit, addr) \
	for (unsigned int (bit) = util::bits::bit_scan_forward((addr)); \
	     (bit) < 8*sizeof(addr); \
	     (bit) = util::bits::bit_scan_next((addr), (bit) + 1))


namespace util {
namespace bits {

namespace detail {
    inline int is_immediate(long n) { return __builtin_constant_p(n); }
    inline long const_mask(long n)  { return (1 << ((n) & 7)); }
} // namespace detail

static inline int bitcount(uint32_t n) {
#ifdef __GNUC__
    return __builtin_popcount(n);
#else
    // From MIT HAKMEM source
    register uint32_t tmp = n
        - ((n >> 1) & 033333333333)
        - ((n >> 2) & 011111111111);
    return ((tmp + (tmp >> 3)) & 030707070707) % 63;
#endif
}

static inline int bitcount(uint64_t n) {
#ifdef __GNUC__
    return __builtin_popcount(n >> 32) + __builtin_popcount(n & 0xFFFFFFFF);
#else
    register uint64_t tmp = n
        - ((n >> 1) & 0x7777777777777777)
        - ((n >> 2) & 0x3333333333333333)
        - ((n >> 3) & 0x1111111111111111);
    return ((tmp + (tmp >> 4)) & 0x0F0F0F0F0F0F0F0F) % 255;
#endif
}

/// Set a given bit in a location pointed to by \a addr
static inline void set_bit(int n, volatile unsigned long* addr) {
	asm volatile("bts %1,%0" : UTIL_ADDR(addr) : "Ir" (n) : "memory");
}

static inline void clear_bit(int n, volatile unsigned long* addr) {
	asm volatile("btr %1,%0" : UTIL_ADDR(addr) : "Ir" (n));
}

/// Toggle a bit in memory
static inline void change_bit(int n, volatile unsigned long* addr) {
	asm volatile("btc %1,%0" : UTIL_ADDR(addr) : "Ir" (n));
}

/// Set a bit and return its old value
static inline int test_and_set_bit(int n, volatile unsigned long *addr) {
	int oldbit;
	asm("bts %2,%1\n\t" "sbb %0,%0" : "=r" (oldbit), UTIL_ADDR(addr) : "Ir" (n));
	return oldbit;
}

/// Clear a bit and return its old value
static inline int test_and_clear_bit(int n, volatile unsigned long *addr) {
	int oldbit;
	asm volatile("btr %2,%1\n\t" "sbb %0,%0" : "=r" (oldbit), UTIL_ADDR(addr) : "Ir" (n));
	return oldbit;
}

/// Change a bit and return its old value
static inline int test_and_change_bit(int n, volatile unsigned long *addr) {
	int oldbit;
	asm volatile("btc %2,%1\n\t" "sbb %0,%0" : "=r" (oldbit), UTIL_ADDR(addr) : "Ir" (n) : "memory");
	return oldbit;
}

/// Find first set bit in word.
/// Undefined if no set bit exists, so code should check against 0 first.
static inline unsigned long bit_scan_forward(unsigned long v) {
	asm("bsf %1,%0" : "=r" (v) : "rm" (v));
	return v;
}

/// Find next set bit in word counting from n'th bit.
/// Undefined if no set bit exists, so code should check against 0 first.
static inline unsigned long bit_scan_next(unsigned long v, unsigned int n) {
    static const unsigned int s_end = 8*sizeof(long);
    unsigned long val = v >> ++n;
    return val && n < s_end ? bit_scan_forward(val)+n : s_end;
}

/// Find last set bit in word.
/// Undefined if no set bit exists, so code should check against 0 first.
static inline unsigned long bit_scan_reverse(unsigned long v) {
	asm("bsr %1,%0" : "=r" (v) : "rm" (v));
	return v;
}

/// Find first zero bit in word.
/// Undefined if no zero bit exists, so code should check against ~0UL first.
static inline unsigned long find_first_zero(unsigned long v) {
    asm ("bsf %1,%0" : "=r" (v) : "r" (~v));
    return v;
}

} // namespace bits
} // namespace util

#endif // _UTIL_BITS_HPP_
