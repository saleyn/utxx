//----------------------------------------------------------------------------
/// \file   string.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Generic string processing functions.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2009-11-16
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

#ifndef _UTXX_STRING_HPP_
#define _UTXX_STRING_HPP_

#include <string>
#include <iostream>
#include <cctype>
#include <string.h>
#include <algorithm>
#include <stdexcept>
#include <sstream>

//-----------------------------------------------------------------------------
// STRING
//-----------------------------------------------------------------------------

namespace utxx {

    template <typename T, int N>
    size_t length(const T (&a)[N]) {
        return N;
    }

    /// Find <s> in the static array of string <choices>.
    /// @return position of <s> in the <choices> array of <def> if string not found.
    /// 
    template <typename T>
    inline T find_index(const char* choices[], size_t n, 
                        const std::string& s, T def = static_cast<T>(-1)) {
        for (size_t i=0; i < n; ++i)
            if (s == choices[i])
                return static_cast<T>(i);
        return def;
    }

    template <typename T, int N>
    inline T find_index(const char* (&choices)[N], const std::string& s,
        T def = static_cast<T>(-1))
    {
        return find_index(choices, N, s, def);
    }

    template <typename T, int N>
    inline T find_index_or_throw(const char* (&choices)[N], const std::string& s)
    {
        T result = find_index<T>(choices, N, s, (T)-1);
        if (result == (T)-1)
            throw std::runtime_error(std::string("String not found: ")+s);
        return result;
    }

    /// Perform wildcard matching of a string. The pattern argument can
    /// contain any letters including '*', '?', and "[]". In case of the brackets
    /// the character set must not exceed 255 chars.
    /// @return 1 if \a a_src matches the \a a_pattern.
    ///         0 if \a a_src doesn't match the \a a_pattern.
    ///        -1 if there's some other error.
    int wildcard_match(const char* a_src, const char* a_pattern);

    /// Case insensitive traits for implementing string_nocase
    ///
    struct traits_nocase : std::char_traits<char> {
        static bool eq(const char& c1, const char& c2) { return toupper(c1) == toupper(c2); }
        static bool lt(const char& c1, const char& c2) { return toupper(c1) <  toupper(c2); }
        static int  compare(const char* s1, const char* s2, size_t n) { return strncasecmp(s1, s2, n); }
        static const char* find(const char* s, size_t n, const char& ch) {
            for (size_t i=0; i < n; ++i)
                if (toupper(s[i] == toupper(ch))) return s+i;
            return 0;
        }
        static bool eq_int_type(const int_type& c1, const int_type& c2) { return eq(c1, c2); }
    };

    /// Case insensitive string
    ///
    typedef std::basic_string<char, traits_nocase> string_nocase;

    inline std::ostream& operator<< ( std::ostream& stm, const string_nocase& str ) { 
        return stm << reinterpret_cast<const std::string&>(str) ; 
    }

    inline std::istream& operator>> (std::istream& stm, string_nocase& str) {
        std::string s; stm >> s;
        if (stm) str.assign(s.begin(),s.end());
        return stm;
    }

    inline std::istream& getline(std::istream& stm, string_nocase& str) {
        std::string s; std::getline(stm,s);
        if (stm) str.assign(s.begin(),s.end());
        return stm;
    }

    /// Convert string to lower case
    inline std::string& to_lower(std::string& s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    /// Convert string to lower case
    inline std::string& to_upper(std::string& s) {
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return s;
    }

    inline std::string to_bin_string(const char* buf, size_t sz, bool hex = false) {
        std::stringstream out; out << "<<";
        const char* begin = buf, *end = buf + sz;
        for(const char* p = begin; p != end; ++p) {
            out << (p == begin ? "" : ",");
            if (hex) out << std::hex;
            out << (int)*(unsigned char*)p;
        }
        out << ">>";
        return out.str();
    }

    template <class T1>
    inline std::string to_string(T1 a1) {
        std::stringstream s; s << a1; return s.str();
    }

    template <class T1, class T2>
    inline std::string to_string(T1 a1, T2 a2) {
        std::stringstream s; s << a1 << a2; return s.str();
    }

    template <class T1, class T2, class T3>
    inline std::string to_string(T1 a1, T2 a2, T3 a3) {
        std::stringstream s; s << a1 << a2 << a3; return s.str();
    }

    template <class T1, class T2, class T3, class T4>
    inline std::string to_string(T1 a1, T2 a2, T3 a3, T4 a4) {
        std::stringstream s; s << a1 << a2 << a3 << a4; return s.str();
    }

    template <class T1, class T2, class T3, class T4, class T5>
    inline std::string to_string(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) {
        std::stringstream s; s << a1 << a2 << a3 << a4 << a5;
        return s.str();
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6>
    inline std::string to_string(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6) {
        std::stringstream s; s << a1 << a2 << a3 << a4 << a5 << a6;
        return s.str();
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    inline std::string to_string(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7) {
        std::stringstream s; s << a1 << a2 << a3 << a4 << a5 << a6 << a7;
        return s.str();
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
    inline std::string to_string(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8) {
        std::stringstream s; s << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8;
        return s.str();
    }

    template <class T1, class T2, class T3, class T4,
              class T5, class T6, class T7, class T8, class T9>
    inline std::string to_string(T1 a1, T2 a2, T3 a3, T4 a4,
                                 T5 a5, T6 a6, T7 a7, T8 a8, T9 a9) {
        std::stringstream s; s << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8 << a9;
        return s.str();
    }

    template <class T1, class T2, class T3, class T4,
              class T5, class T6, class T7, class T8, class T9, class T10>
    inline std::string to_string(T1 a1, T2 a2, T3 a3, T4 a4,
                                 T5 a5, T6 a6, T7 a7, T8 a8, T9 a9, T10 a10) {
        std::stringstream s; s << a1 << a2 << a3 << a4
                               << a5 << a6 << a7 << a8 << a9 << a10;
        return s.str();
    }

} // namespace utxx

#endif // _UTXX_STRING_HPP_
