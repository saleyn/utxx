// vim:ts=2:sw=2:et
//----------------------------------------------------------------------------
/// \file  convert.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Fast integer and floating-point conversion functions — standalone
///        copy with no boost dependencies.
//----------------------------------------------------------------------------
// Copyright (C) 2009 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

***** END LICENSE BLOCK *****
*/
#pragma once

#include <cassert>
#include <cmath>
#include <limits>
#include <reactor/compiler_hints.hpp>
#include <reactor/meta.hpp>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <type_traits>

namespace utxx {

enum alignment { LEFT, RIGHT };

namespace detail {

  static inline char int_to_char(int n)
  {
    static const char  s_characters[] = {"9876543210123456789"};
    static const char* s_middle       = s_characters + 9;
    return s_middle[n];
  }

  template <typename Char, typename I, bool Sign, alignment Align, Char Skip>
  struct convert;

  //-----------------------------------------------------------------------
  // Unrolled loops for converting stream of bytes
  //-----------------------------------------------------------------------
  template <typename Char, int N, alignment Align>
  struct unrolled_byte_loops;

  //-------N=k, right justified--------------------------------------------
  template <typename Char, int N>
  struct unrolled_byte_loops<Char, N, RIGHT> {
    typedef unrolled_byte_loops<Char, N - 1, RIGHT> next;

    inline static uint64_t                          atoi_skip_right(const Char*& bytes, Char skip)
    {
      if (skip && *bytes == skip) return next::atoi_skip_right(--bytes, skip);
      return load_atoi(bytes, 0);
    }

    static void pad_left(Char* bytes, Char ch)
    {
      *bytes = ch;
      next::pad_left(--bytes, ch);
    }

    inline static void save_itoa(Char*& bytes, long n, Char a_pad)
    {
      long m   = n / 10;
      *bytes-- = int_to_char(n - m * 10);
      if (m == 0) {
        if (a_pad) next::pad_left(bytes, a_pad);
        return;
      }
      next::save_itoa(bytes, m, a_pad);
    }

    static uint64_t load_atoi(const Char*& bytes, uint64_t acc)
    {
      typename std::make_unsigned<Char>::type i = *bytes - '0';
      return i > 9u ? 0 : i + 10 * next::load_atoi(--bytes, acc);
    }
  };

  //-------N=k, left justified---------------------------------------------
  template <typename Char, int N>
  struct unrolled_byte_loops<Char, N, LEFT> {
    typedef unrolled_byte_loops<Char, N - 1, LEFT> next;

    inline static uint64_t                         atoi_skip_left(const Char*& bytes, Char skip)
    {
      if (skip && *bytes == skip) return next::atoi_skip_left(++bytes, skip);
      return load_atoi(bytes, 0);
    }

    static void reverse(Char* p, Char* q)
    {
      if (p >= q) return;
      Char c = *p;
      *p     = *q;
      *q     = c;
      next::reverse(++p, --q);
    }

    static void pad_right(Char* bytes, Char ch)
    {
      *bytes = ch;
      next::pad_right(++bytes, ch);
    }

    static void save_itoa(Char*& bytes, long n, Char a_pad)
    {
      long m   = n / 10;
      *bytes++ = int_to_char(n - m * 10);
      if (m == 0) {
        if (a_pad)
          next::pad_right(bytes, a_pad);
        else
          *bytes = '\0';
        return;
      }
      next::save_itoa(bytes, m, a_pad);
    }

    static uint64_t load_atoi(const Char*& bytes, uint64_t acc)
    {
      typename std::make_unsigned<Char>::type i = *bytes - '0';
      return (i > 9u) ? acc : next::load_atoi(++bytes, i + 10 * acc);
    }
  };

  //-------N=1, right justified--------------------------------------------
  template <typename Char>
  struct unrolled_byte_loops<Char, 1, RIGHT> {
    inline static uint64_t atoi_skip_right(const Char*& bytes, Char skip)
    {
      if (skip && *bytes == skip) {
        --bytes;
        return 0;
      }
      return load_atoi(bytes, 0);
    }
    static void        pad_left(Char* bytes, Char ch) { *bytes = ch; }
    inline static void save_itoa(Char*& bytes, long n, Char)
    {
      assert(n / 10 == 0);
      *bytes-- = int_to_char(n % 11);
    }
    static uint64_t load_atoi(const Char*& bytes, uint64_t)
    {
      typename std::make_unsigned<Char>::type i = *bytes - '0';
      if (i > 9u) return 0;
      --bytes;
      return i;
    }
  };

  //-------N=1, left justified---------------------------------------------
  template <typename Char>
  struct unrolled_byte_loops<Char, 1, LEFT> {
    inline static uint64_t atoi_skip_left(const Char*& bytes, Char skip)
    {
      if (skip && *bytes == skip) {
        ++bytes;
        return 0;
      }
      return load_atoi(bytes, 0);
    }
    static void pad_right(Char* bytes, Char ch) { *bytes = ch; }
    static void reverse(Char*, Char*) {}
    static void save_itoa(Char*& bytes, long n, Char)
    {
      assert(n / 10 == 0);
      *bytes++ = int_to_char(n % 10);
    }
    static uint64_t load_atoi(const Char*& bytes, uint64_t acc)
    {
      typename std::make_unsigned<Char>::type i = *bytes - '0';
      if (i > 9u) return acc;
      ++bytes;
      return i + 10 * acc;
    }
  };

  //-------N=0, right justified--------------------------------------------
  template <typename Char>
  struct unrolled_byte_loops<Char, 0, RIGHT> {
    static uint64_t atoi_skip_right(const Char*&, Char) { return 0; }
    static void     pad_left(Char*, Char) {}
    static void     save_itoa(Char*&, long, Char) {}
    static uint64_t load_atoi(const Char*&, uint64_t) { return 0; }
  };

  //-------N=0, left justified---------------------------------------------
  template <typename Char>
  struct unrolled_byte_loops<Char, 0, LEFT> {
    static uint64_t atoi_skip_left(const Char*&, Char) { return 0; }
    static void     pad_right(Char*, Char) {}
    static void     reverse(Char*, Char*) {}
    static void     save_itoa(Char*&, long, Char) {}
    static uint64_t load_atoi(const Char*&, uint64_t) { return 0; }
  };

  //-----------------------------------------------------------------------
  // Conversion helper class
  //-----------------------------------------------------------------------
  template <typename T, int N, bool Sign, alignment Justify, typename Char>
  struct signness_helper;

  //-------Signed, right justified-----------------------------------------
  template <typename T, int N, typename Char>
  struct signness_helper<T, N, true, RIGHT, Char> {
    static char* fast_itoa(Char* bytes, T i, Char pad = '\0')
    {
      char* p;
      if (i < 0) {
        p = signness_helper<T, N, false, RIGHT, Char>::fast_itoa(bytes, -i, pad);
        if (p >= bytes) *p-- = '-';
        return pad ? bytes - 1 : p;
      }
      p = signness_helper<T, N, false, RIGHT, Char>::fast_itoa(bytes, i, pad);
      return pad ? bytes - 1 : p;
    }
    static T fast_atoi(const Char*& bytes, Char skip = '\0')
    {
      const char* p  = bytes;
      bytes         += N - 1;
      uint64_t n     = unrolled_byte_loops<Char, N, RIGHT>::atoi_skip_right(bytes, skip);
      if (bytes < p || *bytes != '-') return static_cast<T>(n);
      --bytes;
      return -static_cast<T>(n);
    }
  };

  //-------Signed, left justified------------------------------------------
  template <typename T, int N, typename Char>
  struct signness_helper<T, N, true, LEFT, Char> {
    static char* fast_itoa(Char* bytes, T i, Char pad = '\0')
    {
      char* p = bytes;
      if (i < 0) {
        *p++ = '-';
        return signness_helper<T, N - 1, false, LEFT, Char>::fast_itoa(p, i, pad);
      }
      return signness_helper<T, N, false, LEFT, Char>::fast_itoa(p, i, pad);
    }

    inline static T fast_atoi(const Char*& bytes, Char skip = '\0')
    {
      auto e = bytes + N;
      if (skip) {
        while (*bytes == skip && bytes != e) ++bytes;
        switch (e - bytes) {
          case 0:  return 0;
          case 1:  return fast_atoi2<1>(bytes);
          case 2:  return fast_atoi2<2>(bytes);
          case 3:  return fast_atoi2<3>(bytes);
          case 4:  return fast_atoi2<4>(bytes);
          case 5:  return fast_atoi2<5>(bytes);
          case 6:  return fast_atoi2<6>(bytes);
          case 7:  return fast_atoi2<7>(bytes);
          case 8:  return fast_atoi2<8>(bytes);
          case 9:  return fast_atoi2<9>(bytes);
          case 10: return fast_atoi2<10>(bytes);
          case 11: return fast_atoi2<11>(bytes);
          case 12: return fast_atoi2<12>(bytes);
          case 13: return fast_atoi2<13>(bytes);
          case 14: return fast_atoi2<14>(bytes);
          case 15: return fast_atoi2<15>(bytes);
          case 16: return fast_atoi2<16>(bytes);
          case 17: return fast_atoi2<17>(bytes);
          case 18: return fast_atoi2<18>(bytes);
          case 19: return fast_atoi2<19>(bytes);
          default: return fast_atoi2<20>(bytes);
        }
      } else
        return fast_atoi2<N>(bytes);
    }

    template <int M>
    inline static T fast_atoi2(const Char*& bytes)
    {
      if (*bytes == '-') {
        long n = unrolled_byte_loops<Char, M - 1, LEFT>::load_atoi(++bytes, 0);
        return static_cast<T>(-n);
      }
      return unrolled_byte_loops<Char, M, LEFT>::load_atoi(bytes, 0);
    }
  };

  //-------Unsigned, right justified---------------------------------------
  template <typename T, int N, typename Char>
  struct signness_helper<T, N, false, RIGHT, Char> {
    static char* fast_itoa(Char* bytes, T i, Char pad = '\0')
    {
      char* p = bytes + N - 1;
      unrolled_byte_loops<Char, N, RIGHT>::save_itoa(p, i, pad);
      return p;
    }
    static T fast_atoi(const Char*& bytes, Char skip = '\0')
    {
      const Char* p  = bytes;
      bytes         += N - 1;
      uint64_t n     = unrolled_byte_loops<Char, N, RIGHT>::atoi_skip_right(bytes, skip);
      if (p > bytes || *bytes != '-') return static_cast<T>(n);
      --bytes;
      return -static_cast<T>(-static_cast<long>(n));
    }
  };

  //-------Unsigned, left justified----------------------------------------
  template <typename T, int N, typename Char>
  struct signness_helper<T, N, false, LEFT, Char> {
    static char* fast_itoa(Char* bytes, T i, Char pad = '\0')
    {
      char* p = bytes;
      unrolled_byte_loops<Char, N, LEFT>::save_itoa(p, i, pad);
      unrolled_byte_loops<Char, N, LEFT>::reverse(bytes, p - 1);
      if (pad) return bytes + N;
      if (p < bytes + N) *p = '\0';
      return p;
    }
    static T fast_atoi(const Char*& bytes, Char skip = '\0')
    {
      const char* p = bytes;
      if (*p == '-')
        return static_cast<T>(
            -static_cast<T>(unrolled_byte_loops<Char, N - 1, LEFT>::atoi_skip_left(++bytes, skip)));
      else
        return static_cast<T>(unrolled_byte_loops<Char, N, LEFT>::atoi_skip_left(bytes, skip));
    }
  };

  //--------------------------------------------------------------------------
  // Converting an unsigned int type to a hexadecimal string (right-aligned)
  template <typename T, int N>
  struct unrolled_loop_itoa16_right {
    typedef unrolled_loop_itoa16_right<T, N - 1> next;
    static void                                  convert(char* bytes, T val)
    {
      bytes[N - 1] = "0123456789ABCDEF"[(unsigned)(val & 0xf)];
      next::convert(bytes, val >> 4);
    }
  };
  template <typename T>
  struct unrolled_loop_itoa16_right<T, 0> {
    static void convert(char* bytes, T val)
    {
      if (val != 0) throw std::runtime_error("itoa16_right: no space for output");
    }
  };

  inline char* find_first_trailing_zero(char* p)
  {
    for (; *(p - 1) == '0'; --p);
    if (*(p - 1) == '.') ++p;
    return p;
  }

  constexpr const double s_pow10v[] = {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8, 1e9,
                                       1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18};

  enum FTOA : size_t {
    FRAC_SIZE    = 52,
    MAX_DECIMALS = (sizeof(s_pow10v) / sizeof(s_pow10v[0])),
    MAX_FLOAT    = ((size_t)1 << (FRAC_SIZE + 1))
  };

  template <typename T>
  inline typename std::enable_if<std::is_unsigned<T>::value, int>::type
  itoa_hex(T a, char*& s, size_t sz)
  {
    size_t len = a ? 0 : 1;
    if (a)
      for (T n = a; n; n >>= 4, len++);
    if (likely(len <= sz)) {
      static const char s_lookup[] = "0123456789ABCDEF";
      char*             p          = s + len;
      const char*       b          = s;
      for (*p-- = '\0'; p >= b; *p-- = s_lookup[a & 0xF], a >>= 4);
      s += len;
    }
    return len;
  }

} // namespace detail

//------------------------------------------------------------------------------
// Fixed-width integer conversion: itoa_left / itoa_right / atoi_left / atoi_right
//------------------------------------------------------------------------------

template <typename T, int N, typename Char>
static inline char* itoa_left(Char* bytes, T value, Char pad = '\0')
{
  return detail::signness_helper<T, N, std::numeric_limits<T>::is_signed, LEFT, Char>::fast_itoa(
      bytes, value, pad);
}

template <typename T, int N, typename Char>
inline char* itoa_left(Char (&bytes)[N], T value, Char pad = '\0')
{ return itoa_left<T, N, Char>(static_cast<char*>(bytes), value, pad); }

template <typename T, int Size>
inline std::string itoa_left(T value, char pad = '\0')
{
  char  buf[Size];
  char* p = itoa_left<T, Size, char>(buf, value, pad);
  return std::string(buf, p - buf);
}

template <typename T, int N, typename Char>
inline const char* atoi_left(const Char* bytes, T& value, Char skip = '\0')
{
  value = detail::signness_helper<T, N, std::numeric_limits<T>::is_signed, LEFT, Char>::fast_atoi(
      bytes, skip);
  return bytes;
}

template <typename T, int N, typename Char>
static inline Char* itoa_right(Char* bytes, T value, Char pad = '\0')
{
  return detail::signness_helper<T, N, std::numeric_limits<T>::is_signed, RIGHT, Char>::fast_itoa(
      bytes, value, pad);
}

template <typename T, int N, typename Char>
inline Char* itoa_right(Char (&bytes)[N], T value, Char pad = '\0')
{ return itoa_right<T, N, Char>(static_cast<char*>(bytes), value, pad); }

template <typename T, int Size>
inline std::string itoa_right(T value, char pad = '\0')
{
  char  buf[Size];
  char* p = itoa_right<T, Size, char>(buf, value, pad);
  return std::string(p + 1, buf + Size - p - 1);
}

template <typename T, int N, typename Char>
inline const char* atoi_right(const Char* bytes, T& value, Char skip = '\0')
{
  const char* p = bytes;
  value = detail::signness_helper<T, N, std::numeric_limits<T>::is_signed, RIGHT, Char>::fast_atoi(
      p, skip);
  return p;
}

//------------------------------------------------------------------------------
// Fallback dynamic-width itoa (no template width)
//------------------------------------------------------------------------------

template <typename T>
char* itoa(T value, char*& result, int base = 10)
{
  assert(base >= 2 && base <= 36);
  char *p = result, *q = p;
  T     tmp;
  do {
    tmp    = value;
    value /= base;
    *p++   = "zyxwvutsrqponmlkjihgfedcba987654321"
             "0123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp - value * base)];
  } while (value);
  if (tmp < 0) *p++ = '-';
  char* begin = result;
  result      = p;
  *p--        = '\0';
  while (q < p) {
    char c = *p;
    *p--   = *q;
    *q++   = c;
  }
  return begin;
}

//------------------------------------------------------------------------------
// fast_atoi
//------------------------------------------------------------------------------

template <typename T, bool TillEOL = true>
inline const char* fast_atoi(const char* a_str, const char* a_end, T& res)
{
  if (a_str >= a_end) return nullptr;
  bool l_neg;
  if (*a_str == '-') {
    l_neg = true;
    ++a_str;
  } else {
    l_neg = false;
  }
  T x = 0;
  do {
    const int c = *a_str - '0';
    if (c < 0 || c > 9) {
      if (TillEOL) return nullptr;
      break;
    }
    x = (x << 3) + (x << 1) + c;
  } while (++a_str != a_end);
  res = l_neg ? -x : x;
  return a_str;
}

template <typename T, bool TillEOL = true>
inline const char* fast_atoi(const char* a_str, size_t a_sz, T& res)
{ return fast_atoi<T, TillEOL>(a_str, a_str + a_sz, res); }

//------------------------------------------------------------------------------
// Hex conversion
//------------------------------------------------------------------------------

template <typename T, int N>
char* itoa16_right(char*& a_result, T a_value)
{
  detail::unrolled_loop_itoa16_right<T, N>::convert(a_result, a_value);
  return a_result + N;
}

template <typename T>
inline typename std::enable_if<std::numeric_limits<T>::is_integer, int>::type
itoa_hex(T a, char*& s, size_t sz)
{
  typedef typename std::make_unsigned<T>::type U;
  return detail::itoa_hex<U>(static_cast<U>(a), s, sz);
}

template <typename T>
inline std::string itoa_hex(T a)
{
  char buf[80];
  auto p = buf;
  itoa_hex(a, p, sizeof(buf));
  return buf;
}

//------------------------------------------------------------------------------
// Floating point: ftoa_left / ftoa_right
//------------------------------------------------------------------------------

template <bool WithTerminator = true, char Terminator = '\0'>
inline int ftoa_left(double f, char* buffer, int buffer_size, int precision, bool compact = true)
{
  using detail::FTOA;

  if (precision < 0) return -1;

  union {
    double f;
    size_t i;
  } a;
  bool neg;
  auto p = buffer;

  if (f < 0) {
    neg = true;
    a.f = -f;
  } else {
    neg = false;
    a.f = f;
  }

  if (a.f > FTOA::MAX_FLOAT || precision >= (int)FTOA::MAX_DECIMALS || ((a.i >> 52) >= 0x7ff)) {
    int len  = snprintf(buffer, buffer_size, "%.*f", precision, f);
    p       += len;
    if (len >= buffer_size) return -1;
    if (compact) p = detail::find_first_trailing_zero(p);
    if (WithTerminator) *p = Terminator;
    return p - buffer;
  }

  size_t int_part;
  bool   had_precision = precision != 0;

  if (precision) {
    double int_f   = std::floor(a.f);
    double frac_f  = std::round((a.f - int_f) * detail::s_pow10v[precision]);
    int_part       = size_t(int_f);
    auto frac_part = size_t(frac_f);
    if (frac_f >= detail::s_pow10v[precision]) {
      int_part++;
      frac_part = 0;
    }
    do {
      if (!frac_part) {
        do {
          *p++ = '0';
        } while (--precision);
        break;
      }
      size_t n  = frac_part / 10;
      *p++      = (char)((frac_part - n * 10) + '0');
      frac_part = n;
    } while (--precision);
    *p++ = '.';
  } else
    int_part = size_t(std::lround(a.f));

  if (!int_part)
    *p++ = '0';
  else
    do {
      size_t n = int_part / 10;
      *p++     = (char)((int_part - n * 10) + '0');
      int_part = n;
    } while (int_part);

  if (neg) *p++ = '-';

  for (int i = 0, j = p - buffer - 1; i < j; ++i, --j) {
    char tmp  = buffer[i];
    buffer[i] = buffer[j];
    buffer[j] = tmp;
  }

  if (compact && had_precision) p = detail::find_first_trailing_zero(p);
  if (WithTerminator) *p = Terminator;
  return p - buffer;
}

inline void ftoa_right(double f, char* buffer, int width, int precision, char lpad = ' ')
{
  using detail::FTOA;

  int int_width = width - (precision ? precision + 1 : 0);
  if (unlikely(precision < 0)) throw std::invalid_argument("ftoa_right: incorrect precision");
  if (unlikely(int_width < 0)) throw std::invalid_argument("ftoa_right: incorrect width");

  union {
    double f;
    size_t i;
  } a;
  int neg;

  if (f < 0) {
    neg = 1;
    a.f = -f;
  } else {
    neg = 0;
    a.f = f;
  }

  if (a.f > FTOA::MAX_FLOAT || precision >= (int)FTOA::MAX_DECIMALS || ((a.i >> 52) >= 0x7ff)) {
    int len = snprintf(buffer, width + 1, "%*.*f", width, precision, f);
    if (len >= width) throw std::invalid_argument("ftoa_right: incorrect width/precision");
    if (lpad != ' ')
      for (auto p = buffer; *p == ' '; p++) *p = lpad;
    return;
  }

  auto   p = buffer + width - 1;
  size_t int_part;

  if (precision) {
    double int_f   = std::floor(a.f);
    double frac_f  = std::round((a.f - int_f) * detail::s_pow10v[precision]);
    int_part       = size_t(int_f);
    auto frac_part = size_t(frac_f);
    if (frac_f >= detail::s_pow10v[precision]) {
      int_part++;
      frac_part = 0;
    }
    do {
      if (!frac_part) {
        do {
          *p-- = '0';
        } while (--precision);
        break;
      }
      size_t n  = frac_part / 10;
      *p--      = (char)((frac_part - n * 10) + '0');
      frac_part = n;
    } while (--precision);
    *p-- = '.';
  } else
    int_part = size_t(std::lround(a.f));

  if (!int_part) {
    if (p >= buffer) *p-- = '0';
  } else
    do {
      size_t n = int_part / 10;
      *p--     = (char)((int_part - n * 10) + '0');
      int_part = n;
    } while (int_part && p >= buffer);

  if (neg) {
    if (p < buffer) throw std::invalid_argument("ftoa_right: insufficient width for '-'");
    *p-- = '-';
  }

  if (unlikely(++p < buffer)) throw std::invalid_argument("ftoa_right: insufficient width");
  for (char* q = buffer; q != p; *q++ = lpad);
}

//------------------------------------------------------------------------------
// atof (fast floating point parse)
//------------------------------------------------------------------------------

template <typename T>
inline typename std::enable_if<
    std::is_same<T, double>::value || std::is_same<T, float>::value, const char*>::type
atof(const char* p, const char* end, T& result)
{
  assert(p != nullptr && end != nullptr);
  while (p < end && (*p == ' ' || *p == '0')) ++p;

  double sign = 1.0;
  if (*p == '-') {
    sign = -1.0;
    ++p;
  } else if (*p == '+') {
    ++p;
  }

  double  value = 0.0;
  uint8_t n     = *p - '0';
  while (p < end && n < 10u) {
    value = value * 10.0 + n;
    n     = *(++p) - '0';
  }

  if (*p == '.') {
    ++p;
    double pow10 = 1.0, acc = 0.0;
    n = *p - '0';
    while (p < end && n < 10u) {
      acc    = acc * 10.0 + n;
      n      = *(++p) - '0';
      pow10 *= 10.0;
    }
    value += acc / pow10;
  }
  result = sign * value;
  return p;
}

} // namespace utxx
