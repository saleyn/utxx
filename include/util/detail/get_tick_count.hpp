//----------------------------------------------------------------------------
/// \file  get_tick_count.hpp
//----------------------------------------------------------------------------
/// \brief This file contains implementation of a function that
/// reads CPU ticks.
//----------------------------------------------------------------------------
// Copyright (c) 1993-2009 Douglas C. Schmidt
// Author:  Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-16
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file may be included in various open-source projects.

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

#ifndef _UTIL_GET_TICK_COUNT_HPP_
#define _UTIL_GET_TICK_COUNT_HPP_

namespace util {

    typedef unsigned long long hrtime_t;

    typedef union {
        struct {
            unsigned int hi;
            unsigned int lo;
        } l;
        hrtime_t ll;
    } tick_t;

namespace detail {

    // Tick count code for PowerPC (Apple G3, G4, G5)
    #ifdef _CPU_POWERPC
    static inline hrtime_t get_tick_count() {
        tick_t t;
        __asm__ __volatile__ ( 
        "1:\n\t" 
            "mftbu %0\n\t" 
            "mftb %1\n\t" 
            "mftbu r6\n\t" 
            "cmpw r6,%0\n\t" 
            "bne 1b" 
            : "=r" (t.l.hi), "=r" (t.l.lo) 
            : 
            : "r6");
        return t.ll;
    }
    #elif (defined(_CPU_IA32) || defined(_CPU_IA64) || defined(__x86_64))
    /* Tick count code for IA-32 architecture (Intel and AMD x86, and 64-bit 
     * versions). */
    static inline hrtime_t get_tick_count() {
        tick_t t;
        __asm__ __volatile__ ("rdtsc" : "=a" (t.l.hi), "=d" (t.l.lo));
        return t.ll;
    }
    #elif defined(_CPU_SPARC_V8PLUS)
    // Tick count code for SPARC architecture
    static inline hrtime_t get_tick_count() {
        tick_t t;
        __asm__ __volatile__ ("rd %%tick, %0" : "=r" (t->ll));
        return t.ll;
    }
    #elif defined(_WIN32)
    static inline hrtime_t get_tick_count() {
        tick_t t;
        // Use RDTSC instruction to get clocks count
        __asm push EAX
        __asm push EDX
        __asm __emit 0fh __asm __emit 031h // RDTSC
        __asm mov t.lo, EAX
        __asm mov t.hi, EDX
        __asm pop EDX
        __asm pop EAX
        return t.ll;
    }
    #endif

} // namespace detail
} // namespace util

#endif // _UTIL_GET_TICK_COUNT
