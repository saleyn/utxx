//----------------------------------------------------------------------------
/// \file  cpu.hpp
//----------------------------------------------------------------------------
/// \brief This file contains some CPU specific informational functions.
/// Most of the code in this file is C++ implementation of some C code found
/// in:
///   http://software.intel.com/en-us/articles/optimal-performance-on-multithreaded-software-with-intel-tools/
///   http://software.intel.com/en-us/articles/intel-64-architecture-processor-topology-enumeration/
/// copyrighted by Gail Lyons
///
/// Reference: 
///   Memory part 2: CPU caches              (http://lwn.net/Articles/252125)
///   Memory part 5: What programmers can do (http://lwn.net/Articles/255364)
//----------------------------------------------------------------------------
// Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>
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

#ifndef _UTIL_CPU_x86_HPP_
#define _UTIL_CPU_x86_HPP_

#include <unistd.h>

namespace util {
namespace detail {

// This macro returns the cpu id data using 1 input in eax.
template <typename T, typename U>
inline void static cpuid(T in, U& a, U& b, U& c, U& d) {
//#define cpuid( in, a, b, c, d )                 
    __asm__ __volatile__ (
#if defined(__i386__) && defined(__PIC__)
        "movl %%ebx, %%edi;"
        "cpuid;"
        "xchgl %%ebx, %%edi;"
        : "=a" (a), "=D" (b)
#else
        "cpuid"
        : "=a" (a), "=b" (b)
#endif
        , "=c" (c), "=d" (d)
        : "a" (in) )
    ;
}

// Calls cpuid with 2 inputs, eax and ecx.
template <typename T, typename U>
inline void static cpuid(T in1, T in2, U& a, U& b, U& c, U& d) {
//#define cpuid2( in1, in2, a, b, c, d )
    __asm__ __volatile__ (
#if defined(__i386__) && defined(__PIC__)
        "movl %%ebx, %%edi;"
        "cpuid;"
        "xchgl %%ebx, %%edi;"
        : "=a" (a), "=D" (b)
#else
        "cpuid"
        : "=a" (a), "=b" (b)
#endif
        , "=c" (c), "=d" (d)
        : "a" (in1), "c" (in2) )
    ;
}

/// Returns true if this system is running Genuine Intel hardware.
static bool is_intel(void) {
    unsigned int a,b,c,d;

    static const unsigned int venB = ('u' << 24) | ('n' << 16) | ('e' << 8) | 'G';
    static const unsigned int venD = ('I' << 24) | ('e' << 16) | ('n' << 8) | 'i';
    static const unsigned int venC = ('l' << 24) | ('e' << 16) | ('t' << 8) | 'n';

    cpuid( 0, a, b, c, d );

    return b == venB && d == venD && c == venC;
}

/// Returns the maximum input value for the cpuid instruction
/// supported by the chip.
static int get_max_input_value() {
    unsigned int a,b,c,d;
    cpuid( 0, a, b, c, d );
    return a;
}

// Returns non-zero if hardware multithreading is supported.
// EDX[28] - Bit 28 set indicates multithreading is supported in hardware.
static bool mt_supported(void) {
    static const unsigned int MT_BIT = 0x10000000;
    unsigned int a,b,c,d;
    if ( is_intel() ) {
        int max = get_max_input_value();
        if ( max >= 1 ) {
            // Get the chip information.
            cpuid( 1, a, b, c, d );
            //Indicate if MT is supported by the hardware
            return ( d & MT_BIT );
        }
    }
    return false;
}

// Returns the number of logical processors per processors package.
// EBX[23:16] indicates number of logical processors per package
inline int logical_processors_per_package(void) {
    static const unsigned int NUM_LOGICAL_BITS = 0x00FF0000;
    unsigned int a, b, c, d;
    if ( !mt_supported() )
        return 1;
    cpuid( 1, a, b, c, d );
    return ( b & NUM_LOGICAL_BITS ) >> 16;
}

// Returns the number of cores per processor pack.
// EAX[31:26] - Bit 26 thru 31 contains the cores per processor pack -1.
inline int cores_per_proc_pak(void) {
    static const unsigned int CORES_PER_PROCPAK = 0xFC000000;
    unsigned int a,b,c,d;
    int num;

    if ( get_max_input_value() < 4 )
        num = 1;
    else {
        cpuid (4, 0, a, b, c, d );
        num = (( a & CORES_PER_PROCPAK ) >> 26 ) + 1;
    }
    return num;
}

/// Return the Advanced Programmable Interface Controller (APIC) ID.
// EBX[31:24] Bits 24-31 contains the 8-bit initial APIC ID for the
// processor this code is running on.
static unsigned int apic_id () {
    static const unsigned int INITIAL_APIC_ID_BITS = 0xFF000000;
    unsigned int a,b,c,d;
    cpuid( 1, a, b, c, d );
    return (b & INITIAL_APIC_ID_BITS) >> 24;
}

inline unsigned int cpu_count() {
    static unsigned int count = sysconf(_SC_NPROCESSORS_CONF);  // _SC_NPROCESSORS_ONLN
	return count;
}

inline unsigned int page_size() {
    static unsigned int size = sysconf(_SC_PAGESIZE);  // _SC_PAGE_SIZE
	return size;
}

inline unsigned int level1_cache_size() {
    static unsigned int sz = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
	return sz;
}

} // namespace detail
} // namespace util

#endif // _UTIL_CPU_x86_HPP_
