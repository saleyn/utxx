//----------------------------------------------------------------------------
/// \brief This is a test file for validating pmap.hpp functionality.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-12-21
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
#ifndef UTXX_STANDALONE
//#define BOOST_TEST_MODULE pmap_test
#include <boost/test/unit_test.hpp>
#else
#define BOOST_AUTO_TEST_CASE( X ) void X()
#define BOOST_TEST_MESSAGE( X ) std::cout << X << std::endl;
#define BOOST_REQUIRE( X ) do { \
        if (!(X)) { std::cout << "Required !" << ##X << ":" << (X) << std::end; exit(1); } \
    } while(0)
#define BOOST_REQUIRE_EQUAL( X,Y ) do { \
        if ((X) != (Y)) { std::cout << (X) << " =/= " << (Y) << std::end; exit(1); } \
    } while(0)
#endif

#include <utxx/bits.hpp>
#include <utxx/string.hpp>
#include <limits>
#include <stdio.h>
#include <boost/function.hpp>
#include <utxx/verbosity.hpp>
#include <utxx/time_val.hpp>
#include <utxx/endian.hpp>
#include <utxx/compiler_hints.hpp>

using namespace utxx;
namespace bsd = boost::spirit::detail;

static const char* test_set[] = {
  /* 0 */  "\x9f\x81\x92",
  /* 1 */  "\x7e\xAf\x81\x71\x93",
  /* 2 */  "\x6d\x7e\xBf\x81\x6d\x7e\x94",
  /* 3 */  "\x5c\x6d\x7e\xCf\x81\x5c\x6d\x7e\x95",
  /* 4 */  "\x4f\x5c\x6d\x7e\xDf\x81\x4f\x5c\x6d\x7e\x96",
  /* 5 */  "\x3f\x4f\x5c\x6d\x7e\xEf\x81\x3f\x4f\x5c\x97",
  /* 6 */  "\x2f\x3f\x4f\x5c\x6d\x7e\xFf\x81\x2f\x3f\x4f\x98",
  /* 7 */  "\x1f\x2f\x3f\x4f\x5c\x6d\x7e\xFf\x81\x1e\x2d\x3c\x99"
};

static const uint8_t buffers0[] = 
    {0xc0,0xf8,0xe0,0xca,0x6f,0x41,0xd8,0x23,0x63,0x2d,0x12,0x54,0x66,0x6d,0xf4,0x87,0x98
    ,0xb1,0x30,0x2d,0x44,0xc7,0x22,0xec,0x0f,0x0a,0xc8,0x95,0x82,0x80,0xff,0x00,0x62
    ,0xa7,0x89,0x80,0x00,0x52,0x11,0x55,0xeb,0x80,0x80,0x80,0x80,0x80,0xc0,0x81,0xb1
    ,0x81,0x0f,0x0a,0xc9,0x83,0x80,0xff,0x00,0x62,0xa8,0x00,0xf1,0x80,0x80,0x80,0x80
    ,0x80,0x80,0x80,0x80,0xb1,0x81,0x0f,0x0a,0xca,0x85,0x80,0xff,0x00,0x62,0xaa,0x00
    ,0xe5,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xb1,0x74,0x03,0x32,0x80,0x15,0x4f
    ,0xec,0x83,0x80,0x82,0x00,0x68,0x9f,0x89,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80
    ,0xb1,0x81,0x15,0x4f,0xed,0x84,0x80,0x82,0x00,0x68,0xa0,0x8d,0x80,0x81,0x80,0x80
    ,0x80,0x80,0x80,0x80,0xb1,0x81,0x15,0x4f,0xee,0x85,0x80,0x82,0x00,0x68,0xa1,0x88
    ,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xb1,0x0f,0x0e,0x52,0x81,0x1c,0x21,0xc4
    ,0x82,0x80,0x81,0x00,0x4c,0x9b,0x8c,0x80,0x80,0x80,0x80,0x80,0x80,0x80};

static const uint8_t buffers1[] =
    {0xc0,0xf8,0xe0,0xca,0x6f,0x41,0xd9,0x23,0x63,0x2d,0x12,0x54,0x66,0x6e,0x82,0x81,0xd8
    ,0x81,0xb1,0x33,0x3f,0x48,0xc7,0x22,0xec,0x1c,0x21,0xc5,0x95,0x82,0x80,0x81,0x00
    ,0x4c,0x9b,0x8b,0x80,0x00,0x52,0x11,0x55,0xfd,0x80,0x80,0x80,0x80,0x80};

static const uint8_t buffers2[] =
    {0xc0,0xf8,0xe0,0xca,0x6f,0x41,0xda,0x23,0x63,0x2d,0x12,0x54,0x66,0x6e,0x90,0x85,0xd8
    ,0x82,0xb1,0x33,0x3f,0x48,0xc7,0x22,0xec,0x1c,0x21,0xc6,0x95,0x82,0x80,0x81,0x00
    ,0x4c,0x9b,0x81,0x80,0x00,0x52,0x11,0x55,0xfd,0x80,0x80,0x80,0x80,0x80,0xc0,0x80
    ,0xb1,0x81,0x1c,0x21,0xc7,0x95,0x80,0x81,0x00,0x4c,0xaf,0x04,0xaa,0x80,0x7f,0x0b
    ,0x6d,0xb6,0x80,0x80,0x80,0x80,0x80,0x80,0xb0,0x7c,0x6d,0x74,0x80,0x1e,0x6b,0xef
    ,0x82,0x80,0x82,0x00,0x73,0xf1,0x87,0x80,0x00,0x74,0x12,0xda,0x80,0x80,0x80,0x80
    ,0x80,0x80,0xb0,0x81,0x1e,0x6b,0xf0,0x83,0x80,0x82,0x00,0x73,0xf0,0x86,0x80,0xfc
    ,0x80,0x80,0x80,0x80,0x80,0xc0,0x81,0xb0,0x81,0x1e,0x6b,0xf1,0x84,0x80,0x82,0x00
    ,0x73,0xef,0x89,0x80,0x84,0x80,0x80,0x80,0x80,0x80};

static const uint8_t buffers3[] =
    {0xc0,0xf8,0xe0,0xca,0x6f,0x41,0xdb,0x23,0x63,0x2d,0x12,0x54,0x66,0x6e,0xd9,0x82,0xd8
    ,0x82,0xb0,0x30,0x2d,0x3c,0xc7,0x22,0xec,0x1e,0x6b,0xf2,0x95,0x83,0x80,0x82,0x00
    ,0x73,0xf0,0x81,0x80,0x00,0x52,0x11,0x56,0xdb,0x80,0x80,0x80,0x80,0x80,0xc0,0x80
    ,0xb0,0x81,0x1e,0x6b,0xf3,0x95,0x80,0x82,0x00,0x71,0xe5,0x82,0x80,0x72,0x7b,0x1a
    ,0xde,0x80,0x80,0x80,0x80,0x80};

static const uint8_t* buffers[] = {buffers0, buffers1, buffers2, buffers3};

int decode_uint_fast(const char** buff, const char* end, uint64_t* val);
int decode_uint_fast2(const char** buff, const char* end, uint64_t* val);
int decode_uint_loop(const char** buff, const char* end, uint64_t* val);
int find_stopbit_byte(const char* buff, const char* end);

static long test1(bool output, int i) {
    uint64_t v;
    const char* p = test_set[i];
    int n = decode_uint_fast(&p, test_set[i]+strlen(test_set[i]), &v);
    if (output)
        printf("%16lx -> %d\n", v, n);
    return n;
}

static long test2(bool output, int i) {
    uint64_t v;
    const char* p = test_set[i];
    int n = decode_uint_fast2(&p, test_set[i]+strlen(test_set[i]), &v);
    if (output)
        printf("%16lx -> %d\n", v, n);
    return n;
}

static long test3(bool output, int i) {
    uint64_t v;
    const char* p = test_set[i];
    int n = decode_uint_loop(&p, test_set[i]+strlen(test_set[i]), &v);
    if (output)
        printf("%16lx -> %d\n", v, n);
    return n;
}

static long test4(bool output, int j) {
    const int N = 8;
    char acc[N];     // Accumulator

    int  n      = 0;
    bool done   = false;
    for (char const* curr = test_set[j], *e=curr+8; curr < e && n < N && !done; ++curr, ++n)
    {
        char  c = *curr;
        done    = (c & 0x80);     // Check the stop bit
        acc[n]  = (c & 0x7f);     // Drop  the stop bit
    }
    //
    // How many entries have we got? Their bits will be stored in the HIGH part
    // of the "PMap", and the low part filled with 0s;   initial "shift" is the
    // bit size of the "0"s padding:
    //
    //assert(n <= N);
    int    shift  = 64 - 7 * n;
    //assert(shift >= 1);     // At least because 9 groups would use 63 bits!

    // Now store the "acc" bits in the reverse order, so that acc[0] would come
    // to the highest part of "pmap:
    //
    long pmap  = 0;
    for (int i = n-1; i >= 0; --i, shift += 7)
    {
        uint64_t   tmp = acc[i];
        tmp    <<= shift;
        pmap |= tmp;
    }
    if (output)
        printf("%16lx -> %ld\n", *(const uint64_t*)test_set[j], pmap);
    return pmap;
}

BOOST_AUTO_TEST_CASE( test_pmap )
{
    long ITERATIONS = getenv("ITERATIONS") ? atol(getenv("ITERATIONS")) : 10000000;

    BOOST_TEST_MESSAGE("Iterations: " << ITERATIONS);

    boost::function<int(bool, int)> tests[] = { &test1, &test2, &test3, &test4 };

    for (int t=0; t < (int)length(test_set); ++t) {
        long r1 = test1(false, t);
        long r2 = test2(false, t);
        long r3 = test3(false, t);
        long r4 = test4(false, t);

        BOOST_REQUIRE_EQUAL(r1, r4);
        BOOST_REQUIRE_EQUAL(r2, r4);
        BOOST_REQUIRE_EQUAL(r3, r4);
    }

    for (uint32_t t=0; t < sizeof(tests)/sizeof(tests[0]); t++)
    {
        timer start;

        tests[t](verbosity::level() >= VERBOSE_DEBUG, 0);

        for (long i=0; i < ITERATIONS; ++i)
            for (int j=0; j < (int)length(test_set); ++j)
                tests[t](false, j);

        double elapsed = start.elapsed();
        int speed = (double)ITERATIONS / elapsed;

        printf("Speed: %d it/s, elapsed: %.6fs\n", speed, elapsed);
    }
}

int GetInteger(const char** buf, const char* end, uint64_t* res) {
    const bool  nullable = true;
    char const* buff = *buf;

    // Here the result is accumulated from 7-bit bytes; maximum of N 7-bit
    // bytes for the size of type "T":
    static const int BitSz = sizeof(uint64_t) * 8;
    static const int N =     BitSz / 7 + 1;
    char acc[N];          // Accumulator

    // Scan the buffer:
    bool done = false;
    int  n    = 0;
    for (char const* curr = buff; curr < end && n < N && !done; ++curr, ++n)
    {
      char c = *curr;
      done   = (c & 0x80);     // Check the stop bit
      acc[n] = (c & 0x7f);     // Get the data bits
    }
    if (!done)
      throw std::runtime_error
            ("FAST::GetInteger: UnTerminated value");

    // Otherwise: must have read at least 1 byte:
    assert(n >= 1);

    // We got "n" bytes in the network (big-endian) mode,  so read them  from
    // right to left:
    //
    uint64_t lres = 0;
    int shift  = 0;
    for (int i = n-1; i >= 0; --i, shift += 7)
    {
      // NB: "c" is a 7-bit value, so the high (7th) bit is always clear,  so
      // converting "c" to "T" would produce a non-negative value. This is OK
      // for now -- we do not want premature sign bit propagation in shift:
      uint64_t tmp = acc[i];
      assert(tmp >= 0);

      tmp <<=  shift;
      lres |=  tmp;
    }

    bool neg = false;
    if (false)
    {
      // For signed types, need to check  if the over-all value  is negative.
      // This is determined by the 6th (sign) bit of the leading 7-bit group:
      neg = (acc[0] & 0x40);

      // If negative, fill in the high part of "lres" ((BitSz-shift) bits) with
      // "1"s:
      if (neg && shift < BitSz)
      {
        // Make a mask of (shift) "1"s and invert it:
        uint64_t mask = ~((uint64_t(1) << shift) - 1);
        // Apply it:
        lres  |= mask;
      }
    }

    // Check for NULL and adjust nullable values:
    bool is_null = false;

    if (unlikely(!is_null))  // Just to avoid compiler warning
        throw std::runtime_error("Impossible happened");

    if (nullable)
    {
      if (n == 1 && !acc[0])
        is_null = true;
      else
        // Non-null: decrement the value (for signed types,  only if it is non-
        // negative).  IMPORTANT:  because of possible overflows,  we must NOT
        // compare "lres" with 0 (may get false equality) -- use the "neg" flag
        // instead!!!
        if (!neg)
          lres--;
    }

    // Invariant test (but only AFTER the NULL adjustment). Note that "neg" set
    // and the value being 0 IS ALLOWED:
    //
    if (neg && lres > 0) // || (is_signed<T>::value && !neg && lres < 0))
      throw std::runtime_error
            ("FAST::GetInteger: lres");

    *res = lres;
    assert(buff + n <= end);

    //if (is_neg != NULL && is_null)
    //  *is_neg = neg;

    return n;
}


inline int find_stopbit_byte(const char* buff, const char* end) {
    const uint64_t*     p = (const uint64_t*)buff;

    uint64_t unmasked = *p & 0x8080808080808080u;
    int pos;

    if (likely(unmasked)) {
        pos = __builtin_ffsl(unmasked) >> 3;
    } else {
        // In case the stop bit is not found in 64 bits,
        // we need to check next bytes
        unmasked = *(++p) & 0x8080000000000000u;
        if (unlikely(!unmasked))
            return 0;
        pos = 8 + (__builtin_ffsl(unmasked) >> 3);
    }

    return likely(buff + pos < end) ? pos : 0;
}


int decode_uint_loop(const char** buff, const char* end, uint64_t* val) {
  const char* p = *buff;
  int e, i = 0;
  uint64_t n = 0;

  int len = find_stopbit_byte(p, end);

  //printf("find_stopbit_byte(%02x) -> %d\n", (uint8_t)*p, len);

  for (p += len-1, e = len*7; i < e; i += 7, --p) {
    uint64_t m = (uint64_t)(*p & 0x7F) << i;
    n |= m;
    //printf(" %2d| 0x%02x, m=%lu, n=%lu\n", i / 7, (uint8_t)*p, m, n);
  }

  *val  = n;
  *buff += len;
  return len;
}

namespace {

    template <int I, int N>
    struct unpack {
        inline static uint64_t value(const char* v) {
            return unpack<I-1,N>::value(v) | (((uint64_t)v[I] & 0x7F) << (N-I)*7);
        }
    };

    template <>
    struct unpack<0,0> {
        inline static uint64_t value(const char* v) {
            return (uint64_t)v[0] & 0x7F;
        }
    };

    template <int N>
    struct unpack<0,N> {
        inline static uint64_t value(const char* v) {
            return ((uint64_t)v[0] & 0x7F) << N*7;
        }
    };

    template <int N>
    struct unpack<N,N> {
        inline static uint64_t value(const char* v) {
            return unpack<N-1,N>::value(v) | ((uint64_t)v[N] & 0x7F);
        }
    };

    template <>
    struct unpack<0,9> {
        inline static uint64_t value(const char* v) {
            return (uint64_t)(v[0] & 0x01) << (7*9);
        }
    };

    template <int N>
    struct unpacker {
        static uint64_t value(const char* v) { return unpack<N,N>::value(v); }
    };
}

inline uint64_t decode_uint_p1(const char* v) {
    return (uint64_t)v[0] & 0x7F;
}
inline uint64_t decode_uint_p2(const char* v) {
    return (((uint64_t)(v[0] & 0x7F)) <<  7)
        |   ((uint64_t)(v[1] & 0x7F));
}
inline uint64_t decode_uint_p3(const char* v) {
    return (((uint64_t)(v[0] & 0x7F)) << 14)
        |  (((uint64_t)(v[1] & 0x7F)) <<  7)
        |   ((uint64_t)(v[2] & 0x7F));
}
inline uint64_t decode_uint_p4(const char* v) {
    return (((uint64_t)(v[0] & 0x7F)) << 21)
        |  (((uint64_t)(v[1] & 0x7F)) << 14)
        |  (((uint64_t)(v[2] & 0x7F)) <<  7)
        |   ((uint64_t)(v[3] & 0x7F));
}
inline uint64_t decode_uint_p5(const char* v) {
    return (((uint64_t)(v[0] & 0x7F)) << 28)
        |  (((uint64_t)(v[1] & 0x7F)) << 21)
        |  (((uint64_t)(v[2] & 0x7F)) << 14)
        |  (((uint64_t)(v[3] & 0x7F)) <<  7)
        |   ((uint64_t)(v[4] & 0x7F));
}
inline uint64_t decode_uint_p6(const char* v) {
    return (((uint64_t)(v[0] & 0x7F)) << 35)
        |  (((uint64_t)(v[1] & 0x7F)) << 28)
        |  (((uint64_t)(v[2] & 0x7F)) << 21)
        |  (((uint64_t)(v[3] & 0x7F)) << 14)
        |  (((uint64_t)(v[4] & 0x7F)) <<  7)
        |   ((uint64_t)(v[5] & 0x7F));
}
inline uint64_t decode_uint_p7(const char* v) {
    return (((uint64_t)(v[0] & 0x7F)) << 42)
        |  (((uint64_t)(v[1] & 0x7F)) << 35)
        |  (((uint64_t)(v[2] & 0x7F)) << 28)
        |  (((uint64_t)(v[3] & 0x7F)) << 21)
        |  (((uint64_t)(v[4] & 0x7F)) << 14)
        |  (((uint64_t)(v[5] & 0x7F)) <<  7)
        |   ((uint64_t)(v[6] & 0x7F));
}
inline uint64_t decode_uint_p8(const char* v) {
    return (((uint64_t)(v[0] & 0x7F)) << 49)
        |  (((uint64_t)(v[1] & 0x7F)) << 42)
        |  (((uint64_t)(v[2] & 0x7F)) << 35)
        |  (((uint64_t)(v[3] & 0x7F)) << 28)
        |  (((uint64_t)(v[4] & 0x7F)) << 21)
        |  (((uint64_t)(v[5] & 0x7F)) << 14)
        |  (((uint64_t)(v[6] & 0x7F)) <<  7)
        |   ((uint64_t)(v[7] & 0x7F));
}
inline uint64_t decode_uint_p9(const char* v) {
    return (((uint64_t)(v[0] & 0x7F)) << 56)
        |  (((uint64_t)(v[1] & 0x7F)) << 49)
        |  (((uint64_t)(v[2] & 0x7F)) << 42)
        |  (((uint64_t)(v[3] & 0x7F)) << 35)
        |  (((uint64_t)(v[4] & 0x7F)) << 28)
        |  (((uint64_t)(v[5] & 0x7F)) << 21)
        |  (((uint64_t)(v[6] & 0x7F)) << 14)
        |  (((uint64_t)(v[7] & 0x7F)) <<  7)
        |   ((uint64_t)(v[8] & 0x7F));
}
inline uint64_t decode_uint_p10(const char* v) {
    return (((uint64_t)(v[0] & 0x01)) << 63)
        |  (((uint64_t)(v[1] & 0x7F)) << 56)
        |  (((uint64_t)(v[2] & 0x7F)) << 49)
        |  (((uint64_t)(v[3] & 0x7F)) << 42)
        |  (((uint64_t)(v[4] & 0x7F)) << 35)
        |  (((uint64_t)(v[5] & 0x7F)) << 28)
        |  (((uint64_t)(v[6] & 0x7F)) << 21)
        |  (((uint64_t)(v[7] & 0x7F)) << 14)
        |  (((uint64_t)(v[8] & 0x7F)) <<  7)
        |   ((uint64_t)(v[9] & 0x7F));
}

int decode_uint_fast(const char** buff, const char* end, uint64_t* val) {
    int n = find_stopbit_byte(*buff, end);
    typedef uint64_t (*fun_type)(const char*);
    static const fun_type f[] = {
        &decode_uint_p1,
        &decode_uint_p2,
        &decode_uint_p3,
        &decode_uint_p4,
        &decode_uint_p5,
        &decode_uint_p6,
        &decode_uint_p7,
        &decode_uint_p8,
        &decode_uint_p9,
        &decode_uint_p10
    };

    if (likely(n)) {
        *val  = f[n](*buff);
        *buff += n;
    }

    return n;
}

int decode_uint_fast2(const char** buff, const char* end, uint64_t* val) {
  int n = find_stopbit_byte(*buff, end);
  typedef uint64_t (*fun_type)(const char*);
  static const fun_type f[] = {
      &unpacker<0>::value,
      &unpacker<1>::value,
      &unpacker<2>::value,
      &unpacker<3>::value,
      &unpacker<4>::value,
      &unpacker<5>::value,
      &unpacker<6>::value,
      &unpacker<7>::value,
      &unpacker<8>::value,
      &unpacker<9>::value
  };

  //BOOST_ASSERT(n < length(f));

  if (likely(n)) {
      *val  = f[n](*buff);
      *buff += n;
  }

  return n;
}

BOOST_AUTO_TEST_CASE( test_pmap_decode_int )
{
    long ITERATIONS = getenv("ITERATIONS") ? atol(getenv("ITERATIONS")) : 10000000;

    for (int i=0; i < (int)length(test_set); ++i) {
        uint64_t t1, t2;
        const char* p = test_set[i];
        const char* q = test_set[i];
        //const char* m = test_set[i];
        long r1 = decode_uint_loop(&p, p+8, &t1);
        long r2 = decode_uint_fast(&q, q+8, &t2);
        BOOST_REQUIRE_EQUAL(r1, r2);
        if (t1 != t2) {
            for (int j=0; j < (int)strlen(test_set[i]); j++)
                printf("%02x ", (uint8_t)test_set[i][j]);
            printf("\n  loop() -> %lx\n", t1);
            printf(  "  fast() -> %lx\n", t2);
        }
        BOOST_REQUIRE_EQUAL(t1, t2);

        //BOOST_REQUIRE_EQUAL(decode_uint_fast(&q, q+8, &t2));
        //BOOST_REQUIRE_EQUAL(p, q);
    }

    boost::function<int(const char**, const char*, uint64_t*)> tests[] = {
        &decode_uint_loop, &decode_uint_fast, &decode_uint_fast2, &GetInteger
    };

    for (int t=0; t < (int)length(tests); t++)
    {
        time_val start = time_val::universal_time();

        for (long i=0; i < ITERATIONS; ++i)
            for (int i=0; i < (int)length(test_set); ++i) {
                uint64_t res;
                const char* p = test_set[i];
                tests[t](&p, p+8, &res);
            }

        double elapsed = time_val::now_diff(start);
        int speed = (double)ITERATIONS / elapsed;

        printf("Speed: %d it/s, elapsed: %.6fs\n", speed, elapsed);
    }
}

uint32_t decode_forts_seqno(const char* buff, int n, long last_seqno, int* seq_reset) {
    int res;
    const char* q = buff;
    uint64_t  tid = 120, seq = 0, pmap;

    while (tid == 120) { // reset
      res = decode_uint_loop(&q, q+5, &pmap); 
      res = decode_uint_loop(&q, q+5, &tid);
      BOOST_REQUIRE(res);
    }

    //printf("PMAP=%x, TID=%d, *p=0x%02x\n", pmap, tid, (uint8_t)*q);
    res = decode_uint_loop(&q, q+5, &seq);

    // If sequence reset, parse new seqno
    if (tid == 49) {
      *seq_reset = 1;
      res = decode_uint_loop(&q, q+10, &tid); // SendingTime
      res = decode_uint_loop(&q, q+5,  &seq); // NewSeqNo
      BOOST_REQUIRE(res);
    }

    return seq;
}


BOOST_AUTO_TEST_CASE( test_pmap_seqno )
{
    const char s[] = "abc";
    int new_seqno;
    std::cout << "Length of s: " << length(s) << std::endl;

    for (int i=0; i < (int)length(test_set); i++) {
        uint32_t n = decode_forts_seqno(test_set[i], 8, i, &new_seqno);
        BOOST_REQUIRE(n);
    }

    for (int i=0; i < (int)length(buffers); i++) {
        const char* p   = (const char*)buffers[i], *q = p;
        const char* end = p + strlen((const char*)buffers[i]);
        for (int j=0; j < 3; j++) {
            uint64_t v1, v2;
            int n1 = GetInteger(&p, end, &v1);
            int n2 = decode_uint_fast(&q, end, &v2);
            BOOST_REQUIRE_EQUAL(n1, n2);
            BOOST_REQUIRE_EQUAL(v1, v2);
        }
    }
}

#ifdef UTXX_STANDALONE
int main()
{
    test_pmap();
    test_pmap_decode_int();
    test_pmap_seqno();
    return 0;
}
#endif
