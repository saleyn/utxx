//----------------------------------------------------------------------------
/// \file  atomic.cpp
//----------------------------------------------------------------------------
/// \brief Atomic primitives are C++ implementation with embedded assembly
/// mostly inspired by linux kernel headers.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-16
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in different open-source projects.

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

#ifndef _UTIL_ATOMIC_HPP_
#define _UTIL_ATOMIC_HPP_

#include <boost/mpl/if.hpp>
#include <boost/mpl/string.hpp>
#include <util/meta.hpp>
#include <util/bits.hpp>

#if __SIZEOF_LONG__ == 8
#   define UTIL_QorL_SUFFIX "q"
#else
#   define UTIL_QorL_SUFFIX "l"
#endif

namespace util {
namespace atomic {

    namespace detail {
        template<int N, typename T> bool cas(
            volatile void* ptr, T vold, T vnew);

        template<> __inline__ bool cas<4, unsigned int>(
            volatile void* ptr, unsigned int vold, unsigned int vnew)
        {
            unsigned int prev;
            __asm__ __volatile__(
                    "lock cmpxchgl %1,%2"
                    : "=a"(prev)
                    : "r"(vnew), "m"(*(unsigned int*)ptr), "0"(vold)
                    : "memory"
                    );
            return prev == vold;
        }

        template<> __inline__ bool cas<8, unsigned long>(
            volatile void* ptr, unsigned long vold, unsigned long vnew)
        {
            unsigned long prev;
            __asm__ __volatile__(
                    "lock cmpxchgq %1,%2"
                    : "=a"(prev)
                    : "r"(vnew), "m"(*(unsigned long*)ptr), "0"(vold)
                    : "memory"
                    );
            return prev == vold;
        }

        template<int N, typename T> unsigned long cmpxchg(
            volatile void* ptr, T vold, T vnew);

        template<> __inline__ unsigned long cmpxchg<4, unsigned int>(
            volatile void* ptr, unsigned int vold, unsigned int vnew)
        {
            unsigned int prev;
            __asm__ __volatile__(
                  "lock cmpxchgl %1,%2"
                : "=a"(prev)
                : "r"(vnew), "m"(*(unsigned int*)ptr), "0"(vold)
                : "memory"
            );
            return prev;
        }

        template<> __inline__ unsigned long cmpxchg<8, unsigned long>(
            volatile void* ptr, unsigned long vold, unsigned long vnew)
        {
            unsigned long prev;
            __asm__ __volatile__(
                  "lock cmpxchgq %1,%2"
                : "=a"(prev)
                : "r"(vnew), "m"(*(unsigned long*)ptr), "0"(vold)
                : "memory"
            );
            return prev;
        }

        template<int N, typename T> bool dcas(
            volatile void* ptr, T* vold, T* vnew);

        template<> __inline__ bool dcas<8, unsigned int>(
            volatile void* ptr, unsigned int* vold, unsigned int* vnew)
        {
            bool r;
            __asm__ __volatile__(
                  "lock cmpxchg8b %0 \n"
                  "setz %1"
                : "+m" (*(unsigned int*)ptr), "=q" (r), "+a" (*vold),
                  "+d" (*(vold+1))
                : "b" (*vnew), "c" (*(vnew+1))
                : "cc");
            return r;
        }

        template<> __inline__ bool dcas<16, unsigned long>(
            volatile void* ptr, unsigned long* vold, unsigned long* vnew)
        {
            bool r;
            __asm__ __volatile__(
                  "lock cmpxchg16b %0 \n"
                  "setz %1"
                : "+m" (*(unsigned long*)ptr), "=q" (r), "+a" (*vold),
                  "+d" (*(vold+1))
                : "b" (*vnew), "c" (*(vnew+1))
                : "cc");
            return r;
        }

    }

template <typename T, typename I>
bool cas(volatile T* p, I vold, I vnew) {
    typedef typename 
        boost::mpl::if_c<sizeof(T) == 4, unsigned int, unsigned long>::type int_t;
    return detail::cas<sizeof(T), int_t>(p, (int_t)vold, (int_t)vnew);
}

template <typename T, typename I>
unsigned long cmpxchg(volatile T* p, I vold, I vnew) {
    typedef typename
        boost::mpl::if_c<sizeof(T) == 4, unsigned int, unsigned long>::type int_t;
    return detail::cmpxchg<sizeof(T), int_t>(p, (int_t)vold, (int_t)vnew);
}

template <typename T, typename I>
bool dcas(volatile T* p, I vold, I vnew) {
    typedef typename
        boost::mpl::if_c<sizeof(T) == 8, unsigned int, unsigned long>::type int_t;
    return detail::dcas<sizeof(T), int_t>(p, (int_t*)&vold, (int_t*)&vnew);
}

static inline long add(volatile long* v, long inc)
{
    long ret;
    __asm__ __volatile__ (
        "lock xadd" UTIL_QorL_SUFFIX " %0,(%1)"
        : "=r" (ret)
        : "r" (v), "0" (inc)
        : "memory"
    );
    return ret;
}

static inline void inc(volatile long* c)
{
    __asm__ __volatile__("lock; inc" UTIL_QorL_SUFFIX " %0" : "=m" (*c) : "m" (*c));
}

static inline void dec(volatile long* c)
{
    __asm__ __volatile__("lock; dec" UTIL_QorL_SUFFIX " %0" : "=m" (*c) : "m" (*c));
}

static inline long xchg(volatile long* target, long value)
{
    long ret;
    __asm__ __volatile__ (
          "lock xchg" UTIL_QorL_SUFFIX " %2,(%1)"
        : "=r" (ret)
        : "r" (target), "0" (value)
        : "memory"
    );
    return ret;
}

/*
static inline unsigned long bit_scan_forward(unsigned long v)
{
    unsigned long r;
    __asm__ __volatile__( "bsf" UTIL_QorL_SUFFIX " %1, %0": "=r"(r): "rm"(v) );
    return r;
}

static inline unsigned long bit_scan_reverse(unsigned long v)
{
    unsigned long r;
    __asm__ __volatile__( "bsr" UTIL_QorL_SUFFIX " %1, %0": "=r"(r): "rm"(v) );
    return r;
}
*/

static inline void memory_barrier() {
    __asm__ __volatile__ ("" ::: "memory");
}

/// Atomically set a given bit in a location pointed to by \a addr
static inline void set_bit(int n, volatile unsigned long* addr) {
    if (bits::detail::is_immediate(n)) {
        asm volatile("lock; orb %1,%0"
            : UTIL_CONST_MASK_ADDR(n, addr)
            : "iq" ((uint8_t)bits::detail::const_mask(n))
            : "memory");
    } else {
        asm volatile("lock; bts %1,%0"
            : UTIL_ADDR(addr) : "Ir" (n) : "memory");
    }
}

/// Atomically clear a given bit in a location pointed to by \a addr
static inline void clear_bit(int n, volatile unsigned long* addr) {
    /*
    if (bits::detail::is_immediate(n)) {
        asm volatile("lock; andb %1,%0"
            : UTIL_CONST_MASK_ADDR(n, addr)
            : "iq" ((uint8_t)bits::detail::const_mask(n)));
    } else {
        asm volatile("lock; btr %1,%0"
            : UTIL_ADDR(addr) : "Ir" (n));
    }
    */
    asm volatile("lock; btr %1,%0" : UTIL_ADDR(addr) : "Ir" (n));
}

/// Atomically toggle a bit in memory
static inline void change_bit(int n, volatile unsigned long* addr) {
    if (bits::detail::is_immediate(n)) {
        asm volatile("lock; xorb %1,%0"
            : UTIL_CONST_MASK_ADDR(n, addr)
            : "iq" ((uint8_t)bits::detail::const_mask(n)));
    } else {
        asm volatile("lock; btc %1,%0"
            : UTIL_ADDR(addr) : "Ir" (n));
    }
}

/// Atomically set a bit and return its old value
static inline int test_and_set_bit(int n, volatile unsigned long *addr) {
    int oldbit;
    asm volatile("lock; bts %2,%1\n\t"
        "sbb %0,%0" : "=r" (oldbit), UTIL_ADDR(addr) : "Ir" (n) : "memory");
    return oldbit;
}

/// Atomically clear a bit and return its old value
static inline int test_and_clear_bit(int n, volatile unsigned long *addr) {
    int oldbit;
    asm volatile("lock; btr %2,%1\n\t"
        "sbb %0,%0" : "=r" (oldbit), UTIL_ADDR(addr) : "Ir" (n) : "memory");
    return oldbit;
}

/// Atomically toggle a bit and return its old value
static inline int test_and_change_bit(int n, volatile unsigned long *addr) {
    int oldbit;
    asm volatile("lock; btc %2,%1\n\t"
        "sbb %0,%0" : "=r" (oldbit), UTIL_ADDR(addr) : "Ir" (n) : "memory");
    return oldbit;
}

/// Find first set bit in word.
/// Undefined if no set bit exists, so code should check against 0 first.
static inline unsigned long bit_scan_forward(unsigned long v) {
	return bits::bit_scan_forward(v);
}

/// Find last set bit in word.
/// Undefined if no set bit exists, so code should check against 0 first.
static inline unsigned long bit_scan_reverse(unsigned long v) {
	return bits::bit_scan_reverse(v);
}

} // namespace atomic
} // namespace util

#endif // _UTIL_ATOMIC_HPP_
