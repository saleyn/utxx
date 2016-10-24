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
#pragma once

#ifdef __GXX_EXPERIMENTAL_CXX0X__
#  include <unordered_map>
#else
#  include <boost/unordered_map.hpp>
#endif

#include <limits.h>
#include <math.h>
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

    //-----------------------------------------------------------------------------
    // MurmurHash2, 64-bit and 32-bit versions, by Austin Appleby (MIT license)
    //-----------------------------------------------------------------------------
    inline uint64_t murmur_hash64(const void* key, int len, unsigned int seed)
    {
        const uint64_t m = 0xc6a4a7935bd1e995;
        const int r = 47;

        uint64_t h = seed ^ (len * m);

        const uint64_t * data = (const uint64_t *)key;
        const uint64_t * end = data + (len/8);

        while(data != end)
        {
            uint64_t k = *data++;

            k *= m; 
            k ^= k >> r; 
            k *= m; 

            h ^= k;
            h *= m; 
        }

        const unsigned char * data2 = (const unsigned char*)data;

        switch(len & 7)
        {
            case 7: h ^= uint64_t(data2[6]) << 48;
            case 6: h ^= uint64_t(data2[5]) << 40;
            case 5: h ^= uint64_t(data2[4]) << 32;
            case 4: h ^= uint64_t(data2[3]) << 24;
            case 3: h ^= uint64_t(data2[2]) << 16;
            case 2: h ^= uint64_t(data2[1]) << 8;
            case 1: h ^= uint64_t(data2[0]);
                    h *= m;
        };

        h ^= h >> r;
        h *= m;
        h ^= h >> r;

        return h;
    } 

    // 64-bit hash for 32-bit platforms
    inline uint32_t murmur_hash32(const void* key, int len, unsigned int seed)
    {
        // 'm' and 'r' are mixing constants generated offline.
        // They're not really 'magic', they just happen to work well.
        const unsigned int m = 0x5bd1e995;
        const int r = 24;

        // Initialize the hash to a 'random' value

        unsigned int h = seed ^ len;

        // Mix 4 bytes at a time into the hash

        const unsigned char * data = (const unsigned char *)key;

        while(len >= 4)
        {
            unsigned int k = *(unsigned int *)data;

            k *= m; 
            k ^= k >> r; 
            k *= m; 

            h *= m; 
            h ^= k;

            data += 4;
            len -= 4;
        }

        // Handle the last few bytes of the input array

        switch(len)
        {
            case 3: h ^= data[2] << 16;
            case 2: h ^= data[1] << 8;
            case 1: h ^= data[0];
                    h *= m;
        };

        // Do a few final mixes of the hash to ensure the last few
        // bytes are well-incorporated.

        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;

        return h;
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
    // Copyright (c) 2014 Darach Ennis < darach at gmail dot com >.
    //
    // Permission is hereby granted, free of charge, to any person obtaining a
    // copy of this software and associated documentation files (the
    // "Software"), to deal in the Software without restriction, including
    // without limitation the rights to use, copy, modify, merge, publish,
    // distribute, sublicense, and/or sell copies of the Software, and to permit
    // persons to whom the Software is furnished to do so, subject to the
    // following conditions:
    //
    // The above copyright notice and this permission notice shall be included
    // in all copies or substantial portions of the Software.
    //
    // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    // OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    // MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
    // NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    // DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
    // OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
    // USE OR OTHER DEALINGS IN THE SOFTWARE.

    // Fast Jump Consistent Hash Function that prov
    // See: http://arxiv.org/ftp/arxiv/papers/1406/1406.2294.pdf
    // The function satisfies the two properties:
    //   (1) about the same number of keys map to each bucket
    //   (2) the mapping from key to bucket is perturbed as little as possible
    //       when the number of buckets changes
    inline int jch_chash(unsigned long key, unsigned int num_buckets)
    {
        // a reasonably fast, good period, low memory use, xorshift64 based prng
        static auto lcg_next = [](unsigned long x)
        {
            x ^= x >> 12; // a
            x ^= x << 25; // b
            x ^= x >> 27; // c
            return std::make_pair
                ((double)(x * 2685821657736338717LL) / ULONG_MAX, x);
        };

        unsigned long seed = key; int b = -1; int j = 0;

        do {
            b = j;
            double r;
            std::tie(r, seed) = lcg_next(seed);
            j = floor( (b + 1)/r );
        } while(j < int(num_buckets));

        return b;
    }

} // namespace detail
} // namespace utxx