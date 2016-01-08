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
#pragma once

#include <string>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <string.h>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <array>
#include <type_traits>
#include <utxx/types.hpp>
#include <utxx/print.hpp>

//-----------------------------------------------------------------------------
// STRING
//-----------------------------------------------------------------------------

namespace utxx {

    /// Return the static length of an array of type T
    template <typename T, int N>
    constexpr size_t length(const T (&a)[N])    { return N; }

    /// Return the static length of a character string
    template <int N>
    constexpr size_t length(const char (&a)[N]) { return N-1; }

    /// Return the static length of any given type.
    /// Example:
    /// \code
    /// size_t b[3]; cout << length<decltype(b)>() << endl;
    /// \endcode
    template <typename T>
    constexpr size_t length() { return std::extent<T>::value; }

    /// Convert boolean to string
    inline constexpr const char* to_string(bool a_value) {
        return a_value ? "true" : "false";
    }

    /// Copy up to \a n bytes of \a a_src to \a a_dest.
    /// The buffer is copied until \a a_delim character is found or
    /// min of \a a_dsize and \a a_slen is reached. The resulting string
    /// is NULL-terminated
    /// @return pointer to the last byte copied
    inline char* copy(char* a_dest, size_t a_dsize, const char* a_src, size_t a_slen, char a_delim='\0') {
        const char* s  = a_src;
        const char* de = a_dest + a_dsize;
        const char* e  = s + std::min<size_t>(a_dsize, a_slen);
        for(; *s != a_delim && s != e; *a_dest++ = *s++);
        if (a_dest == de)
            a_dest--;
        *a_dest = '\0';
        return a_dest;
    }

    template <int N>
    inline char* copy(char (&a_dest)[N], const char* a_src, size_t a_slen, char a_delim='\0') {
        return copy(a_dest, N, a_src, a_slen, a_delim);
    }

    template <int N>
    inline char* copy(char (&a_dest)[N], const std::string& a_src, char a_delim='\0') {
        return copy(a_dest, N, a_src.c_str(), a_src.size(), a_delim);
    }

    inline char* copy(char* a_dest, size_t a_dsize, const std::string& a_src, char a_delim='\0') {
        return copy(a_dest, a_dsize, a_src.c_str(), a_src.size(), a_delim);
    }

    template <int N>
    inline char* copy(std::array<char, N>& a_dest, const std::string& a_src, char a_delim='\0') {
        return copy(a_dest.data(), N, a_src.c_str(), a_src.size(), a_delim);
    }

    template <int N, int M>
    inline char* copy(std::array<char, N>& a_dest, const std::array<char, M>& a_src, char a_delim='\0') {
        return copy(a_dest.data(), N, a_src.data(), M, a_delim);
    }

    template <int N, int M>
    inline char* copy(char (&a_dest)[N], const std::array<char, M>& a_src, char a_delim='\0') {
        return copy(a_dest, N, a_src.data(), M, a_delim);
    }

    /// Split \a a_str at the specified delimiter.
    /// Example: \code split<LEFT>("abc, efg", ", ") -> {"abc", "efg"} \endcode
    template <alignment Side = LEFT>
    inline std::pair<std::string, std::string> split(std::string const& a_str, std::string const& a_delim = "|") {
        auto   found =  Side == LEFT ? a_str.find(a_delim) : a_str.rfind(a_delim);
        return found == std::string::npos
            ? (Side == LEFT ? std::make_pair(a_str, std::string()) : std::make_pair(std::string(), a_str))
            : std::make_pair(a_str.substr(0, found), a_str.substr(found+a_delim.size()));
    }

    namespace { auto def_joiner = [](auto& a) { return a; }; }

    /// Join two strings with a delimiter \a a_delim
    inline std::string strjoin(const std::string& a, const std::string& b, const std::string& a_delim) {
        if (a.empty()) return b.empty() ? "" : b;
        if (b.empty()) return a;
        std::string s;
        s.reserve(a.size() + b.size() + a_delim.size() + 1);
        return s.append(a).append(a_delim).append(b);
    }

    inline std::string strjoin(const char* a, const char* b, const std::string& a_delim) {
        if (a[0] == '\0') return b[0] == '\0' ? "" : b;
        if (b[0] == '\0') return a;
        std::string s;
        s.reserve(strlen(a) + strlen(b) + a_delim.size() + 1);
        return s.append(a).append(a_delim).append(b);
    }

    /// Join an iterator range to string delimited by \a a_delim
    /// \code Example: std::string s=join(vec.begin(), vec.end());\endcode
    template <class A, class ToStrFun = decltype(def_joiner)>
    std::string join(const A& a_begin, const A& a_end, const std::string& a_delim = ",", const ToStrFun& a_convert = def_joiner)
    {
        std::string result;
        auto it  = a_begin;
        if  (it != a_end) result.append(a_convert(*it++));
        for (; it != a_end; ++it) {
            result.append(a_delim);
            result.append(a_convert(*it));
        }
        return result;
    }

    /// Join strings from a given collection to string delimited by \a a_delim
    /// \code Example: std::string s=join(vec);\endcode
    template <class Collection, class ToStrFun = decltype(def_joiner)>
    std::string join(const Collection& a_vec, const std::string& a_delim, const ToStrFun& a_convert = def_joiner)
    {
        return join(a_vec.begin(), a_vec.end(), a_delim, a_convert);
    }

    /// Convert a string to an integer value
    /// \code
    /// std::cout << to_int64("\1\2") << std::endl;
    ///     output: 258     // I.e. (1 << 8 | 2)
    /// \endcode
    constexpr uint64_t to_int64(const char* a_str, size_t sz) {
        return sz ? (to_int64(a_str, sz-1) << 8 | (uint8_t)a_str[sz-1]) : 0;
    }

    template <int N>
    #if __cplusplus >= 201103L
    constexpr
    #endif
    uint64_t to_int64(const char (&a)[N]) {
        return to_int64(a, N-1);
    }

    inline char* from_int64(size_t a_int, char* out, const char* a_end, char a_eol = '\0') {
        char* p = out;
        while (a_int && p != a_end) {
            *p++ = a_int & 0xFF;
            a_int >>= 8;
        }
        for (int i = 0, j = (p - out - 1); i < j; i++, j--)
            std::swap(out[i], out[j]);
        *p = a_eol;
        return p;
    }

    template <int N>
    inline char* from_int64(size_t a_int, char (&out)[N], char a_eol = '\0') {
        return from_int64(a_int, out, out+N, a_eol);
    }

    inline std::string from_int64(size_t a_int) {
        char buf[32];
        auto end = from_int64(a_int, buf);
        return std::string(buf, end-buf);
    }

    /// Find the position of character \a c.
    /// @return pointer to the position of character \a c, or \a end if
    ///         no character \a c is found.
    inline const char* find_pos(const char* str, const char* end, char c) {
        for (const char* p = str; p != end; ++p)
            if (*p == c)
                return p;
        return end;
    }

    /// Compare strings optionally using non-case-sensitive comparator
    inline int compare(const char* a, const char* b, size_t sz, bool a_nocase) {
        return (a_nocase ? strncasecmp(a, b, sz) : strncmp(a, b, sz));
    }

    /// Find \a s in the static array of string \a choices.
    /// @return position of \a s in the \a choices array or \a def if
    ///         the string not found.
    /// 
    template <typename T>
    inline T find_index(const char* choices[], size_t n, 
                        const std::string& s,  T def = static_cast<T>(-1),
                        bool a_nocase = false) {
        for (size_t i=0; i < n; ++i)
            if (!compare(s.c_str(), choices[i], s.size(), a_nocase))
                return static_cast<T>(i);
        return def;
    }

    /// Find \a value in the static array of string \a choices.
    /// @param len is the length of \a value. If 0, return \a def.
    /// @return position of \a value in the \a choices array or \a def if
    ///         the string not found.
    /// 
    template <typename T>
    inline T find_index(const char* choices[], size_t n, 
                        const char* value,     size_t len,
                        T def = static_cast<T>(-1),
                        bool a_nocase = false) {
        if (!len) return def;
        for (size_t i=0; i < n; ++i)
            if (!compare(value, choices[i], len, a_nocase))
                return static_cast<T>(i);
        return def;
    }

    /// Find \a value in the static array of string \a choices.
    /// @return position of \a value in the \a choices array or \a def if
    ///         the string not found.
    /// 
    template <typename T, int N>
    inline T find_index(const char* (&choices)[N], const std::string& value,
        T def = static_cast<T>(-1), bool a_nocase = false)
    {
        return find_index(choices, N, value, def, a_nocase);
    }

    /// Find \a value in the static array of string \a choices.
    /// @param len is the length of \a value. If 0, return \a def.
    /// @return position of \a value in the \a choices array or \a def if
    ///         the string not found.
    /// 
    template <typename T, int N>
    inline T find_index(const char* (&choices)[N], const char* value, size_t len,
        T def = static_cast<T>(-1), bool a_nocase = false)
    {
        return find_index(choices, N, value, len, def, a_nocase);
    }

    template <typename T, int N>
    inline T find_index_or_throw(const char* (&choices)[N], const std::string& s,
        T def = static_cast<T>(-1), bool a_nocase = false)
    {
        T result = find_index<T>(choices, N, s, def, a_nocase);
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
    bool wildcard_match(const char* a_src, const char* a_pattern);

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

    /// Convert a buffer to an Erlang-like binary string of bytes in the
    /// form: <<123,234,12,18>>
    /// @param buf      is the string buffer to convert
    /// @param sz       is the length of buffer
    /// @param hex      output hex numbers
    /// @param readable if true prints buffer as a readable string if it
    ///                 contains all printable bytes. If false - binary
    ///                 list is printed
    /// @param eol      if true - terminate string with end-of-line
    std::string to_bin_string(const char* buf, size_t sz,
                              bool hex = false, bool readable = true,
                              bool eol = false);

    template <class... Args>
    inline std::string to_string(Args&&... args) {
        buffered_print buf;
        buf.print(std::forward<Args>(args)...);
        return buf.to_string();
    }

} // namespace utxx
