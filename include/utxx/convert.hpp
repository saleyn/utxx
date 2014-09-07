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

#ifndef _UTXX_ATOI_HPP_
#define _UTXX_ATOI_HPP_

#include <limits>
#include <string.h>
#include <stdio.h>
#include <boost/type_traits/make_signed.hpp>
#include <boost/type_traits/make_unsigned.hpp>
#include <boost/assert.hpp>
#include <string>
#include <stdexcept>
#include <utxx/meta.hpp>
#include <utxx/bits.hpp>
#include <utxx/compiler_hints.hpp>
#include <stdint.h>

namespace utxx {

enum alignment { LEFT_JUSTIFIED, RIGHT_JUSTIFIED };

namespace detail {
    static inline const char int_to_char(int n) {
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
    struct unrolled_byte_loops<Char, N, RIGHT_JUSTIFIED> {
        typedef unrolled_byte_loops<Char, N-1, RIGHT_JUSTIFIED> next;

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
    struct unrolled_byte_loops<Char, N, LEFT_JUSTIFIED> {
        typedef unrolled_byte_loops<Char, N-1, LEFT_JUSTIFIED> next;

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
    struct unrolled_byte_loops<Char, 1, RIGHT_JUSTIFIED> {
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
    struct unrolled_byte_loops<Char, 1, LEFT_JUSTIFIED> {
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
    struct unrolled_byte_loops<Char, 0, RIGHT_JUSTIFIED> {
        static uint64_t atoi_skip_right(const Char*& bytes, Char skip) { return 0; }
        static void     pad_left(Char* bytes, Char ch) {}
        static void     save_itoa(Char*& bytes, long n, Char a_pad) {}
        static uint64_t load_atoi(const Char*& bytes, uint64_t acc) { return 0; }
    };
    //-------N=0, left justified---------------------------------------------
    template <typename Char>
    struct unrolled_byte_loops<Char, 0, LEFT_JUSTIFIED> {
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
    struct signness_helper<T, N, true, RIGHT_JUSTIFIED, Char> {
        /**
         * A replacement to itoa() library function that does the job
         * 4-5 times faster. Additionally it allows skipping leading
         * characters before making the conversion.
         */
        static char* fast_itoa(Char* bytes, T i, Char pad = '\0') {
            char* p;
            if (i < 0) {
                p = signness_helper<T, N, false, RIGHT_JUSTIFIED, Char>::
                    fast_itoa(bytes, -i, pad);
                if (p >= bytes)
                    *p--= '-';
                return pad ? bytes-1 : p;
            }
            p = signness_helper<T, N, false, RIGHT_JUSTIFIED, Char>::
                fast_itoa(bytes, i, pad);
            return pad ? bytes-1 : p;
        }

        /**
         * A replacement to atoi() library function that does
         * the job 4 times faster. Additionally it allows skipping
         * leading characters before making a signness_helper.
         */
        static T fast_atoi(const Char*& bytes, Char skip = '\0') {
            const char* p = bytes;
            bytes += N - 1;
            uint64_t n = unrolled_byte_loops<Char,N,RIGHT_JUSTIFIED>::
                atoi_skip_right(bytes, skip);
            if (bytes < p || *bytes != '-')
                return static_cast<T>(n);
            --bytes;
            return -static_cast<T>(n);
        }
    };

    //-------Signed, left justified------------------------------------------
    template <typename T, int N, typename Char>
    struct signness_helper<T, N, true, LEFT_JUSTIFIED, Char> {
        static char* fast_itoa(Char* bytes, T i, Char pad = '\0') {
            char* p = bytes;
            if (i < 0) {
                *p++ = '-';
                return signness_helper<T, N-1, false, LEFT_JUSTIFIED, Char>::
                    fast_itoa(p, i, pad);
            }
            return signness_helper<T, N, false, LEFT_JUSTIFIED, Char>::
                fast_itoa(p, i, pad);
        }

        inline static T fast_atoi(const Char*& bytes, Char skip = '\0') {
            const char* p = bytes;
            if (skip && *p == skip)
                return fast_atoi(++bytes, skip);
            if (*p == '-') {
                long n = unrolled_byte_loops<Char, N-1, LEFT_JUSTIFIED>::
                    atoi_skip_left(++bytes, skip);
                return static_cast<T>(-n);
            }
            return unrolled_byte_loops<Char, N, LEFT_JUSTIFIED>::
                atoi_skip_left(bytes, skip);
        }
    };

    //-------Unsigned, right justified---------------------------------------
    template <typename T, int N, typename Char>
    struct signness_helper<T, N, false, RIGHT_JUSTIFIED, Char> {
        static char* fast_itoa(Char* bytes, T i, Char pad = '\0') {
            char* p = bytes + N - 1;
            unrolled_byte_loops<Char, N, RIGHT_JUSTIFIED>::save_itoa(p, i, pad);
            return p;
        }
        static T fast_atoi(const Char*& bytes, Char skip = '\0') {
            const Char* p = bytes;
            bytes += N - 1;
            uint64_t n = unrolled_byte_loops<Char, N, RIGHT_JUSTIFIED>::
                atoi_skip_right(bytes, skip);
            if (p > bytes || *bytes != '-')
                return static_cast<T>(n);
            --bytes;
            return -static_cast<T>(-static_cast<long>(n));
        }
    };

    //-------Unsigned, left justified----------------------------------------
    template <typename T, int N, typename Char>
    struct signness_helper<T, N, false, LEFT_JUSTIFIED, Char> {
        static char* fast_itoa(Char* bytes, T i, Char pad = '\0') {
            char* p = bytes;
            unrolled_byte_loops<Char, N, LEFT_JUSTIFIED>::save_itoa(p, i, pad);
            unrolled_byte_loops<Char, N, LEFT_JUSTIFIED>::reverse(bytes, p-1);
            if (pad) return bytes+N;
            if (p < bytes+N) *p = '\0'; // Since we have space we take advantage of it.
            return p;
        }

        static T fast_atoi(const Char*& bytes, Char skip = '\0') {
            const char* p = bytes;
            if (*p == '-') {
                return static_cast<T>(-static_cast<T>(
                    unrolled_byte_loops<Char,N-1,LEFT_JUSTIFIED>::
                    atoi_skip_left(++bytes, skip)));
            } else {
                return static_cast<T>(
                    unrolled_byte_loops<Char,N,LEFT_JUSTIFIED>::
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
 * A replacement to atoi() library function that does the job 4 times faster.
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
        T,N,std::numeric_limits<T>::is_signed, LEFT_JUSTIFIED, Char>::
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
    const char* p = bytes;
    value = detail::signness_helper<
        T,N,std::numeric_limits<T>::is_signed, LEFT_JUSTIFIED, Char>::
        fast_atoi(p, skip);
    return p;
}

template <typename T, int N, typename Char>
inline const char* atoi_left(const Char (&bytes)[N], T& value, Char skip = '\0') {
    return atoi_left<T,N,Char>(static_cast<const char*>(bytes), value, skip);
}


/**
 * A replacement to atoi() library function that does the job 4 times faster.
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
static inline char* itoa_right(Char *bytes, T value, Char pad = '\0') {
    return detail::signness_helper<T,N,std::numeric_limits<T>::is_signed,
            RIGHT_JUSTIFIED, Char>::fast_itoa(bytes, value, pad);
}

template <typename T, int N, typename Char>
inline char* itoa_right(Char (&bytes)[N], T value, Char pad = '\0') {
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
 * @return parsed integer value.
 */
template <typename T, int N, typename Char>
inline const char* atoi_right(const Char* bytes, T& value, Char skip = '\0') {
    const char* p = bytes;
    value = detail::signness_helper<T,N,std::numeric_limits<T>::is_signed,
          RIGHT_JUSTIFIED, Char>::fast_atoi(p, skip);
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

/// @param till_eol instructs that the integer must be validated till a_end.
///                 If false, "123ABC" is considered a valid 123 number. Otherwise
///                 the function will return NULL.
/// @return input string ptr beyond the the value read if successful, NULL otherwise
//
template <typename T>
const char* fast_atoi(const char* a_str, const char* a_end, T* result, bool till_eol = true) {
    if (a_str >= a_end) return NULL;

    bool l_neg;

    if (*a_str == '-') { l_neg = true; ++a_str; }
    else               { l_neg = false; }

    T x = 0;

    do {
        const int c = *a_str - '0';
        if (c < 0 || c > 9) {
            if (till_eol)
               return NULL;
            break;
        }
        x = (x << 3) + (x << 1) + c;
    } while (++a_str != a_end);

    *result = l_neg ? -x : x;
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
    T tmp;

    do {
        tmp = value;
        value /= base;
        *p++ = "zyxwvutsrqponmlkjihgfedcba987654321"
              "0123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp - value * base)];
    } while ( value );

    if (tmp < 0) *p++ = '-';

    char* begin = result;
    result = p;

    *p-- = '\0';

    // Reverse the string
    while(q < p) {
        char c = *p;
        *p--= *q;
        *q++ = c;
    }

    return begin;
}

/// Convert an unsigned number to the hexadecimal string
/// This is a special optimised case of "itoa_right" (the output string is
/// right-aligned, left-padded with '0's if necessary)
/// @return pointer to the end
template <typename T, int N>
char* itoa16_right(char* (&result), T value)
{
  detail::unrolled_loop_itoa16_right<T,N>::convert(result, value);
  return result + N;
}

template <typename T, int N>
char* itoa16_right(char (&result)[N], T value)
{
  char* buff = static_cast<char*>(result);
  detail::unrolled_loop_itoa16_right<T,N>::convert(buff, value);
  return buff + N;
}

template <typename T>
const char* fast_atoi_skip_ws(const char* a_str, size_t a_sz, T* result,
    bool a_till_eol = true)
{
    const char* l_end = a_str + a_sz;
    // Find first non-white space char by treating ' ' like '\0'
    while (!(a_str == l_end || (*a_str & 0xF))) ++a_str;
    return fast_atoi<T>(a_str, l_end, result, a_till_eol);
}

template <typename T>
const char* fast_atoi(const char* a_str, size_t a_sz, T* result,
    bool a_till_eol = true)
{
    const char* l_end = a_str + a_sz;
    return fast_atoi<T>(a_str, l_end, result, a_till_eol);
}

template <typename T>
bool fast_atoi_skip_ws(const std::string& a_str, T* result,
    bool a_till_eol = true)
{
    return fast_atoi_skip_ws<T>(a_str.c_str(), a_str.size(), result, a_till_eol)
           != NULL;
}

/// \copydetail fast_atoi()
template <typename T>
bool fast_atoi(const std::string& a_value, T* a_result,
    bool a_till_eol = true)
{
    return fast_atoi<T>(a_value.c_str(), a_value.size(), a_result, a_till_eol)
           != NULL;
}

//--------------------------------------------------------------------------------
/// Convert a floating point number to string
/// @return
//--------------------------------------------------------------------------------
int ftoa_fast(double f, char *outbuf, int maxlen, int precision, bool compact = true);

//--------------------------------------------------------------------------------
/// Parses floating point numbers with fixed number of decimal digits from string.
//
// atof:
//      09-May-2009 Tom Van Baak (tvb) www.LeapSecond.com
//      http://www.leapsecond.com/tools/fast_atof.c
//      Contributor: 2010-10-15 Serge Aleynikov
//--------------------------------------------------------------------------------
template <typename T>
const char* atof(const char* p, const char* end, T* result)
{
    BOOST_ASSERT(p != NULL && end != NULL && result != NULL);

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

    *result = sign * value;
    return p;
}

/**
 * Convert an integer to string.
 */
template <typename T>
std::string int_to_string(T n) {
    char buf[20];
    itoa_left(buf, n);
    return buf;
}

namespace detail {
    template <typename T>
    typename std::enable_if<std::is_unsigned<T>::value, int>::type
    itoa_hex(T a, char*& s, size_t sz) {
        int len = a ? 0 : 1;
        if (a)
            for (T n = a; n; n >>= 4, len++);

        if (likely(len <= sz)) {
            static const char s_lookup[] = "0123456789ABCDEF";
            char* p = s + len;
            char* b = s-1;
            for (*p-- = '\0'; p != b; a >>=4, *p-- = s_lookup[a & 0xF]);
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
typename std::enable_if<std::is_integral<T>::value, int>::type
itoa_hex(T a, char*& s, size_t sz) {
    return (std::is_signed<T>::value && a < 0)
         ? detail::itoa_hex(std::make_unsigned<T>(a), s, sz)
         : detail::itoa_hex(a, s, sz);
}

} // namespace utxx

#endif // _UTXX_ATOI_HPP_
