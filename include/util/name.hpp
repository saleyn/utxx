//----------------------------------------------------------------------------
/// \file  name.hpp
//----------------------------------------------------------------------------
/// \brief Short name (up to 10 characters) encoded in an 8 bytes integer.
/// The characters in the name are limited to:
/// 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()-_+={}[]|:;<>'?
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-30
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the util open-source project.

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
#ifndef _UTIL_BASIC_SHORT_NAME_HPP_
#define _UTIL_BASIC_SHORT_NAME_HPP_

#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include <util/error.hpp>

namespace util {
    namespace {
        static const char s_fwd_name_lookup_table[] = {
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          /*             #   $   %   &   '   (   )   *   +       -   .   / */
             0,  0,  0, 56, 57, 58, 59, 60, 61, 62, 63,  1,  0,  2,  3,  4,
          /* 0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ? */
            10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 50, 51, 52, 53, 54, 55,
          /* @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O */
             5, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
          /* P   Q   R   S   T   U   V   W   X   Y   Z   [       ]   ^   _ */
            35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,  0, 47, 48, 49,
          /*                                                               */
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          /*                                             {   |   }   ~     */
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  7,  8,  9,  0,

             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
        };

        static const char s_rev_name_lookup_table[] = {
             0, '+','-','.','/','@','{','|','}','~','0','1','2','3','4','5',
            '6','7','8','9','A','B','C','D','E','F','G','H','I','J','K','L',
            'M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','[',']',
            '^','_',':',';','<','=','>','?','#','$','%','&','\'','(',')','*',
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
        };

    }

    template <size_t Size>
    class basic_short_name {
        typedef basic_short_name<Size> self_type;
    protected:
        static const size_t s_size       = Size;
        static const size_t s_bits_per_c = 6;
        static const size_t s_len_bits   = 4;
        static const size_t s_len_shift  = sizeof(uint64_t)*8 - s_len_bits;
        static const size_t s_len_mask   = ~0llu << s_len_shift;
        static const size_t s_val_mask   = ~s_len_mask;
        static const size_t s_char_mask  = (1 << s_bits_per_c) - 1;
        BOOST_STATIC_ASSERT(s_size < 11);
        BOOST_STATIC_ASSERT(sizeof(s_fwd_name_lookup_table) == 256);
        BOOST_STATIC_ASSERT(sizeof(s_rev_name_lookup_table) == 256);
        BOOST_STATIC_ASSERT(s_len_mask == 0xF000000000000000);
        BOOST_STATIC_ASSERT(s_val_mask == 0x0FFFFFFFFFFFFFFF);

        uint64_t m_value;

        inline int unsafe_write(char* a_buf) const {
            size_t n = length();
            char* p  = a_buf;
            for(size_t i = s_len_shift-s_bits_per_c, j = n; j; --j, i -= s_bits_per_c) {
                char ch = (m_value >> i) & s_char_mask;
                *p++ = s_rev_name_lookup_table[static_cast<int>(ch)];
            }
            return n;
        }

        inline char* right_pad(char ch, char* begin, char* end) const {
            if (begin == end) return begin;
            if (!ch) { end = begin + 1; return begin; }
            while (begin < end)
                *begin++ = ch;
            return begin;
        }

        void set_and_check(const char* a_buf, size_t a_size, bool a_no_case)
            throw (badarg_error)
        {
            int rc = set(a_buf, a_size, a_no_case);
            if (rc)
                throw badarg_error("Invalid character at position ",
                    -rc, " in '",
                    std::string(a_buf, a_size && !*(a_buf+a_size-1) ? a_size-1 : a_size),
                    "'");
        }

        bool valid_char(char ch) { return s_fwd_name_lookup_table[static_cast<int>(ch)]; }
    public:
        /// Number of characters needed to store the value.
        enum { SIZE = Size };

        size_t size() const { return SIZE; }

        template <int N>
        void set(const char (&a_buf)[N], bool a_no_case = false) throw (badarg_error) {
            BOOST_STATIC_ASSERT(N <= Size);
            set_and_check(a_buf, N, a_no_case);
        }

        void set(const std::string& a_val, bool a_no_case = false) throw (badarg_error) {
            set_and_check(a_val.c_str(), a_val.size(), a_no_case);
        }

        template <int N>
        void set(const char (&a_buf)[N], int& a_err, bool a_no_case = false) {
            BOOST_STATIC_ASSERT(N <= Size);
            a_err = set(a_buf, N, a_no_case);
        }

        /// Convert alphanumeric value to integer internal representation.
        /// It will truncate the name to Size characters.
        /// @param a_buf is the source string buffer
        /// @param a_size is the string length
        /// @param a_no_case if true perform upper case conversion.
        /// @return 0 on success.
        ///        -N on failure indicating position of the invalid character.
        int set(const char* a_buf, size_t a_size, bool a_no_case = false) {
            size_t sz = std::min(Size, a_size);
            const char* p = a_buf, *e = a_buf+sz;
            int rc = 0;
            m_value = 0;
            if (a_no_case)
                for(int i = s_len_shift - s_bits_per_c; *p && p != e; i -= s_bits_per_c, ++p) {
                    char c = ::toupper(*p);
                    if (!valid_char(c)) { rc = p == a_buf ? -1 : -(p - a_buf); break; }
                    m_value |= static_cast<uint64_t>(
                        s_fwd_name_lookup_table[static_cast<int>(c)]) << i;
                }
            else
                for(int i = s_len_shift - s_bits_per_c; *p && p != e; i -= s_bits_per_c) {
                    if (!valid_char(*p)) { rc = p == a_buf ? -1 : -(p - a_buf); break; }
                    m_value |= static_cast<uint64_t>(
                        s_fwd_name_lookup_table[static_cast<int>(*p++)]) << i;
                }
            sz = p - a_buf;
            m_value |= sz << s_len_shift;
            return rc;
        }

        /// Write decoded name to a given
        /// character buffer left-justified using \a pad as the
        /// padding character on the right.
        template <int N>
        int write(char (&a_buf)[N], char pad) const {
            int n = write(a_buf, N);
            return right_pad(pad, a_buf+n, a_buf+N) - a_buf;
        }

        int write(char* a_buf, size_t a_size) const {
            BOOST_ASSERT(a_size == SIZE || a_size > length());
            size_t n = unsafe_write(a_buf);
            if (n < a_size) a_buf[n] = '\0';
            return n;
        }

        /// Write decoded name to a given character buffer.
        template <int N>
        int write(char (&a_buf)[N]) const {
            BOOST_ASSERT(length() < N);
            return write(a_buf, N);
        }

        std::string to_string(char pad = '\0') const {
            char buf[SIZE];
            size_t len = write(buf, pad);
            return std::string(buf, len);
        }

        size_t   length()   const { return m_value >> s_len_shift; }
        operator uint64_t() const { return m_value; }
        uint64_t to_int()   const { return m_value; }

        /*
        bool operator==(const basic_name<Size>& a_rhs) const {
            return m_value == a_rhs.m_value;
        }
        */
        bool operator< (const self_type& a_rhs) const {
            return (m_value & s_val_mask) < (a_rhs.m_value & s_val_mask);
        }
        bool operator> (const self_type& a_rhs) const {
            return (m_value & s_val_mask) > (a_rhs.m_value & s_val_mask);
        }

        std::ostream& operator<<(std::ostream& out) const { out << to_string(); }
    };

    class name_t : public basic_short_name<10u> {
        typedef basic_short_name<10u> base;
    public:
        name_t() { m_value = 0u; }
        explicit name_t(uint64_t a_symbol) {
            BOOST_ASSERT(((a_symbol & s_len_mask) >> s_len_shift) <= s_size);
            m_value = a_symbol;
        }
        explicit name_t(const std::string& a, bool a_no_case = false) {
            this->set(a, a_no_case);
        }

        name_t(const char* a_buf, size_t a_sz, bool a_no_case = false) {
            this->set_and_check(a_buf, a_sz, a_no_case);
        }

        template <int N>
        name_t (const char (&a_buf)[N]) { this->set_and_check(a_buf, N, false); }
    };

} // namespace util

#endif // _UTIL_BASIC_SHORT_NAME_HPP_
