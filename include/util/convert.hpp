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

This file may be included in various open-source projects.

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

#ifndef _UTIL_ATOI_HPP_
#define _UTIL_ATOI_HPP_

#include <limits>
#include <string.h>
#include <stdio.h>
#include <boost/type_traits/make_signed.hpp>
#include <boost/type_traits/make_unsigned.hpp>
#include <boost/assert.hpp>
#include <string>
#include <util/meta.hpp>
#include <stdint.h>

namespace util {

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
            *bytes-- = int_to_char(n % 10);
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

    /*
    // Similar but simplier approach with no error checking
    // The number is assumed to be right-justified.
    template <int N, typename T>
    class unsafe_atoi {
        inline static void _atoi(const char*& c, T& n) {
            BOOST_ASSERT((*c >= '0' && *c <= '9') || *c == ' ');
            // N & 0xF treats '0' and ' ' the same
            n = n*10 + *(c++) & 0xF;
            unsafe_atoi<N-1, T>::_atoi(c, *(c++) & 0xF, n);
        }
    public:
        inline static const char* _atoi(const char* c, T& n) {
            BOOST_ASSERT((*c >= '0' && *c <= '9') || *c == ' ');
            n = *(c++) & 0xF;
            unsafe_atoi<N-1, T>::_atoi(c, n);
            return c;
        }
        inline static const char* _atoi_sgn(const char* c, T& n) {
            BOOST_ASSERT((*c >= '0' && *c <= '9') || *c == ' ' || *c == '-');
            switch (*c) {
                case ' ': { unsafe_atoi<N-1, T>::_atoi_sgn(++c, n); return c; }
                case '-': { const char* p = unsafe_atoi<N-1, T>::_atoi(++c, n); n = -n; return p; }
                default:  { unsafe_atoi<N, T>::_atoi(c, n); return c; }
            }
        }
    };

    template <typename T>
    class unsafe_atoi<1, T> {
        inline static void _atoi(const char*& c, T& n) {
            BOOST_ASSERT((*c >= '0' && *c <= '9') || *c == ' ');
            n *= 10;
            n += *(c++) & 0xF;
        }
    public:
        inline static const char* _atoi(const char* c, T& n) {
            BOOST_ASSERT((*c >= '0' && *c <= '9') || *c == ' ');
            n = *(c++) & 0xF;
            return c;
        }
        inline static const char* _atoi_sgn(const char* c, T& n) {
            _atoi(c, n);
            return c;
        }
    };

    template <bool T> struct sgn {};

    template <int N, typename T>
    const char* atoi(const char* c, T& n, sgn<false>) {
        return unsafe_atoi<N, T>::unsafe_atoi(c, n);
    }

    template <int N, typename T>
    const char* atoi(const char* c, T& n, sgn<true>) {
        return unsafe_atoi<N, T>::unsafe_atoi_sgn(c, n);
    }
    */

} // namespace detail

/*
/// Skips leading '0' and ' ' and converts an ASCII string to number.
/// The buffer size N must be statically known. Assign the result to
/// a signed integer for additional checking for leading '-'.
template <typename T, int N>
const char* unsafe_atoi(const char* c, T& n) {
    return detail::atoi<N, T>(c, detail::sgn<std::numeric_limits<T>::is_signed>());
}

template <typename T, int N>
const char* unsafe_atoi(const char*(&a_buf)[N], T& n) {
    return detail::atoi<N, T>(a_buf, detail::sgn<std::numeric_limits<T>::is_signed>());
}
*/


/**
 * A faster replacement to itoa() library function.
 * Additionally it allows skipping leading characters before making a conversion.
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
    char* p = itoa_left<T,Size>(buf, value, pad);
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
 * Additionally it allows skipping leading characters before making a conversion.
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
    char* p = itoa_right<T,Size>(buf, value, pad);
    return std::string(p+1, buf+Size - p - 1);
}

/**
 * A faster replacement to atoi() library function.
 * Additionally it allows skipping leading characters before making a conversion.
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
/// right padded with \a a_pad character.
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

/// @param till_eol instructs that the integer must be validated till a_str+a_sz.
///                 If false "123ABC" is considered a valid 123 number. Otherwise
///                 the function will return false.
inline bool fast_atoi(const char* a_str, const char* a_end, long& result, bool till_eol = true) {
    if (a_str >= a_end) return false;

    bool l_neg;

    if (*a_str == '-') { l_neg = true; ++a_str; }
    else               { l_neg = false; }

    long x = 0;

    do {
        const int c = *a_str - '0';
        if (c < 0 || c > 9) {
           if (till_eol)
               return false;
            else
                break;
        }
        x = (x << 3) + (x << 1) + c;
    } while (++a_str != a_end);

    result = l_neg ? -x : x;
    return true;
}

inline bool fast_atoi_skip_ws(const char* a_str, size_t a_sz, long& result,
    bool a_till_eol = true)
{
    const char* l_end = a_str + a_sz;
    // Find first non-white space char by treating ' ' like '\0'
    while (!(a_str == l_end || (*a_str & 0xF))) ++a_str;
    return fast_atoi(a_str, l_end, result, a_till_eol);
}

inline bool fast_atoi(const char* a_str, size_t a_sz, long& result,
    bool a_till_eol = true)
{
    const char* l_end = a_str + a_sz;
    return fast_atoi(a_str, l_end, result, a_till_eol);
}

inline bool fast_atoi_skip_ws(const std::string& a_str, long& result,
    bool a_till_eol = true)
{
    return fast_atoi_skip_ws(a_str.c_str(), a_str.size(), result, a_till_eol);
}

/// \copydetail fast_atoi()
inline bool fast_atoi(const std::string& a_value, long& a_result,
    bool a_till_eol = true)
{
    return fast_atoi(a_value.c_str(), a_value.size(), a_result, a_till_eol);
}

//--------------------------------------------------------------------------------
/// Convert a floating point number to string
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
double atof(const char* p, const char* end);

/**
 * Convert an integer to string.
 */
template <typename T>
std::string int_to_string(T n) {
    char buf[20];
    itoa_left(buf, n);
    return buf;
}

} // namespace util

#endif // _UTIL_ATOI_HPP_
