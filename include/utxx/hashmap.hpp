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

#include <stdint.h>
#include <utxx/compiler_hints.hpp>

namespace utxx {
namespace detail {

    #if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
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

    /// Hash functor implemention hsieh hashing algorithm
    // See http://www.azillionmonkeys.com/qed/hash.html
    // Copyright 2004-2008 (c) by Paul Hsieh
    inline size_t hsieh_hash(const char* data, int len) {
        uint32_t hash = len, tmp;
        int rem;

        if (unlikely(len <= 0 || data == NULL)) return 0;

        rem = len & 3;
        len >>= 2;

        /* Main loop */
        for (; len > 0; len--) {
            hash  += *(const uint16_t *)data;
            tmp    = (*(const uint16_t *)(data+2) << 11) ^ hash;
            hash   = (hash << 16) ^ tmp;
            data  += 2*sizeof (uint16_t);
            hash  += hash >> 11;
        }

        /* Handle end cases */
        switch (rem) {
            case 3: hash += *(const uint16_t *)data;
                    hash ^= hash << 16;
                    hash ^= data[sizeof (uint16_t)] << 18;
                    hash += hash >> 11;
                    break;
            case 2: hash += *(const uint16_t *)data;
                    hash ^= hash << 11;
                    hash += hash >> 17;
                    break;
            case 1: hash += *data;
                    hash ^= hash << 10;
                    hash += hash >> 1;
        }

        /* Force "avalanching" of final 127 bits */
        hash ^= hash << 3;
        hash += hash >> 5;
        hash ^= hash << 4;
        hash += hash >> 17;
        hash ^= hash << 25;
        hash += hash >> 6;

        return hash;
    }

    template <typename T>
    struct hash_fun;

    template <>
    struct hash_fun<const char*> {
        size_t operator()(const char* data) const {
            return hsieh_hash(data, strlen(data));
        }
    };

    template <>
    struct hash_fun<std::string> {
        size_t operator()(const std::string& data) const {
            return hsieh_hash(data.c_str(), data.size());
        }
    };

    inline uint32_t crapwow(const char* key, uint32_t len, uint32_t seed=0) {
#if !defined(__LP64__) && !defined(_MSC_VER) && ( defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) )
        uint32_t hash;
        asm(
            "leal 0x5052acdb(%%ecx,%%esi), %%esi\n"
            "movl %%ecx, %%ebx\n"
            "cmpl $8, %%ecx\n"
            "jb DW%=\n"
        "QW%=:\n"
            "movl $0x5052acdb, %%eax\n"
            "mull (%%edi)\n"
            "addl $-8, %%ecx\n"
            "xorl %%eax, %%ebx\n"
            "xorl %%edx, %%esi\n"
            "movl $0x57559429, %%eax\n"
            "mull 4(%%edi)\n"
            "xorl %%eax, %%esi\n"
            "xorl %%edx, %%ebx\n"
            "addl $8, %%edi\n"
            "cmpl $8, %%ecx\n"
            "jae QW%=\n"
        "DW%=:\n"
            "cmpl $4, %%ecx\n"
            "jb B%=\n"
            "movl $0x5052acdb, %%eax\n"
            "mull (%%edi)\n"
            "addl $4, %%edi\n"
            "xorl %%eax, %%ebx\n"
            "addl $-4, %%ecx\n"
            "xorl %%edx, %%esi\n"
        "B%=:\n"
            "testl %%ecx, %%ecx\n"
            "jz F%=\n"
            "shll $3, %%ecx\n"
            "movl $1, %%edx\n"
            "movl $0x57559429, %%eax\n"
            "shll %%cl, %%edx\n"
            "addl $-1, %%edx\n"
            "andl (%%edi), %%edx\n"
            "mull %%edx\n"
            "xorl %%eax, %%esi\n"
            "xorl %%edx, %%ebx\n"
        "F%=:\n"
            "leal 0x5052acdb(%%esi), %%edx\n"
            "xorl %%ebx, %%edx\n"
            "movl $0x5052acdb, %%eax\n"
            "mull %%edx\n"
            "xorl %%ebx, %%eax\n"
            "xorl %%edx, %%esi\n"
            "xorl %%esi, %%eax\n"
            : "=a"(hash), "=c"(len), "=S"(len), "=D"(key)
            : "c"(len), "S"(seed), "D"(key)
            : "%ebx", "%edx", "cc"
        );
        return hash;
#else
        #define cwfold( a, b, lo, hi ) { p = (uint32_t)(a) * (uint64_t)(b); lo ^= (uint32_t)p; hi ^= (uint32_t)(p >> 32); }
        #define cwmixa( in ) { cwfold( in, m, k, h ); }
        #define cwmixb( in ) { cwfold( in, n, h, k ); }

        const uint32_t m = 0x57559429, n = 0x5052acdb, *key4 = (const uint32_t *)key;
        uint32_t h = len, k = len + seed + n;
        uint64_t p;

        while ( len >= 8 ) { cwmixb(key4[0]) cwmixa(key4[1]) key4 += 2; len -= 8; }
        if    ( len >= 4 ) { cwmixb(key4[0]) key4 += 1; len -= 4; }
        if    ( len )      { cwmixa(key4[0] & ( ( 1 << ( len * 8 ) ) - 1 ) ) }
        cwmixb( h ^ (k + n) )
        return k ^ h;
#endif
    }

} // namespace detail
} // namespace utxx

#endif // _UTXX_HASHMAP_HPP_
