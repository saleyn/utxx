// vim:ts=4:sw=4:et
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
#include <vector>
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wempty-body"

    /// Get the length of a string stored inside a fixed-length string buffer
    /// String may not be NULL-terminated
    inline int strnlen(const char* s, const char* end) {
        auto p=s; for(;*p && p!=end; ++p);
        return p-s;
    }

    /// Get the length of a string stored inside a fixed-length string buffer
    /// String may not be NULL-terminated
    template <int N>
    int strnlen(const char (&s)[N]) { return strnlen(s, s+N); }

#pragma GCC diagnostic pop    

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

    template <size_t N>
    inline char* copy(std::array<char, N>& a_dest, const std::string& a_src, char a_delim='\0') {
        return copy(a_dest.data(), N, a_src.c_str(), a_src.size(), a_delim);
    }

    template <size_t N, size_t M>
    inline char* copy(std::array<char, N>& a_dest, const std::array<char, M>& a_src, char a_delim='\0') {
        return copy(a_dest.data(), N, a_src.data(), M, a_delim);
    }

    template <int N, size_t M>
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

    /// Replace the first occurance of string "from" in "str" with "to"
    inline std::string replace(std::string str, const std::string& from, const std::string& to) {
        size_t start_pos = str.find(from);
        if(start_pos != std::string::npos)
            str.replace(start_pos, from.length(), to);
        return str;
    }

    /// Replace all occurances of string "from" in "str" with "to"
    inline std::string replace_all(std::string str, const std::string& from, const std::string& to) {
        if (!from.empty()) {
            size_t start_pos = 0;
            while((start_pos = str.find(from, start_pos)) != std::string::npos) {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length();
            }
        }
        return str;
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

    /// Trim from start (in place)
    inline void ltrim(std::string& s, std::string const& delim = std::string()) {
        const std::string& d = delim.empty() ? std::string(" \t\n\r") : delim;
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&d](char ch) {
            return d.find(ch) == std::string::npos;
        }));
    }

    /// Trim from end (in place)
    inline void rtrim(std::string& s, std::string const& delim = std::string()) {
        const std::string& d = delim.empty() ? std::string(" \t\n\r") : delim;
        s.erase(std::find_if(s.rbegin(), s.rend(), [&d](char ch) {
            return d.find(ch) == std::string::npos;
        }).base(), s.end());
    }

    inline void ltrim(std::string& s, char c) { auto ss=std::string(1,c); ltrim(s, ss); }
    inline void rtrim(std::string& s, char c) { auto ss=std::string(1,c); rtrim(s, ss); }

    /// Trim from both ends (in place)
    inline void trim(std::string &s, std::string const& delim=std::string()) {
        ltrim(s, delim); rtrim(s, delim);
    }

    /// Trim from start (copying)
    inline std::string ltrim_copy(std::string s, std::string const& delim=std::string()) {
        ltrim(s, delim);
        return s;
    }

    /// Trim from end (copying)
    inline std::string rtrim_copy(std::string s, std::string const& delim=std::string()) {
        rtrim(s, delim);
        return s;
    }

    /// Trim from both ends (copying)
    inline std::string trim_copy(std::string s, std::string const& delim=std::string()) {
        trim(s, delim);
        return s;
    }

    /// Write a hex representation of the character 'c' to "dst" string
    inline char* hex(char* dst, char c, bool lower=false) {
        static const char* s_array[2] = {"0123456789ABCDEF", "0123456789abcdef"};
        auto* s = s_array[lower];
        *dst++  = s[int((c >> 4) & 0xF)];
        *dst++  = s[int(c & 0xF)];
        return    dst;
    }

    /// Decode the next two chars in a hex representation of the "a_src" string
    inline char unhex(const char* a_src) {
        auto f = [](char c) { c = toupper(c); return char(c >= 'A' ? 10+(c-'A') : c-'0'); };
        return (f(*a_src) << 4) | f(*(a_src+1));
    }

    /// Encode a hex representation of the "a_str" string
    inline std::string hex(const char* a_str, size_t sz, bool a_lower=false) {
        std::string res;
        res.reserve(sz*2);
        for (const char* p = a_str, *e = p+sz; p != e; ++p) {
            char ss[2];
            hex(ss, *p, a_lower);
            res.append(ss, 2);
        }
        return res;
    }

    /// Encode a hex representation of the "a_val" value converted to string
    template <typename T>
    inline std::string hex(T a_val, bool a_lower=false) {
        auto src = std::to_string(a_val);
        return hex(src.c_str(), src.size(), a_lower);
    }

    /// Encode a hex representation of the "s" string
    inline std::string hex(const std::string& s, bool lower=false) { return hex(s.c_str(), s.size(), lower); }

    /// Unhex the hex representation of the string "a_str" to vector
    template <typename T>
    inline T unhex(const char* a_str, size_t sz) {
        assert((sz&1) == 0); // size must be even
        T res;
        res.reserve(sz / 2);
        for (const char* p = a_str, *e = p+sz; p != e; p += 2)
            res.push_back(unhex(p));
        return res;
    }

    /// Unhex the hex representation of the string "a_str" to vector
    template <typename T>
    inline T unhex(const std::string& s) {
        return unhex<T>(s.c_str(), s.size());
    }

    inline std::vector<char> unhex_vector(const std::string& s) {
        return unhex<std::vector<char>>(s.c_str(), s.size());
    }

    inline std::string unhex_string(const std::string& s) {
        return unhex<std::string>(s.c_str(), s.size());
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

    /// Perform wildcard matching of a string.
    /// The pattern argument can contain any letters including '*' and '?'.
    /// '*' means to match 0 or more characters;
    /// '?' means to match exactly one character.
    /// @return true if \a a_src matches the \a a_pattern.
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
                              bool eol = false, const char* delim = ",",
                              const char* pfx = "<<", const char* sfx = ">>");

    template <class... Args>
    inline std::string to_string(Args&&... args) {
        buffered_print buf;
        buf.print(std::forward<Args>(args)...);
        return buf.to_string();
    }

    /// Convert input to a printable hex string
    /// @see: https://stackoverflow.com/questions/311165/how-do-you-convert-a-byte-array-to-a-hexadecimal-string-and-vice-versa/14333437#14333437
    template <typename Ch=char>
    inline std::string to_hex_string(const Ch* a_bytes, size_t a_sz)
    {
        std::string s(a_sz*2, 0);
        for (auto* p = &s[0], *e = p + s.size(); p < e;) {
            auto hi = *a_bytes >> 4;
            auto lo = *a_bytes++ & 0xF;
            *p++ = (char)(55 + hi + (((hi-10)>>31)&-7));
            *p++ = (char)(55 + lo + (((lo-10)>>31)&-7));
        }
        return s;
    }

    template <typename T = std::string>
    std::string to_hex_string(T s) { return to_hex_string(&s.front(), s.size()); }

    //--------------------------------------------------------------------------
    /// Representation of a string value that has an embedded buffer.
    /// Short strings (size <= N) don't involve memory allocations (def: N=47).
    /// Short string can be set to have NULL value by using "set_null()" method.
    /// NULL strings are represented by having size() = -1.
    //--------------------------------------------------------------------------
    template <typename Char  = char, int MaxSz = 64-1-2*sizeof(void*),
              class    Alloc = std::allocator<Char>>
    class basic_short_string : private Alloc {
        using Base           = Alloc;

        static basic_short_string init_null() {
            basic_short_string s; s.set_null(); return s;
        }
    public:
        using value_type     = Char;
        using iterator       = Char*;
        using const_iterator = const Char*;

        static constexpr size_t max_size()   { return MaxSz; }
        static constexpr size_t round_size(size_t a)   {
            return ((a+(1+2*sizeof(void*)))+7)&~7;
        }

        static const  basic_short_string& null_value() {
            static basic_short_string s_null = init_null();
            return s_null;
        }

        explicit
        basic_short_string(const Alloc& ac = Alloc())
            : Base(ac), m_val(m_buf),m_sz(0),m_max_sz(MaxSz) { m_buf[0] = '\0'; }
        explicit
        basic_short_string(const Char* a,           const Alloc& ac = Alloc())
            : Base(ac), m_val(m_buf),m_sz(0),m_max_sz(MaxSz)
            { set(a,strlen((const char*)a));}
        basic_short_string(const Char* a, size_t n, const Alloc& ac = Alloc())
            : Base(ac), m_val(m_buf),m_sz(0),m_max_sz(MaxSz) { set(a, n); }
        basic_short_string(basic_short_string&&      a, const Alloc& ac = Alloc())
            : Base(ac) { assign<false>(std::move(a)); }
        basic_short_string(const basic_short_string& a, const Alloc& ac = Alloc())
            : Base(ac), m_val(m_buf),m_sz(0),m_max_sz(MaxSz) { set(a); }
        explicit
        basic_short_string(std::basic_string<Char>&& a, const Alloc& ac = Alloc())
            : Base(ac), m_val(m_buf),m_sz(0),m_max_sz(MaxSz) { set(a); }
        explicit
        basic_short_string(const std::basic_string<Char>& a, const Alloc& ac = Alloc())
            : Base(ac), m_val(m_buf),m_sz(0),m_max_sz(MaxSz) { set(a); }

        ~basic_short_string() { deallocate(); }

        void set(const Char*                    a) { set(a, strlen((char*)a));        }
        void set(const std::basic_string<Char>& a) { set(a.c_str(), a.size()); }
        void set(const basic_short_string&      a) { set(a.c_str(), a.size()); }
        void set(const Char* a, int n) {
            assert(n >= -1);
            if (n > int(m_max_sz)) {
                deallocate();
                if (n > MaxSz) {
                    auto max = ((n + 1) + 7) & ~7;  // Round to 8 bytes
                    m_max_sz = max-1;
                    m_val    = Base::allocate(max);
                }
            }
            if (n <= 0)
                m_val[0] = '\0';
            else {
                assert(a);
                memcpy((char*)m_val, (char*)a, n);
                m_val[n] = '\0';
            }
            m_sz = n;
        }

        /// Set to empty string without deallocating memory
        void clear() { m_val[0] = '\0'; m_sz = 0; }

        /// Deallocate memory (if allocated) and set to empty string
        void reset() { deallocate(); clear();     }

        void append(const std::basic_string<Char>& a) { append(a.c_str(), a.size()); }
        void append(const Char* a)                    { append(a, strlen((char*)a)); }
        void append(const Char* a, size_t n) {
            assert(a);
            auto oldsz = is_null() ? 0 : m_sz;
            auto sz    = oldsz+n;
            if  (sz   <= capacity())
                memcpy(m_val+oldsz, a, n);
            else {
                auto max = ((sz + 1) + 7) & ~7;  // Round to 8 bytes
                auto p   = Base::allocate(max);
                memcpy(p,  m_val, oldsz);
                memcpy(p+oldsz, a, n);
                deallocate();
                m_max_sz = max-1;
                m_val    = p;
            }
            m_sz         = sz;
            m_val[m_sz]  = '\0';
        }

        /// Reserve space for holding \a a_capacity bytes without changing size.
        void reserve(size_t a_capacity) {
            if (a_capacity <= capacity())
                return;

            auto max = ((a_capacity + 1) + 7) & ~7;  // Round to 8 bytes
            auto p   = Base::allocate(max);
            auto n   = std::max(0, m_sz);
            memcpy(p,  m_val, n+1); // Include trailing '\0'
            deallocate();
            m_max_sz = max-1;
            m_val    = p;
        }

        void operator= (const Char*                     a) { set(a); }
        void operator= (const std::basic_string<Char>&  a) { set(a); }
        void operator= (basic_short_string&&            a) { assign<true>(std::move(a)); }
        void operator= (const basic_short_string&       a) { set(a); }

        bool operator==(const Char* a)        const { return !strcmp((char*)m_val, (char*)a); }
        bool operator!=(const Char* a)        const { return !operator==(a); }
        bool operator==(const std::basic_string<Char>& a) const { return !strcmp((char*)m_val, (char*)a.c_str()); }
        bool operator!=(const std::basic_string<Char>& a) const { return !operator==(a); }

        bool operator==(const basic_short_string& a) const {
            return m_sz == a.m_sz && !strcmp((char*)m_val, (char*)a.c_str());
        }

        bool operator!=(const basic_short_string& a) const {
            return !operator==(a);
        }

        template <typename StreamT>
        inline friend StreamT& operator<<(StreamT& out, basic_short_string const& a) {
            out << a.c_str();
            return out;
        }

        Char  operator[](int n)         const { assert(n >= 0 && n < m_sz); return m_val[n]; }
        Char& operator[](int n)               { assert(n >= 0 && n < m_sz); return m_val[n]; }

        operator       bool()           const { return !is_null();     }
        operator const Char*()          const { return m_val;          }

        const Char*    c_str()          const { return m_val;          }
        int            size()           const { return m_sz;           }

        /// Change size and add null-terminator (the size must not exceed capacity!)
        void           size(size_t n)         { assert(n < m_max_sz); m_val[n] = '\0'; m_sz = n; }

        /// Reserve space for \a n bytes and set size to \a n.
        void           resize(size_t n)       { reserve(n);  size(n);  }

        size_t         capacity()       const { return m_max_sz;       }
        Char*          str()                  { return m_val;          }
        bool           allocated()      const { return m_val != m_buf; }

        bool           is_null()        const { return m_sz < 0;       }
        void           set_null()             { m_sz=-1; m_val[0]='\0';}

        iterator       begin()                { return m_val;          }
        iterator       end()                  { return is_null() ? m_val : m_val+m_sz; }

        const_iterator begin()          const { return m_val;          }
        const_iterator end()            const { return is_null() ? m_val : m_val+m_sz; }

        const_iterator cbegin()         const { return m_val;          }
        const_iterator cend()           const { return is_null() ? m_val : m_val+m_sz; }
    private:
        Char*  m_val;
        int    m_sz;
        uint   m_max_sz;
        Char   m_buf[MaxSz+1];

        void deallocate() {
            if (allocated()) {
                Base::deallocate(m_val, m_max_sz+1);
                m_max_sz = MaxSz;
                m_val    = m_buf;
            }
        }

        template <bool Reset>
        void assign(basic_short_string&& a) {
            if (Reset) reset();
            if (a.allocated()) { m_val = a.m_val; a.m_val = a.m_buf;      }
            else               { m_val = m_buf;   strcpy((char*)m_buf, (const char*)a.m_buf); }
            m_sz       = a.m_sz;
            m_max_sz   = a.m_max_sz;
            a.m_buf[0] = '\0';
            a.m_sz     = 0;
        }

        static_assert(sizeof(typename Alloc::value_type) == sizeof(Char), "Invalid size");
    };

    using short_string = basic_short_string<>;
    using ustring      = std::basic_string<uint8_t>;

  //----------------------------------------------------------------------------
  /// Compact string class storing up to \a N-2 characters
  //----------------------------------------------------------------------------
  /// The (N-1)'s character is reserved to store string size
  /// The stored value is '\0' terminated
  //----------------------------------------------------------------------------
  template <size_t N>
  struct basic_fixed_string {
    static_assert(N > 2 && N <= 128, "Invalid string size");

    /// Max length that the string can hold
    static constexpr size_t MaxSize() { return N-2; }

    basic_fixed_string() { clear(); }
    explicit
    basic_fixed_string(std::nullptr_t            a) : basic_fixed_string() {}
    explicit
    basic_fixed_string(const std::string&        a) : basic_fixed_string(a.c_str(), a.size()) {}
    explicit
    basic_fixed_string(const char*               a) : basic_fixed_string(a, a ? strlen(a) : 0){}
    basic_fixed_string(const char* a,  size_t a_sz) { set(a, a_sz); }
    basic_fixed_string(const basic_fixed_string& a) : basic_fixed_string(a.c_str(), a.size()) {}
    basic_fixed_string(basic_fixed_string&&      a) : basic_fixed_string(a.c_str(), a.size()) {}

    void operator=(basic_fixed_string const& a) { set(a.c_str(), a.size()); }
    void operator=(basic_fixed_string&& a)      { set(a.c_str(), a.size()); }
    void operator=(std::string const&   a)      { set(a.c_str(), a.size()); }
    void operator=(char        const&   a)      { set(a);                   }

    const char* c_str()    const { return m_data.data();       }
    const char* data()     const { return m_data.data();       }
    size_t      size()     const { return size_t(m_data[N-1]); }
    bool        empty()    const { return !size();             }

    void clear() {m_data[0] = m_data[N-1] = '\0'; }

    void fill(char a, uint8_t new_sz = 0) {
        if (new_sz > MaxSize()) new_sz = MaxSize();
        memset(m_data.data(), a, new_sz);
        m_data[N-1] = new_sz;
    }

    bool equals(const char* s) const { return strncmp(c_str(), s, size())==0; }
    bool equals(const std::string& s) const { return equals(s.c_str()); }

    std::string to_string() const { return std::string(c_str(), size()); }

    bool operator== (basic_fixed_string   const& a_rhs) const {
        return size()==a_rhs.size() && !memcmp(c_str(),a_rhs.c_str(),size());
    }

    bool operator== (std::string const& a_rhs) const {
        return size()==a_rhs.size() && !memcmp(c_str(),a_rhs.c_str(),size());
    }

    bool operator== (char const* a_rhs) const {
        return size()==strlen(a_rhs) && !memcmp(c_str(),a_rhs, size());
    }

    template <int M>
    bool operator== (char  const (&a_rhs)[M]) const {
        return size()== M-1 && !memcmp(c_str(),a_rhs, size());
    }

    bool operator!= (basic_fixed_string const& a) const { return !operator==(a); }
    bool operator!= (std::string        const& a) const { return !operator==(a); }
    bool operator!= (char               const* a) const { return !operator==(a); }
    template <int M>
    bool operator!= (char          const (&a)[M]) const { return !operator==(a); }

    bool operator<  (basic_fixed_string   const& a) const {
        return strncmp(c_str(), a.c_str(), size()) < 0;
    }

    bool operator>  (basic_fixed_string   const& a_rhs) const {
        return strncmp(c_str(), a_rhs.c_str(), size()) > 0;
    }

    operator const char*() const          { return c_str(); }

    void set(std::string const& a)        { set(a.c_str(), a.size()); }

    void set(const char* a)               { set(a, strlen(a)); }

    void set(const char* a, size_t a_sz)  {
        size_t n    = std::min<size_t>(a_sz, MaxSize());
        if (a) memcpy(m_data.data(), a, n);
        m_data[n]   = '\0';
        m_data[N-1] = char(n);
        assert(n <= MaxSize());
    }

    template <typename Lambda, bool SetNull = true>
    void set(const Lambda& fun) {
        auto n = int(fun(m_data.data(), m_data.data()+MaxSize()));
        assert(n <= int(MaxSize()));
        if (SetNull)
            m_data[n] = '\0';
        m_data[N-1] = char(n);
    }

    template <typename StreamT>
    friend inline StreamT& operator<<(StreamT& out, basic_fixed_string<N> const& a) {
        out << a.c_str(); return out;
    }
  private:
    std::array<char, N> m_data;
  };

} // namespace utxx

namespace std {

  template<int N>
  struct hash<utxx::basic_fixed_string<N>> : public __hash_base<size_t, string> {
    size_t operator()(const utxx::basic_fixed_string<N>& k) const {
      return _Hash_impl::hash(k.c_str(), k.size());
    }
  };

  template<int N>
  struct equal_to<utxx::basic_fixed_string<N>> {
    bool operator()(const utxx::basic_fixed_string<N>& a, const utxx::basic_fixed_string<N>& b) const {
      return a == b;
    }
  };

} // namespace std
