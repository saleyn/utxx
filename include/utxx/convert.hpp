//----------------------------------------------------------------------------
/// \file  convert.hpp
//----------------------------------------------------------------------------
/// \brief Set of convertion functions to handle fast conversion to/from
/// string.
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

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
#pragma once

#include <limits>
#include <string.h>
#include <stdio.h>
#include <boost/type_traits/make_signed.hpp>
#include <boost/type_traits/make_unsigned.hpp>
#include <boost/assert.hpp>
#include <string>
#include <stdexcept>
#include <cmath>
#include <utxx/meta.hpp>
#include <utxx/bits.hpp>
#include <utxx/types.hpp>
#include <utxx/compiler_hints.hpp>
#include <stdint.h>

namespace utxx {

namespace detail {
    static inline char int_to_char(int n) {
        static const char  s_characters[] = { "9876543210123456789" };
        static const char* s_middle = s_characters + 9;
        return s_middle[n];
    }

    template<typename Char, typename I, bool Sign, alignment Align, Char Skip>
    struct convert;

    //-----------------------------------------------------------------------
    // Unrolled loops for converting stream of bytes.
    //-----------------------------------------------------------------------
    template <typename Char, int N, alignment Align>
    struct unrolled_byte_loops;

    //-------N=k, right justified--------------------------------------------
    template <typename Char, int N>
    struct unrolled_byte_loops<Char, N, RIGHT> {
        typedef unrolled_byte_loops<Char, N-1, RIGHT> next;

        inline static uint64_t atoi_skip_right(const Char*& bytes, Char skip) {
            if (skip && *bytes == skip)
                return next::atoi_skip_right(--bytes, skip);
            return load_atoi(bytes, 0);
        }

        static void pad_left(Char* bytes, Char ch) {
            *bytes = ch;
            next::pad_left(--bytes, ch);
        }

        inline static void save_itoa(Char*& bytes, long n, Char a_pad) {
            long m = n / 10;
            *bytes-- = int_to_char(n - m*10);
            if (m == 0) {
                if (a_pad)
                    next::pad_left(bytes, a_pad);
                return;
            }
            next::save_itoa(bytes, m, a_pad);
        }

        static uint64_t load_atoi(const Char*& bytes, uint64_t acc) {
            typename boost::make_unsigned<Char>::type i=*bytes-'0';
            return i > 9u ? 0 : i + 10 * next::load_atoi(--bytes, acc);
        }
    };

    //-------N=k, left justified--------------------------------------------
    template <typename Char, int N>
    struct unrolled_byte_loops<Char, N, LEFT> {
        typedef unrolled_byte_loops<Char, N-1, LEFT> next;

        inline static uint64_t atoi_skip_left(const Char*& bytes, Char skip) {
            if (skip && *bytes == skip)
                return next::atoi_skip_left(++bytes, skip);
            return load_atoi(bytes, 0);
        }

        static void reverse(Char* p, Char* q) {
            if (p >= q) return;
            Char c = *p; *p = *q; *q = c;
            next::reverse(++p, --q);
        }

        static void pad_right(Char* bytes, Char ch) {
            *bytes = ch;
            next::pad_right(++bytes, ch);
        }

        static void save_itoa(Char*& bytes, long n, Char a_pad) {
            long m = n / 10;
            *bytes++ = int_to_char(n - m*10);
            if (m == 0) {
                if (a_pad)
                    next::pad_right(bytes, a_pad);
                else
                    *bytes = '\0';
                return;
            }
            next::save_itoa(bytes, m, a_pad);
        }

        static uint64_t load_atoi(const Char*& bytes, uint64_t acc) {
            typename boost::make_unsigned<Char>::type i=*bytes-'0';
            return (i > 9u) ? acc : next::load_atoi(++bytes, i + 10*acc);
        }
    };

    //-------N=1, right justified--------------------------------------------
    template <typename Char>
    struct unrolled_byte_loops<Char, 1, RIGHT> {
        inline static uint64_t atoi_skip_right(const Char*& bytes, Char skip) {
            if (skip && *bytes == skip) { --bytes; return 0; }
            return load_atoi(bytes, 0);
        }

        static void pad_left(Char* bytes, Char ch) { *bytes = ch; }

        inline static void save_itoa(Char*& bytes, long n, Char a_pad) {
            BOOST_ASSERT(n / 10 == 0);
            *bytes-- = int_to_char(n % 11);
        }

        static uint64_t load_atoi(const Char*& bytes, uint64_t acc) {
            typename boost::make_unsigned<Char>::type i=*bytes-'0';
            if (i > 9u) return 0;
            --bytes;
            return i;
        }
    };

    //-------N=1, left justified---------------------------------------------
    template <typename Char>
    struct unrolled_byte_loops<Char, 1, LEFT> {
        inline static uint64_t atoi_skip_left(const Char*& bytes, Char skip) {
            if (skip && *bytes == skip) { ++bytes; return 0; }
            return load_atoi(bytes, 0);
        }

        static void pad_right(Char* bytes, Char ch) { *bytes = ch; }
        static void reverse(Char* p, Char* q) {}

        static void save_itoa(Char*& bytes, long n, Char a_pad) {
            BOOST_ASSERT(n / 10 == 0);
            *bytes++ = int_to_char(n % 10);
        }

        static uint64_t load_atoi(const Char*& bytes, uint64_t acc) {
            typename boost::make_unsigned<Char>::type i=*bytes-'0';
            if (i > 9u) return acc;
            ++bytes;
            return i + 10*acc;
        }
    };

    //-------N=0, right justified--------------------------------------------
    template <typename Char>
    struct unrolled_byte_loops<Char, 0, RIGHT> {
        static uint64_t atoi_skip_right(const Char*& bytes, Char skip) { return 0; }
        static void     pad_left(Char* bytes, Char ch) {}
        static void     save_itoa(Char*& bytes, long n, Char a_pad) {}
        static uint64_t load_atoi(const Char*& bytes, uint64_t acc) { return 0; }
    };
    //-------N=0, left justified---------------------------------------------
    template <typename Char>
    struct unrolled_byte_loops<Char, 0, LEFT> {
        static uint64_t atoi_skip_left(const Char*& bytes, Char skip) { return 0; }
        static void     pad_right(Char* bytes, Char ch) {}
        static void     reverse(Char* p, Char* q) {}
        static void     save_itoa(Char*& bytes, long n, Char a_pad) {}
        static uint64_t load_atoi(const Char*& bytes, uint64_t acc) { return 0; }
    };

    //-----------------------------------------------------------------------
    // Conversion helper class
    //-----------------------------------------------------------------------
    template <typename T, int N, bool Sign, alignment Justify, typename Char>
    struct signness_helper;

    //-------Signed, right justified-----------------------------------------
    template <typename T, int N, typename Char>
    struct signness_helper<T, N, true, RIGHT, Char> {
        /**
         * A replacement to itoa() library function that does the job
         * 4-5 times faster. Additionally it allows skipping leading
         * characters before making the conversion.
         */
        static char* fast_itoa(Char* bytes, T i, Char pad = '\0') {
            char* p;
            if (i < 0) {
                p = signness_helper<T, N, false, RIGHT, Char>::
                    fast_itoa(bytes, -i, pad);
                if (p >= bytes)
                    *p--= '-';
                return pad ? bytes-1 : p;
            }
            p = signness_helper<T, N, false, RIGHT, Char>::
                fast_itoa(bytes, i, pad);
            return pad ? bytes-1 : p;
        }

        /**
         * A replacement to atoi() library function that does
         * the job 4 times faster. Additionally it allows skipping
         * leading characters before invoking a signness_helper.
         */
        static T fast_atoi(const Char*& bytes, Char skip = '\0') {
            const char* p = bytes;
            bytes += N - 1;
            uint64_t n = unrolled_byte_loops<Char,N,RIGHT>::
                atoi_skip_right(bytes, skip);
            if (bytes < p || *bytes != '-')
                return static_cast<T>(n);
            --bytes;
            return -static_cast<T>(n);
        }
    };

    //-------Signed, left justified------------------------------------------
    template <typename T, int N, typename Char>
    struct signness_helper<T, N, true, LEFT, Char> {
        static char* fast_itoa(Char* bytes, T i, Char pad = '\0') {
            char* p = bytes;
            if (i < 0) {
                *p++ = '-';
                return signness_helper<T, N-1, false, LEFT, Char>::
                    fast_itoa(p, i, pad);
            }
            return signness_helper<T, N, false, LEFT, Char>::
                fast_itoa(p, i, pad);
        }

        inline static T fast_atoi(const Char*& bytes, Char skip = '\0') {
            auto  e = bytes + N;
            if (skip) {
                while (*bytes == skip && bytes != e) ++bytes;
                switch (e - bytes) {
                    case  0: return 0;
                    case  1: return fast_atoi2< 1>(bytes);
                    case  2: return fast_atoi2< 2>(bytes);
                    case  3: return fast_atoi2< 3>(bytes);
                    case  4: return fast_atoi2< 4>(bytes);
                    case  5: return fast_atoi2< 5>(bytes);
                    case  6: return fast_atoi2< 6>(bytes);
                    case  7: return fast_atoi2< 7>(bytes);
                    case  8: return fast_atoi2< 8>(bytes);
                    case  9: return fast_atoi2< 9>(bytes);
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
        inline static T fast_atoi2(const Char*& bytes) {
            if (*bytes == '-') {
                long n = unrolled_byte_loops<Char, N-1, LEFT>::
                    load_atoi(++bytes, 0);
                return static_cast<T>(-n);
            }
            return unrolled_byte_loops<Char, N, LEFT>::load_atoi(bytes, 0);
        }
    };

    //-------Unsigned, right justified---------------------------------------
    template <typename T, int N, typename Char>
    struct signness_helper<T, N, false, RIGHT, Char> {
        static char* fast_itoa(Char* bytes, T i, Char pad = '\0') {
            char* p = bytes + N - 1;
            unrolled_byte_loops<Char, N, RIGHT>::save_itoa(p, i, pad);
            return p;
        }
        static T fast_atoi(const Char*& bytes, Char skip = '\0') {
            const Char* p = bytes;
            bytes += N - 1;
            uint64_t n = unrolled_byte_loops<Char, N, RIGHT>::
                atoi_skip_right(bytes, skip);
            if (p > bytes || *bytes != '-')
                return static_cast<T>(n);
            --bytes;
            return -static_cast<T>(-static_cast<long>(n));
        }
    };

    //-------Unsigned, left justified----------------------------------------
    template <typename T, int N, typename Char>
    struct signness_helper<T, N, false, LEFT, Char> {
        static char* fast_itoa(Char* bytes, T i, Char pad = '\0') {
            char* p = bytes;
            unrolled_byte_loops<Char, N, LEFT>::save_itoa(p, i, pad);
            unrolled_byte_loops<Char, N, LEFT>::reverse(bytes, p-1);
            if (pad) return bytes+N;
            if (p < bytes+N) *p = '\0'; // Since we have space we take advantage of it.
            return p;
        }

        static T fast_atoi(const Char*& bytes, Char skip = '\0') {
            const char* p = bytes;
            if (*p == '-') {
                return static_cast<T>(-static_cast<T>(
                    unrolled_byte_loops<Char,N-1,LEFT>::
                    atoi_skip_left(++bytes, skip)));
            } else {
                return static_cast<T>(
                    unrolled_byte_loops<Char,N,LEFT>::
                    atoi_skip_left(bytes, skip));
            }
        }
    };

    //-------------------------------------------------------------------------
    // Similar but simplier approach with no error checking
    // The number is assumed to be right-justified.
    template <int N>
    struct unrolled_loop_atoul {
        inline static void convert(const char*& p, uint64_t& n) {
            //BOOST_ASSERT(*p >= '0' && *p <= '9' || *p == ' ');
            n = n*10 + (*(p++) & 0x0f);  // ' ' is treated the same as '0'
            unrolled_loop_atoul<N-1>::convert(p, n);
        }
        inline static uint64_t convert(const char*& p) {
            //BOOST_ASSERT(*p >= '0' && *p <= '9' || *p == ' ');
            uint64_t n = *p++ & 0x0f;
            unrolled_loop_atoul<N-1>::convert(p, n);
            return n;
        }
        inline static uint64_t convert_unsigned(const char*& p) {
            if (*p == ' ')
                return unrolled_loop_atoul<N-1>::convert_unsigned(++p);
            if (*p < '0' || *p > '9')
                return 0u;
            return unrolled_loop_atoul<N>::convert(p);
        }
        inline static int64_t convert_signed(const char*& p) {
            if (*p == ' ')
                return unrolled_loop_atoul<N-1>::convert_signed(++p);
            if (*p == '-')
                return -static_cast<int64_t>(unrolled_loop_atoul<N-1>::convert(++p));
            if (*p < '0' || *p > '9')
                return 0;
            return unrolled_loop_atoul<N>::convert(p);
        }
    };

    template <>
    struct unrolled_loop_atoul<1> {
        inline static void convert(const char*& p, uint64_t& n) {
            //BOOST_ASSERT(*p >= '0' && *p <= '9' || *p == ' ');
            n = n*10 + (*p++ & 0x0f);
        }
        inline static uint64_t convert(const char*& p) {
            //BOOST_ASSERT(*p >= '0' && *p <= '9' || *p == ' ');
            return *p++ & 0x0f;
        }
        inline static uint64_t convert_unsigned(const char*& p) {
            return convert(p);
        }
        inline static int64_t convert_signed(const char*& p) {
            return convert(p);
        }
    };

    template <>
    struct unrolled_loop_atoul<0>;

    //--------------------------------------------------------------------------
    // Converting an insigned int type to a hexadecimal string
    // The output is of length "N", right-adjusted, left-filled with 0s:
    //
    template <typename T, int N>
    struct unrolled_loop_itoa16_right
    {
      typedef unrolled_loop_itoa16_right<T, N-1> next;

      static void convert(char* bytes, T val)
      {
        unsigned int ind = (unsigned int)(val & 0xf);
        bytes[N-1] = "0123456789ABCDEF"[ind];
        next::convert(bytes, val >> 4);
      }
    };

    template<typename T>
    struct unrolled_loop_itoa16_right<T, 0>
    {
      static void convert(char* bytes, T val)
      {
        if (val != 0)
          throw std::runtime_error("utxx::detail::unrolled_loop_utoa16_left::"
                                   "convert: No space for output");
      }
    };

    //-------------------------------------------------------------------------
    // For internal use by ftoa_left(), ftoa_right():
    //
    inline char* find_first_trailing_zero(char* p)
    {
      for (; *(p-1) == '0'; --p);
      if (*(p-1) == '.') ++p;
      return p;
    }

    constexpr const double s_pow10v[] = {
        1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8, 1e9,
        1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18
    };

    enum FTOA : size_t {
          FRAC_SIZE    = 52
        , MAX_DECIMALS = (sizeof(s_pow10v) / sizeof(s_pow10v[0]))
        , MAX_FLOAT    = ((size_t)1 << (FRAC_SIZE+1))
    };
/*
    //constexpr double cs_rnd = 0.55555555555555555;
    constexpr double cs_rnd = 0.5;

    constexpr double cs_pow10_rnd[] = {
      cs_rnd / 1ll,
      cs_rnd / 10ll,
      cs_rnd / 100ll,
      cs_rnd / 1000ll,
      cs_rnd / 10000ll,
      cs_rnd / 100000ll,
      cs_rnd / 1000000ll,
      cs_rnd / 10000000ll,
      cs_rnd / 100000000ll,
      cs_rnd / 1000000000ll,
      cs_rnd / 10000000000ll,
      cs_rnd / 100000000000ll,
      cs_rnd / 1000000000000ll,
      cs_rnd / 10000000000000ll,
      cs_rnd / 100000000000000ll,
      cs_rnd / 1000000000000000ll,
      cs_rnd / 10000000000000000ll,
      cs_rnd / 100000000000000000ll,
      cs_rnd / 1000000000000000000ll
    };

    enum FTOA : size_t {
          FRAC_SIZE     = 52
        , EXP_SIZE      = 11
        , EXP_MASK      = (1ll << EXP_SIZE) - 1
        , FRAC_MASK     = (1ll << FRAC_SIZE) - 1
        , FRAC_MASK2    = (1ll << (FRAC_SIZE + 1)) - 1
        , MAX_FLOAT     = 1ll << (FRAC_SIZE+1)
        , MAX_DECIMALS  = sizeof(cs_pow10_rnd) / sizeof(cs_pow10_rnd[0])
    };
    */
} // namespace detail

/// This function converts a fixed-length string to integer.
/// The integer must be left padded with spaces or zeros.
/// Note: It performs no error checking!!!
template <int N>
inline uint64_t unsafe_fixed_atoul(const char*& p) {
    return detail::unrolled_loop_atoul<N>::convert_unsigned(p);
}

template <int N>
inline const char* unsafe_fixed_atoul(const char* p, uint64_t& value) {
    const char* q = p;
    value = detail::unrolled_loop_atoul<N>::convert_unsigned(q);
    return q;
}

/// This function converts a fixed-length string to integer.
/// The integer must be left padded with spaces or zeros.
/// Note: It performs no error checking!!!
template <int N>
inline int64_t unsafe_fixed_atol(const char*& p) {
    return detail::unrolled_loop_atoul<N>::convert_signed(p);
}

template <int N>
inline const char* unsafe_fixed_atol(const char* p, int64_t& value) {
    const char* q = p;
    value = detail::unrolled_loop_atoul<N>::convert_signed(q);
    return q;
}

/**
 * A replacement to itoa() library function that does the job 4 times faster.
 * The value written is aligned on the left and padded on the right with \a pad
 * character, unless it is '\0'.
 * @return Pointer above the rightmost character (value or pad) written.
 * @code
 *   E.g.
 *   itoa<int, 5>(buf, 1234) -> "1234  "
 *                                   ^
 *                                   +--- return pointer points to buf+4.
 * @endcode
 */
template <typename T, int N, typename Char>
static inline char* itoa_left(Char *bytes, T value, Char pad = '\0') {
    return detail::signness_helper<
        T,N,std::numeric_limits<T>::is_signed, LEFT, Char>::
        fast_itoa(bytes, value, pad);
}

template <typename T, int N, typename Char>
inline char* itoa_left(Char (&bytes)[N], T value, Char pad = '\0') {
    return itoa_left<T,N,Char>(static_cast<char*>(bytes), value, pad);
}

/**
 * Convert integer to std::string.
 */
template <typename T, int Size>
inline std::string itoa_left(T value, char pad = '\0') {
    char buf[Size];
    char* p = itoa_left<T>(buf, value, pad);
    return std::string(buf, p - buf);
}



/**
 * A faster replacement to atoi() library function.
 * Additionally it allows skipping leading characters before making a conversion.
 * @param bytes will be set to the end of parsed value.
 * @return pointer past the last parsed character in the input string.
 */
template <typename T, int N, typename Char>
inline const char* atoi_left(const Char* bytes, T& value, Char skip = '\0') {
    value = detail::signness_helper<
        T,N,std::numeric_limits<T>::is_signed, LEFT, Char>::
        fast_atoi(bytes, skip);
    return bytes;
}

template <typename T, int N, typename Char>
inline const char* atoi_left(const Char (&bytes)[N], T& value, Char skip = '\0') {
    return atoi_left<T,N,Char>(static_cast<const char*>(bytes), value, skip);
}


/**
 * A replacement to itoa() library function that does the job 4 times faster.
 * The value written is aligned on the right and padded on the left with \a pad
 * character, unless it is '\0'.
 * @return Pointer below the leftmost character (value or pad) written.
 * @code
 *   E.g.
 *   itoa<int, 5>(buf, 1234) -> "  1234"
 *                                ^
 *                                +--- return pointer points to buf+1.
 * @endcode
 */
template <typename T, int N, typename Char>
static inline Char* itoa_right(Char *bytes, T value, Char pad = '\0') {
    return detail::signness_helper<T,N,std::numeric_limits<T>::is_signed,
            RIGHT, Char>::fast_itoa(bytes, value, pad);
}

template <typename T, int N, typename Char>
inline Char* itoa_right(Char (&bytes)[N], T value, Char pad = '\0') {
    return itoa_right<T,N,Char>(static_cast<char*>(bytes), value, pad);
}

/**
 * Convert integer to std::string.
 */
template <typename T, int Size>
inline std::string itoa_right(T value, char pad = '\0') {
    char buf[Size];
    char* p = itoa_right<T>(buf, value, pad);
    return std::string(p+1, buf+Size - p - 1);
}

/**
 * A faster replacement to atoi() library function.
 * Additionally it allows skipping trailing characters before making a conversion.
 * @param bytes will be set to the end of parsed value.
 * @return pointer past the last parsed character in the input string.
 */
template <typename T, int N, typename Char>
inline const char* atoi_right(const Char* bytes, T& value, Char skip = '\0') {
    const char* p = bytes;
    value = detail::signness_helper<T,N,std::numeric_limits<T>::is_signed,
          RIGHT, Char>::fast_atoi(p, skip);
    return p;
}

template <typename T, int N, typename Char>
inline const char* atoi_right(const Char (&bytes)[N], T& value, Char skip = '\0') {
    return atoi_right<T,N,Char>(static_cast<const char*>(bytes), value, skip);
}

//--------------------------------------------------------------------------------
/// Fallback implementation of itoa. Prints \a a_value into \a a_data buffer
/// right-adjusted, left padded with \a a_pad character.
/// @return pointer to the beginning of the buffer.
// 2010-10-15 Serge Aleynikov
//--------------------------------------------------------------------------------
template <typename T, typename Char>
inline Char* itoa_right(Char* a_data, size_t a_size, T a_value, Char a_pad = '\0') {
    BOOST_ASSERT(a_size > 0);
    Char* p = a_data + a_size - 1;
    while (p >= a_data) {
        T n = a_value / 10;
        *p-- = '0' + a_value - n * 10;
        a_value = n;
        if (a_value == 0)
            break;
    }

    if (a_pad == '\0')
        return p;
    else {
        while (p >= a_data)
            *p-- = a_pad;
        return a_data;
    }
}

/// @tparam TillEOL instructs that the integer must be validated till a_end.
///                 If false, "123ABC" is considered a valid 123 number. Otherwise
///                 the function will return NULL.
/// @return input string ptr beyond the the value read if successful, NULL otherwise
//
template <typename T, bool TillEOL = true>
inline const char* fast_atoi(const char* a_str, const char* a_end, T& res) {
    if (a_str >= a_end) return nullptr;

    bool l_neg;

    if (*a_str == '-') { l_neg = true; ++a_str; }
    else               { l_neg = false; }

    T x = 0;

    do {
        const int c = *a_str - '0';
        if (c < 0 || c > 9) {
            if (TillEOL)
               return nullptr;
            break;
        }
        x = (x << 3) + (x << 1) + c;
    } while (++a_str != a_end);

    res = l_neg ? -x : x;
    return a_str;
}

/// Convert a number to string
///
/// Note: the function doesn't perform boundary checking. Make sure there's enough
/// space in the \a result buffer (10 bytes for 32bit, and 20 bytes for 64bit)
/// @return pointer to the beginning of the string; "result" contains ptr to the
/// end:
template <typename T>
char* itoa(T value, char*& result, int base = 10) {
    BOOST_ASSERT(base >= 2 || base <= 36);

    char* p = result, *q = p;
    T     tmp;

    do {
        tmp    = value;
        value /= base;
        *p++   = "zyxwvutsrqponmlkjihgfedcba987654321"
                "0123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp - value * base)];
    } while (value);

    if (tmp < 0) *p++ = '-';

    char* begin = result;
    result = p;

    *p-- = '\0';

    // Reverse the string
    while(q < p) {
        char c = *p;
        *p--   = *q;
        *q++   =  c;
    }

    return begin;
}

/// Convert an unsigned number to the hexadecimal string
/// This is a special optimised case of "itoa_right" (the output string is
/// right-aligned, left-padded with '0's if necessary)
/// @return pointer to the end
template <typename T, int N>
char* itoa16_right(char*& a_result, T a_value)
{
  detail::unrolled_loop_itoa16_right<T,N>::convert(a_result, a_value);
  return a_result + N;
}

template <typename T, int N>
char* itoa16_right(char (&a_result)[N], T a_value)
{
  char* buff = static_cast<char*>(a_result);
  detail::unrolled_loop_itoa16_right<T,N>::convert(buff, a_value);
  return buff + N;
}

template <typename T, bool TillEOL = true>
const char* fast_atoi_skip_ws(const char* a_str, size_t a_sz, T& res)
{
    const char* l_end = a_str + a_sz;
    // Find first non-white space char by treating ' ' like '\0'
    while (!(a_str == l_end || (*a_str & 0xF))) ++a_str;
    return fast_atoi<T, TillEOL>(a_str, l_end, res);
}

template <typename T, bool TillEOL = true>
const char* fast_atoi(const char* a_str, size_t a_sz, T& res)
{
    const char* l_end = a_str + a_sz;
    return fast_atoi<T, TillEOL>(a_str, l_end, res);
}

template <typename T, bool TillEOL = true>
bool fast_atoi_skip_ws(const std::string& a_str, T& res)
{
    return fast_atoi_skip_ws<T, TillEOL>(a_str.c_str(), a_str.size(), res);
}

/// \copydetail fast_atoi()
template <typename T, bool TillEOL = true>
bool fast_atoi(const std::string& a_value, T& a_res)
{
    return fast_atoi<T, TillEOL>(a_value.c_str(), a_value.size(), a_res);
}

//--------------------------------------------------------------------------------
// Floating point formatting
//--------------------------------------------------------------------------------

/// Convert a floating point number to a left-justified zero-terminated string
/// @param f is the number to convert
/// @param buffer is the output buffer
/// @param buffer_size is the size of the output buffer
/// @param precision is the number of digits past the decimal point
/// @param compact when true extra trailing 0's will be truncated
/// @return number of digits written or -1 on error
template <bool WithTerminator = true, char Terminator = '\0'>
inline int ftoa_left(double f, char* buffer, int buffer_size, int precision, bool compact = true)
{
    using detail::FTOA;

    if (precision < 0)
        return -1;

    union {double f; size_t i;} a;
    bool   neg;
    auto   p = buffer;

    if (f < 0) {
        neg = true;
        a.f = -f;
    } else {
        neg = false;
        a.f = f;
    }

    // Don't bother with optimizing too large numbers or too large precision
    // Note that a.i>>52 test below is a faster check for NAN and INF than
    // calling std::isnan() and std::isinf()
    if (a.f > FTOA::MAX_FLOAT || precision >= long(FTOA::MAX_DECIMALS) ||
       ((a.i >> 52) >= 0x7ff))
    {
        int len = snprintf(buffer, buffer_size, "%.*f", precision, f);
        p += len;
        if (len >= buffer_size)
            return -1;
        // Delete trailing zeroes
        if (compact)
            p = detail::find_first_trailing_zero(p);
        if (WithTerminator)
            *p = Terminator;
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
            // rounding overflow carry into int_part
            int_part++;
            frac_part = 0;
        }

        do {
            if (!frac_part) {
                do { *p++ = '0'; } while (--precision);
                break;
            }
            size_t n = frac_part / 10;
            *p++ = (char)((frac_part - n*10) + '0');
            frac_part = n;
        } while (--precision);

        *p++ = '.';
    }
    else
        int_part = size_t(std::lround(a.f));

    if (!int_part)
        *p++ = '0';
    else do {
        size_t n = int_part / 10;
        *p++ = (char)((int_part - n*10) + '0');
        int_part = n;
    } while (int_part);

    if (neg)
        *p++ = '-';

    // Reverse string
    for (int i=0, j=p-buffer-1; i < j; ++i, --j) {
        char  tmp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = tmp;
    }

    // Delete trailing zeroes
    if (compact && had_precision)
        p = detail::find_first_trailing_zero(p);
    if (WithTerminator)
        *p = Terminator;
    return p - buffer;
}

/// Convert a floating point number to a right-justified non-zero-terminated string
/// @param f is the number to convert
/// @param buffer is the output buffer
/// @param width is the width of the width of formatted output (buffer space
///              must be sufficient!)
/// @param precision is the number of digits past the decimal point
/// @param lpad      is the left-padding character
inline void ftoa_right(double f, char* buffer, int width, int precision, char lpad = ' ')
{
    using detail::FTOA;

    int int_width = width - (precision ? precision+1 : 0);
    if (unlikely(precision < 0))
        throw std::invalid_argument("ftoa_right: incorrect precision");
    if (unlikely(int_width < 0))
        throw std::invalid_argument("ftoa_right: incorrect width");

    union {double f; size_t i;} a;
    int neg;

    if (f < 0) {
        neg = 1;
        a.f = -f;
    } else {
        neg = 0;
        a.f = f;
    }

    // Don't bother with optimizing too large numbers or too large precision
    if (a.f > FTOA::MAX_FLOAT || precision >= long(FTOA::MAX_DECIMALS) ||
        ((a.i >> 52) >= 0x7ff)) {
        int len = snprintf(buffer, width+1, "%*.*f", width, precision, f);
        if (len >= width)
            throw std::invalid_argument("ftoa_right: incorrect width or precision");
        // Add padding
        if (lpad != ' ')
            for (auto p = buffer; *p == ' '; p++)
              *p = lpad;
        return;
    }

    auto p = buffer + width - 1;
    size_t int_part;

    if (precision) {
        double int_f   = std::floor(a.f);
        double frac_f  = std::round((a.f - int_f) * detail::s_pow10v[precision]);

        int_part       = size_t(int_f);
        auto frac_part = size_t(frac_f);

        if (frac_f >= detail::s_pow10v[precision]) {
            // rounding overflow carry into int_part
            int_part++;
            frac_part = 0;
        }

        do {
            if (!frac_part) {
                do { *p-- = '0'; } while (--precision);
                break;
            }
            size_t n = frac_part / 10;
            *p-- = (char)((frac_part - n*10) + '0');
            frac_part = n;
        } while (--precision);

        *p-- = '.';
    }
    else
        int_part = size_t(std::lround(a.f));

    if (!int_part) {
        if (p >= buffer) *p-- = '0';
    } else do {
        size_t n = int_part / 10;
        *p-- = (char)((int_part - n*10) + '0');
        int_part = n;
    } while (int_part && p >= buffer);

    if (neg) {
      if (p < buffer)
        throw std::invalid_argument("ftoa_right: insufficient width for '-' sign");
      *p-- = '-';
    }

    if (unlikely(++p < buffer))
        throw std::invalid_argument("ftoa_right: insufficient width");
    // Left-pad
    for (char* q = buffer; q != p; *q++ = lpad);
}

//--------------------------------------------------------------------------------
/// Parses floating point numbers with fixed number of decimal digits from string.
/// @return pointer past the last successfully parsed character or \a p if
/// nothing was parsed.
//
// atof:
//      09-May-2009 Tom Van Baak (tvb) www.LeapSecond.com
//      http://www.leapsecond.com/tools/fast_atof.c
//      Contributor: 2010-10-15 Serge Aleynikov
//--------------------------------------------------------------------------------
template <typename T>
inline typename std::enable_if<std::is_same<T,double>::value ||
                               std::is_same<T,float >::value, const char*>::
type atof(const char* p, const char* end, T& result)
{
    BOOST_ASSERT(p != nullptr && end != nullptr);

    // Skip leading white space, if any.
    while (p < end && (*p == ' ' || *p == '0'))
        ++p;

    // Get sign, if any.

    double sign = 1.0;

    if (*p == '-') {
        sign = -1.0;
        ++p;
    } else if (*p == '+') {
        ++p;
    }

    // Get digits before decimal point or exponent, if any.

    double value = 0.0;
    uint8_t n = *p - '0';
    while (p < end && n < 10u) {
        value = value * 10.0 + n;
        n = *(++p) - '0';
    }

    // Get digits after decimal point, if any.

    if (*p == '.') {
        ++p;

        double pow10 = 1.0;
        double acc   = 0.0;
        n = *p - '0';
        while (p < end && n < 10u) {
            acc = acc * 10.0 + n;
            n = *(++p) - '0';
            pow10 *= 10.0;
        }
        value += acc / pow10;
    }

    result = sign * value;
    return p;
}

/// Convert an integer to string.
template <typename T>
inline typename std::enable_if<std::numeric_limits<T>::is_integer, std::string>::
type int_to_string(T n) {
    static const int s_size =
        sizeof(T) == 1 ?  2 :
        sizeof(T) == 2 ?  6 :
        sizeof(T) == 4 ? 10 :
        sizeof(T) == 8 ? 20 : -1;
    static_assert(s_size != -1, "Invalid type T");
    char buf[s_size];
    itoa_left(buf, n);
    return buf;
}

namespace detail {
    template <typename T>
    inline typename std::enable_if<std::is_unsigned<T>::value, int>::type
    itoa_hex(T a, char*& s, size_t sz) {
        size_t len = a ? 0 : 1;
        if (a)
            for (T n = a; n; n >>= 4, len++);

        if (likely(len <= sz)) {
            static const char s_lookup[] = "0123456789ABCDEF";
            char* p = s + len;
            const char* b = s;
            for (*p-- = '\0'; p >= b; *p-- = s_lookup[a & 0xF], a >>=4);
            s += len;
        }
        return len;
    }
} // namespace detail

//--------------------------------------------------------------------------------
/// Convert an integer to hex string
/// @return number of bytes that WOULD BE NEEDED to write a hex representaion of
///         \a a. If it exceeds \a sz - nothing is written.
//--------------------------------------------------------------------------------
template <typename T>
inline typename std::enable_if<std::numeric_limits<T>::is_integer, int>::
type itoa_hex(T a, char*& s, size_t sz) {
    typedef typename std::make_unsigned<T>::type U;
    return detail::itoa_hex<U>(a, s, sz);
}

//--------------------------------------------------------------------------------
/// Convert an integer to hex string
//--------------------------------------------------------------------------------
template <typename T>
inline std::string itoa_hex(T a) {
    char buf[80];
    auto p = buf;
    itoa_hex(a, p, sizeof(buf));
    return buf;
}

//--------------------------------------------------------------------------------
/// Print bits of \a a_val to a_buf
/// \tparam MostSignificantFirst when true, \a a_drop_trailing_zeros are removed
///                              from right side, otherwise from left side
/// \tparam MaxOctets prints bits if the number fits in at most these number
///                   of octets, otherwise prints hex. If this number is
///                   negative, prints hex.
//--------------------------------------------------------------------------------
template <typename T, bool MostSignificantFirst=true, int MaxOctets=sizeof(T)>
typename std::enable_if<std::is_integral<T>::value, int>::
type itoa_bits(char* a_buf, int a_sz, T a_val, bool a_drop_trailing_zeros=false) {
    static constexpr const int s_size = 8*sizeof(T);
    static_assert(MaxOctets < 0 || MaxOctets <= sizeof(T), "Invalid size");
    assert((MaxOctets < 0 && a_sz >= s_size+9) || (a_sz >= MaxOctets*9+1));

    int lsb = __builtin_ffsl(a_val); // 1 + position of the least significant 1.
    if(!lsb) { strncpy(a_buf, "", a_sz); return 0; }
    int msb = __builtin_clzl(a_val); // Number of leading 0.

    auto p  = a_buf;
    int len = s_size
            - (a_drop_trailing_zeros
                ? ((((MostSignificantFirst ? lsb : msb))+7)&~7)
                : 0);
    if (len/8 > MaxOctets || MaxOctets < 0) {
        *p++ = '0'; *p++ = 'x';
        return itoa_hex(a_val, p, a_sz)+2;
    }

    int from = MostSignificantFirst ? s_size : len;
    int to   = MostSignificantFirst ? s_size-std::min(64,len+8) : 0;

    // print bits
    for(int i  = from; i > to; --i) {
        if (i != from && i%8 == 0) *p++ ='-';
        *p++ = (a_val & (1ul << (i-1))) ? '1' : '0';
    }

    *p = '\0';
    assert(p <= a_buf+a_sz);
    return p-a_buf;
}

//--------------------------------------------------------------------------------
/// Print bits of \a a_val to a_buf
/// \tparam MostSignificantFirst when true, \a a_drop_trailing_zeros are removed
///                              from right side, otherwise from left side
/// \tparam MaxOctets prints bits if the number fits in at most these number
///                   of octets, otherwise prints hex. If this number is
///                   negative, prints hex.
//--------------------------------------------------------------------------------
template <typename T, bool MostSignificantFirst=true, int MaxOctets=sizeof(T)>
std::string itoa_bits(T a_val, bool a_drop_trailing_zeros=false) {
    char buf[96];
    itoa_bits<T, MostSignificantFirst, MaxOctets>
        (buf, sizeof(buf), a_val, a_drop_trailing_zeros);
    return std::string(buf);
}

} // namespace utxx
