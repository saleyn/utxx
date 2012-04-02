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

This file may be included in several open-source projects.

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
#ifndef _UTIL_NCHAR_HPP_
#define _UTIL_NCHAR_HPP_

#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <algorithm>
#include <string>
#include <ostream>
#include <string.h>
#include <util/endian.hpp>
#include <util/convert.hpp>

namespace util {

namespace detail {

    /**
     * \brief A character buffer of size N storing data in 
     * big endian format.
     * The buffer allows for easy conversion between big endian and 
     * native data representation.
     * The class doesn't provide constructors and assignment operator
     * overloads so that it can be included in unions.
     */
    template <int N>
    class basic_nchar {
    protected:
        char m_data[N];
        BOOST_STATIC_ASSERT(N > 0);
    public:
        void set(const char (&a)[N+1])      { memcpy(m_data, a, N); }
        template <int M>
        void set(const char (&a)[M])        { copy_from(a, M); }
        void set(const std::string& a)      { copy_from(a.c_str(), a.size()); }
        void set(const char* a, size_t n)   { copy_from(a, n); }
        void set(const basic_nchar<N>& a)   { set(a.m_data, N); }

        template <int M>
        size_t copy_from(const basic_nchar<M>& a) {
            BOOST_STATIC_ASSERT(N <= M);
            return copy_from(a.data(), M);
        }

        size_t copy_from(const std::string& a) {
            return copy_from(a.c_str(), a.size());
        }

        size_t copy_from(const char* a, size_t n) {
            size_t m = std::min((size_t)N, n);
            memcpy(m_data, a, m);
            if (m < N) m_data[m] = '\0';
            return m;
        }

        size_t copy_from(const char* a, size_t n, char pad) {
            size_t m = copy_from(a, n);
            fill(pad, m);
            return N;
        }

        void fill(char a_ch, int a_offset = 0) {
            memset(&m_data[a_offset], a_ch, std::max(0, N - a_offset));
        }

        char& operator[] (size_t n) { return m_data[n]; }

        const char*   data() const  { return m_data; }
        char*         data()        { return m_data; }
        size_t        size() const  { return N; }

        operator uint8_t* () const  { return reinterpret_cast<uint8_t*>(m_data); }
        operator uint8_t* ()        { return reinterpret_cast<uint8_t*>(m_data); }

        operator char* () const     { return m_data; }
        operator char* ()           { return m_data; }

        std::string to_string(int until_char = '\0') const {
            const char* p = m_data, *end = m_data+N;
            for(; p < end && (*p != (char)until_char || until_char < 0); ++p);
            return std::string(m_data, p - m_data);
        }

        std::ostream& to_bin_string(std::ostream& out, int until_char = -1) const {
            out << "<<" << (int)*(uint8_t*)m_data;
            for(const char* p = m_data+1, *end = m_data + N;
                p < end && (until_char < 0 || *p != (char)until_char); ++p)
                out << ',' << (int)*(unsigned char*)p;
            return out << ">>";
        }

        /// Convert the ASCII buffer to an integer
        /// @param skip optionally skip leading characters matching \a skip.
        template <typename T>
        T to_integer(char skip = '\0') const {
            T n;
            atoi_left(m_data, n, skip);
            return n;
        }

        /// Convert the integer to a string representation aligned to left 
        /// with optional padding on the right.
        /// @param pad optional padding character.
        template <typename T>
        int from_integer(T n, char pad = '\0', bool a_align_left = true) {
            char* p = a_align_left ? itoa_left(m_data, n, pad)
                                   : itoa_right(m_data, n, pad);
            return p - m_data;
        }

        /// Store the variable of type T as binary integer using big 
        /// endian encoding.
        template <typename T>
        void from_binary(T a) {
            BOOST_STATIC_ASSERT(sizeof(T) == N && 
                (sizeof(T) <= 8) && (sizeof(T) & 1) == 0);
            util::store_be(m_data, a);
        }

        /// Return the result by treating the content as big 
        /// endian binary integer or double encoding.
        template <typename T>
        T to_binary() const {
            BOOST_STATIC_ASSERT(sizeof(T) == N && 
                (sizeof(T) <= 8) && (sizeof(T) & 1) == 0);
            T n; util::cast_be(m_data, n);
            return n;
        }

        std::ostream& dump(std::ostream& out) {
            bool printable = true;
            const char* end = m_data + N;
            for (const char* p = m_data; p != end; ++p) {
                if (*p < ' ' || *p > '~') {
                    printable = false; break;
                } else if (p > m_data && *p == '\0') {
                    end = ++p; break;
                }
            }
            for (const char* p = m_data; p != end; ++p) {
                if (printable)
                    out << *p;
                else
                    out << (p == m_data ? "" : ",") << (int)*(unsigned char*)p;
            }
            return out;
        }
    };
} // namespace detail

/**
 * \brief A character buffer of size N storing data in 
 * big endian format.
 * The buffer allows for easy conversion between big endian and 
 * native data representation.
 */
template <int N>
class nchar : public detail::basic_nchar<N> {
    typedef detail::basic_nchar<N> super;
public:
    nchar() {}

    template <typename T>
    nchar(T a)                      { *this = a; }

    nchar(const char (&a)[N+1])     { super::set(a); }
    template <int M>
    nchar(const char (&a)[M])       { super::set(a); }
    nchar(const std::string& a)     { super::set(a); }
    nchar(const uint8_t (&a)[N])    { super::set(a, N); }
    nchar(const char* a, size_t n)  { super::set(a, n); }

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

} // namespace util

#endif // _UTIL_NCHAR_HPP_

