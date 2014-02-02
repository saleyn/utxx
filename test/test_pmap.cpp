//----------------------------------------------------------------------------
/// \file  test_bitmap.cpp
//----------------------------------------------------------------------------
/// \brief This is a test file for validating bitmap.hpp functionality.
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
//#define BOOST_TEST_MODULE bitmap_test 

#include <boost/test/unit_test.hpp>
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

static void test1(bool output) {
    const uint64_t mask = 0x8080808080808080ul;

    for (int i=0; i < utxx::length(test_set); ++i) {
        const uint64_t v = *(const uint64_t*)test_set[i];
        uint64_t stop = v & mask;
        int      pos  = stop ? utxx::bits::bit_scan_forward(stop) : -1;
        if (output)
            printf("%16lx -> %d\n", v, pos < 0 ? pos : (pos >> 3)+1);
    }
}

static void test2(bool output) {
    const int N = 8;
    char acc[N];     // Accumulator

    for (int i=0; i < utxx::length(test_set); ++i) {
        int  n      = 0;
        bool done   = false;
        for (char const* curr = test_set[i], *e=curr+8; curr < e && n < N && !done; ++curr, ++n)
        {
            char  c = *curr;
            done    = (c & 0x80);     // Check the stop bit
            acc[n]  = (c & 0x7f);     // Drop  the stop bit
        }
        if (output)
            printf("%16lx -> %d\n", *(const uint64_t*)test_set[i], n);
    }
}
/*
  // How many entries have we got? Their bits will be stored in the HIGH part
  // of the "PMap", and the low part filled with 0s;   initial "shift" is the
  // bit size of the "0"s padding:
  //
  assert(n <= N);
  int    shift  = 64 - 7 * n;
  assert(shift >= 1);     // At least because 9 groups would use 63 bits!

  // Now store the "acc" bits in the reverse order, so that acc[0] would come
  // to the highest part of "pmap:
  //
  pmap  = 0;
  for (int i = n-1; i >= 0; --i, shift += 7)
  {
    uint64_t   tmp = acc[i];
    tmp    <<= shift;
    (*pmap) |= tmp;
  }
*/

BOOST_AUTO_TEST_CASE( test_pmap )
{
    long ITERATIONS = getenv("ITERATIONS") ? atol(getenv("ITERATIONS")) : 10000000;
    uint64_t value;

    BOOST_MESSAGE("Iterations: " << ITERATIONS);

    boost::function<void(bool)> tests[] = { &test1, &test2 };

    for (int t=0; t < 2; t++)
    {
        time_val start = time_val::universal_time();

        tests[t](verbosity::level() >= VERBOSE_DEBUG);

        for (long i=0; i < ITERATIONS; ++i)
            tests[t](false);

        double elapsed = time_val::universal_time().now_diff(start);
        int speed = (double)ITERATIONS / elapsed;

        printf("Speed: %d it/s, elapsed: %.6fs\n", speed, elapsed);
    }
}

int GetInteger(const char** buf, const char* end, uint64_t* res) {
    bool        nullable;
    char const* buff = *buf;
    bool        is_null;

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
    is_null = false;
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

    //if (is_neg != NULL)
    //  *is_neg = neg;

    return n;
  }


int unmask_7bit_uint56(const char** buff, const char* end, uint64_t* value) {
    static const uint64_t s_mask   = 0x8080808080808080ul;
    static const uint64_t s_unmask = 0x7F7F7F7F7F7F7F7Ful;

    uint64_t v    = *(const uint64_t*)*buff;
    uint64_t stop = v & s_mask;
    if (stop) {
        int      pos  = __builtin_ffsl(stop);
        uint64_t shft = pos == 64 ? -1ul : (1ul << pos)-1;
        pos  >>= 3;
        *value = v & s_unmask & shft;
        *buff += pos;
        return pos;
    }

    *value = v & s_unmask;
    *buff += 8;
    return 0;
    /*
    printf("=[%d]==> 0x", num);
    for (const char* q = buff; q != end; ++q)
        printf("%02x", *(uint8_t*)q);
    printf(" -> %d (%d) *p=%02x\n", pos >> 3, pos, *(uint8_t*)p);
    printf("=[%d]==> 0x%-016lx\n"
           "(mask v=0x%-016lx) shft=0x%-016lx\n",
        num, v, v & 0x7F7F7F7F7F7F7F7Ful & shft, shft);
    v &= 0x7F7F7F7F7F7F7F7Ful & shft;
    uint64_t pmap = v;
    //uint64_t pmap = utxx::cast64be((const char*)&v);
    */
}

inline int find_stopbit_byte(const char** buff, const char* end) {
    const uint64_t s_mask = 0x8080808080808080ul;
    const char*     start = *buff;

    uint64_t v = *(const uint64_t*)start;
    uint64_t stop = v & s_mask;
    int pos;

    if (likely(stop)) {
        pos = __builtin_ffsl(stop) >> 3;
        *buff += pos;
    } else {
        // In case the stop bit is not found in 64 bits,
        // we need to check next bytes
        const char* p = *buff + 8;
        if (p > end) p = end;
        pos = find_stopbit_byte(&p, end);
        *buff = p;
    }

    return pos;
}

int decode_uint_loop(const char** buff, const char* end, uint64_t* val) {
  const char* p = *buff;
  uint64_t n = 0;

  for (int i=0; p != end; i += 7) {
    uint64_t m = (uint64_t)(*p & 0x7F) << i;
    n |= m;
    //printf("    i=%d, p=%02x, m=%016x, n=%016x\n", i, *(uint8_t*)p, m, n);
    if (*p++ & 0x80) {
        *val  = n;
        n     = p - *buff;
        *buff = p;
        return n;
    }
  }

  return 0;
}

inline uint64_t decode_uint_p1(uint64_t v) {
    return v;
}
inline uint64_t decode_uint_p2(uint64_t v) {
    return v       & 0x000000000000007F
         |(v >> 1) & 0x0000000000003F80;
}
inline uint64_t decode_uint_p3(uint64_t v) {
    return v       & 0x000000000000007F
         |(v >> 1) & 0x0000000000003F80
         |(v >> 2) & 0x00000000001FC000;
}
inline uint64_t decode_uint_p4(uint64_t v) {
    return v       & 0x000000000000007F
         |(v >> 1) & 0x0000000000003F80
         |(v >> 2) & 0x00000000001FC000
         |(v >> 3) & 0x000000000FE00000;
}
inline uint64_t decode_uint_p5(uint64_t v) {
    return v       & 0x000000000000007F
         |(v >> 1) & 0x0000000000003F80
         |(v >> 2) & 0x00000000001FC000
         |(v >> 3) & 0x000000000FE00000
         |(v >> 4) & 0x00000007F0000000;
}
inline uint64_t decode_uint_p6(uint64_t v) {
    return v       & 0x000000000000007F
         |(v >> 1) & 0x0000000000003F80
         |(v >> 2) & 0x00000000001FC000
         |(v >> 3) & 0x000000000FE00000
         |(v >> 4) & 0x00000007F0000000
         |(v >> 5) & 0x000003F800000000;
}
inline uint64_t decode_uint_p7(uint64_t v) {
    return v       & 0x000000000000007F
         |(v >> 1) & 0x0000000000003F80
         |(v >> 2) & 0x00000000001FC000
         |(v >> 3) & 0x000000000FE00000
         |(v >> 4) & 0x00000007F0000000
         |(v >> 5) & 0x000003F800000000
         |(v >> 6) & 0x0001FC0000000000;
}
inline uint64_t decode_uint_p8(uint64_t v) {
    return v       & 0x000000000000007F
         |(v >> 1) & 0x0000000000003F80
         |(v >> 2) & 0x00000000001FC000
         |(v >> 3) & 0x000000000FE00000
         |(v >> 4) & 0x00000007F0000000
         |(v >> 5) & 0x000003F800000000
         |(v >> 6) & 0x0001FC0000000000
         |(v >> 7) & 0x00FE000000000000;
}

int decode_uint_fast2(const char** buff, const char* end, uint64_t* val) {
  int n = unmask_7bit_uint56(buff, end, val);
  uint64_t v = *val;
  typedef uint64_t (*fun_type)(uint64_t);
  static const fun_type f[] = {
      &decode_uint_p1,
      &decode_uint_p2,
      &decode_uint_p3,
      &decode_uint_p4,
      &decode_uint_p5,
      &decode_uint_p6,
      &decode_uint_p7,
      &decode_uint_p8
  };

  if (v > length(f))
      return 0;

  *val = f[n](v);
  return n;
}

int decode_uint_fast(const char** buff, const char* end, uint64_t* val) {
  int n = unmask_7bit_uint56(buff, end, val);
  uint64_t v = *val;
  switch (n) {
    case 1: break;
    case 2: *val = v       & 0x000000000000007F
                 |(v >> 1) & 0x0000000000003F80;
            break;
    case 3: *val = v       & 0x000000000000007F
                 |(v >> 1) & 0x0000000000003F80
                 |(v >> 2) & 0x00000000001FC000;
            break;
    case 4: *val = v       & 0x000000000000007F
                 |(v >> 1) & 0x0000000000003F80
                 |(v >> 2) & 0x00000000001FC000
                 |(v >> 3) & 0x000000000FE00000;
            break;
    case 5: *val = v       & 0x000000000000007F
                 |(v >> 1) & 0x0000000000003F80
                 |(v >> 2) & 0x00000000001FC000
                 |(v >> 3) & 0x000000000FE00000
                 |(v >> 4) & 0x00000007F0000000;
            break;
    case 6: *val = v       & 0x000000000000007F
                 |(v >> 1) & 0x0000000000003F80
                 |(v >> 2) & 0x00000000001FC000
                 |(v >> 3) & 0x000000000FE00000
                 |(v >> 4) & 0x00000007F0000000
                 |(v >> 5) & 0x000003F800000000;
            break;
    case 7: *val = v       & 0x000000000000007F
                 |(v >> 1) & 0x0000000000003F80
                 |(v >> 2) & 0x00000000001FC000
                 |(v >> 3) & 0x000000000FE00000
                 |(v >> 4) & 0x00000007F0000000
                 |(v >> 5) & 0x000003F800000000
                 |(v >> 6) & 0x0001FC0000000000;
            break;
    case 8: *val = v       & 0x000000000000007F
                 |(v >> 1) & 0x0000000000003F80
                 |(v >> 2) & 0x00000000001FC000
                 |(v >> 3) & 0x000000000FE00000
                 |(v >> 4) & 0x00000007F0000000
                 |(v >> 5) & 0x000003F800000000
                 |(v >> 6) & 0x0001FC0000000000
                 |(v >> 7) & 0x00FE000000000000;
            break;
  }

  return n;
}

BOOST_AUTO_TEST_CASE( test_pmap_decode_int )
{
    long ITERATIONS = getenv("ITERATIONS") ? atol(getenv("ITERATIONS")) : 10000000;
    uint64_t value;

    for (int i=0; i < length(test_set); ++i) {
        const uint64_t v = *(const uint64_t*)test_set[i];
        uint64_t t1, t2, t3;
        int r1, r2, r3;
        const char* p = test_set[i];
        const char* q = test_set[i];
        const char* m = test_set[i];
        BOOST_REQUIRE_EQUAL(decode_uint_loop(&p, p+8, &t1),
                            decode_uint_fast(&q, q+8, &t2));
        BOOST_REQUIRE_EQUAL(p, q);
        if (t1 != t2) {
            for (int j=0; j < strlen(test_set[i]); j++)
                printf("%02x ", (uint8_t)test_set[i][j]);
            printf("\n  loop() -> %lx\n", t1);
            printf(  "  fast() -> %lx\n", t2);
        }
        BOOST_REQUIRE_EQUAL(t1, t2);
/*
        BOOST_REQUIRE_EQUAL(
                            decode_uint_fast(&q, q+8, &t2));
        BOOST_REQUIRE_EQUAL(p, q);
        */
    }

    boost::function<int(const char**, const char*, uint64_t*)> tests[] = {
        &decode_uint_loop, &decode_uint_fast, &decode_uint_fast2, &GetInteger
    };

    for (int t=0; t < length(tests); t++)
    {
        time_val start = time_val::universal_time();

        for (long i=0; i < ITERATIONS; ++i)
            for (int i=0; i < length(test_set); ++i) {
                const uint64_t v = *(const uint64_t*)test_set[i];
                uint64_t res;
                const char* p = test_set[i];
                tests[t](&p, p+8, &res);
            }

        double elapsed = time_val::universal_time().now_diff(start);
        int speed = (double)ITERATIONS / elapsed;

        printf("Speed: %d it/s, elapsed: %.6fs\n", speed, elapsed);
    }
}


uint32_t decode_forts_seqno(const char* buff, int n, int num) {
    int len = find_stopbit_byte(&buff, buff+n);
    const char* q = buff;
    uint64_t  tid, seq = 0;
    int      tres = decode_uint_loop(&q, q+5, &tid);
    int      sres = decode_uint_loop(&q, q+5, &seq);

    /*
    printf("  stop=0x%-016lx, pmap=%-016lx, ", stop, pmap);
    printf("len=%d, seq=%08x (res=%d)\n",  q-p, seq, res);
    assert(pos >= 0);
    assert(!tres);
    assert(!sres);
    */
    return seq;
}

BOOST_AUTO_TEST_CASE( test_pmap_seqno )
{
    const char s[] = "abc";
    std::cout << "Length of s: " << length(s) << std::endl;

    for (int i=0; i < utxx::length(test_set); i++)
        uint32_t n = decode_forts_seqno(test_set[i], 8, i);
}


