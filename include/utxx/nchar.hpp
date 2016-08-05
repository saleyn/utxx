//----------------------------------------------------------------------------
/// \file  nchar.hpp
//----------------------------------------------------------------------------
/// \brief Character buffer that offers no padding when included into 
/// structures and automatic conversion to/from big endian representation.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-30
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
#ifndef _UTXX_NCHAR_HPP_
#define _UTXX_NCHAR_HPP_

#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <algorithm>
#include <string>
#include <ostream>
#include <string.h>
#include <utxx/endian.hpp>
#include <utxx/convert.hpp>
#include <utxx/string.hpp>
#include <utxx/print_opts.hpp>

namespace utxx {

namespace detail {

    /**
     * \brief A character buffer of size N storing data in 
     * big endian format.
     * The buffer allows for easy conversion between big endian and 
     * native data representation.
     * The class doesn't provide constructors and assignment operator
     * overloads so that it can be included in unions.
     */
    template <int N, class Char = char>
    class basic_nchar {
    protected:
        Char m_data[N];
        BOOST_STATIC_ASSERT(N > 0);
    public:
        void set(const Char (&a)[N+1])          { memcpy(m_data, a, N); }
        template <int M>
        void set(const Char (&a)[M])            { copy_from(a, M); }
        void set(const std::string& a)          { copy_from(a.c_str(),a.size());}
        void set(const Char* a, size_t n)       { copy_from(a, n); }
        void set(const basic_nchar<N,Char>& a)  { set(a.m_data, N); }

        template <int M>
        size_t copy_from(const basic_nchar<M, Char>& a) {
            BOOST_STATIC_ASSERT(N <= M);
            return copy_from(a.data(), M);
        }

        size_t copy_from(const std::basic_string<Char>& a) {
            return copy_from(a.c_str(), a.size());
        }

        size_t copy_from(const Char* a, size_t n) {
            size_t m = std::min((size_t)N, n);
            memcpy(m_data, a, m);
            if (m < N) m_data[m] = '\0';
            return m;
        }

        size_t copy_from(const Char* a, size_t n, Char pad) {
            size_t m = copy_from(a, n);
            fill(pad, m);
            return N;
        }

        /// Copy up to \a n bytes of internal buffer to \a dest.
        /// The buffer is copied until the \a delim character is found or \a n
        /// is reached.
        /// @return pointer to the last byte copied
        char* copy_to(Char* dest, size_t n, Char delim = '\0') const {
            return copy(dest, n, m_data, N, delim);        // from utxx/string.hpp
        }

        template <int M>
        char* copy_to(Char (&dest)[M], Char delim = '\0') const {
            return copy(dest, M, m_data, N, delim);        // from utxx/string.hpp
        }

        void fill(Char a_ch, int a_offset = 0) {
            const int n = N - a_offset;
            if (n > 0)
                memset(&m_data[a_offset], a_ch, n);
        }

        char& operator[] (size_t n) { return m_data[n]; }

        const Char*   data()    const  { return m_data; }
        Char*         data()           { return m_data; }
        constexpr size_t size() const  { return N; }
        const Char*   end()     const  { return m_data + N; }

        operator const  Char*() const  { return m_data; }
        operator        Char*()        { return m_data; }

        /// Return a string with trailing characters matching \a rtrim removed.
        std::string to_string(Char rtrim = '\0') const {
            const Char* end = m_data+N;
            if (rtrim)
                for(const Char* q = end-1, *e = m_data-1; q != e && (*q == rtrim || !*q); end = q--);
            for(const Char*p = m_data; p != end; ++p)
                if (!*p) {
                    end = p;
                    break;
                }
            return std::string(m_data, end - m_data);
        }

        /// Find the length of the string contained in the buffer up to the
        /// given delimiter.
        /// @return the string length up to the \a delimiter or size() if
        ///         the \a delimiter is not found
        size_t len(Char delimiter) const {
            const Char* pos = find_pos(m_data, end(), delimiter);
            return pos - m_data;
        }

        std::ostream& to_bin_string(std::ostream& out, int until_char = -1) const {
            out << "<<" << (int)*(uint8_t*)m_data;
            for(const Char* p = m_data+1, *end = m_data + N;
                p < end && (until_char < 0 || *p != (char)until_char); ++p)
                out << ',' << (int)*(unsigned char*)p;
            return out << ">>";
        }

        /// Convert the ASCII buffer to an integer (assuming it's left-aligned)
        /// @param skip skip leading characters matching \a skip.
        template <typename T>
        T to_integer(Char skip) const {
            T n;
            atoi_left(m_data, n, skip);
            return n;
        }

        /// Convert the ASCII buffer to an integer (assumes the value to be
        /// right-aligned, skips leading space and 0's)
        template <typename T>
        T to_integer() const {
            const Char* p = m_data;
            return unsafe_fixed_atol<N>(p);
        }

        /// Convert the integer to a string representation aligned to left
        /// with optional padding on the right.
        /// @param pad optional padding character.
        template <typename T>
        int from_integer(T n, char pad = '\0', bool a_align_left = true) {
            char* p = a_align_left ? itoa_left(m_data, n, pad)
                                   : itoa_right(m_data, N, n, pad);
            return p - m_data;
        }

        /// Convert the ASCII buffer to a double
        /// @param skip optionally skip leading characters matching \a skip.
        double to_double(Char skip = '\0') const {
            const Char *p = m_data, *end = p + N;
            if (skip)
                while (*p == skip && p != end) p++;
            double res = 0.0;
            (void)atof<double>(p, end, res);
            return res;
        }

        /// Convert the double to a string representation aligned to left 
        /// with optional padding on the right.
        /// @param n is the number to convert
        /// @param a_precision number of digits abter decimal point
        /// @param a_trail optional trailing character.
        /// @param a_compact strip extra '0' at the end
        /// @returns number of characters written, or -1 on error
        int from_double(double n, int a_precision, bool a_compact, Char a_trail='\0') {
            int k = ftoa_left(n, m_data, N, a_precision, a_compact);
            if (a_trail && k < N)
                for (auto* p = m_data+k, *end = m_data+N; p != end; ++p)
                    *p = a_trail;
            return k;
        }

        /// Convert the double to a string representation aligned to right
        /// with optional padding on the right.
        /// @param n is the number to convert
        /// @param a_precision number of digits abter decimal point
        /// @param a_lef_pad   left-padding character
        /// @returns number of characters written, or -1 on error
        int from_double(double n, int a_precision, Char a_left_pad) {
            try { ftoa_right(n, m_data, N, a_precision, a_left_pad); }
            catch (std::invalid_argument& e) { return -1; }
            return N;
        }

        /// Store the variable of type T as binary integer using big
        /// endian encoding.
        template <typename T, bool IsBigEndian = true>
        void from_binary(T a) {
            BOOST_STATIC_ASSERT(sizeof(T) == N && 
                (sizeof(T) <= 8) && (sizeof(T) & 1) == 0);
            if (IsBigEndian) utxx::store_be(m_data, a);
            else             utxx::store_le(m_data, a);

        }

        /// Return the result by treating the content as big 
        /// endian binary integer or double encoding.
        template <typename T, bool IsBigEndian = true>
        T to_binary() const {
            BOOST_STATIC_ASSERT(sizeof(T) == N && 
                (sizeof(T) <= 8) && (sizeof(T) & 1) == 0);
            T n;
            if (IsBigEndian) utxx::cast_be(m_data, n);
            else             utxx::cast_le(m_data, n);
            return n;
        }

        template <typename StreamT>
        StreamT& dump(StreamT& out, size_t a_sz = 0,
                      print_opts a_opts = print_opts::printable_or_dec,
                      const char* a_sep = ",", const char* a_hex_pfx = "0x") const
        {
            const Char* end = m_data + std::min<int>(a_sz ? a_sz : N, N);
            return output(out, m_data, end, a_opts, a_sep, a_hex_pfx);
        }
    };
} // namespace detail

/**
 * \brief A character buffer of size N storing data in 
 * big endian format.
 * The buffer allows for easy conversion between big endian and 
 * native data representation.
 */
template <int N, class Char = char>
class nchar : public detail::basic_nchar<N, Char> {
    typedef detail::basic_nchar<N, Char> super;
public:
    nchar() {}

    template <typename T>
    nchar(T a)                      { *this = a; }

    nchar(const Char (&a)[N+1])     { super::set(a); }
    template <int M>
    nchar(const Char (&a)[M])       { super::set(a); }
    nchar(const std::string& a)     { super::set(a); }
    nchar(const uint8_t (&a)[N])    { super::set(a, N); }
    nchar(const Char* a, size_t n)  { super::set(a, n); }

    template <typename T>
    void operator= (T a)             { super::set(a); }

    void operator= (int16_t  a)      { super::from_binary(a); }
    void operator= (int32_t  a)      { super::from_binary(a); }
    void operator= (int64_t  a)      { super::from_binary(a); }
    void operator= (uint16_t a)      { super::from_binary(a); }
    void operator= (uint32_t a)      { super::from_binary(a); }
    void operator= (uint64_t a)      { super::from_binary(a); }
    void operator= (double   a)      { super::from_binary(a); }
};



template <int N>
::std::ostream& operator<< (::std::ostream& out, const nchar<N>& a) {
    return a.dump(out);
}

} // namespace utxx

#endif // _UTXX_NCHAR_HPP_

