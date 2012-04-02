//----------------------------------------------------------------------------
/// \file  endian.hpp
//----------------------------------------------------------------------------
/// \brief A wrapper around boost endian-handling functions.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
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

#ifndef _UTIL_ENDIAN_HPP_
#define _UTIL_ENDIAN_HPP_

#include <boost/spirit/home/support/detail/endian.hpp>
#include <boost/static_assert.hpp>
#include <stdint.h>

namespace util {

namespace {
#if BOOST_VERSION >= 104900
    namespace bsd = boost::spirit::detail;
#else
    namespace bsd = boost::detail;
#endif
}

template <typename T, typename Char>
inline void put_be(Char*& s, T n) {
    bsd::store_big_endian<T, sizeof(T)>((void*)s, n);
    s += sizeof(T);
}

template <typename T, typename Char>
inline void get_be(const Char*& s, T& n) {
    n = bsd::load_big_endian<T, sizeof(T)>((const void*)s);
    s += sizeof(T);
}

inline void get_be(const char*& s, double& n) {
    BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
    union { double f; uint64_t i; } u;
    u.i = bsd::load_big_endian<uint64_t, sizeof(double)>(static_cast<const void*>(s));
    n = u.f;
    s += sizeof(double);
}

template <typename T, typename Char>
inline T get_be(const Char*& s) { T n; get_be(s, n); return n; }

template <typename T>
inline void store_be(char* s, T n) {
    bsd::store_big_endian<T, sizeof(T)>(static_cast<void*>(s), n);
}

inline void store_be(char* s, double n) {
    BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
    union u { double f; uint64_t i; };
    bsd::store_big_endian<uint64_t, sizeof(double)>(
        static_cast<void*>(s), reinterpret_cast<u*>(&n)->i);
}

template <typename T>
inline void cast_be(const char* s, T& n) {
    n = bsd::load_big_endian<T, sizeof(T)>((const void*)s);
}

inline void cast_be(const char* s, double& n) {
    BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
    union { double f; uint64_t i; } u;
    u.i = bsd::load_big_endian<uint64_t, sizeof(double)>((const void*)s);
    n = u.f;
}

template <typename T, typename Char>
inline T cast_be(const Char* s) { T n; cast_be(s, n); return n; }

template <class Char>
inline void put8   (Char*& s, uint8_t n ) { put_be(s, n); }
template <class Char>
inline void put16be(Char*& s, uint16_t n) { put_be(s, n); }
template <class Char>
inline void put32be(Char*& s, uint32_t n) { put_be(s, n); }
template <class Char>
inline void put64be(Char*& s, uint64_t n) { put_be(s, n); }

template <class Char>
inline uint8_t  get8   (const Char*& s) { uint8_t  n; get_be(s, n); return n; }
template <class Char>
inline uint16_t get16be(const Char*& s) { uint16_t n; get_be(s, n); return n; }
template <class Char>
inline uint32_t get32be(const Char*& s) { uint32_t n; get_be(s, n); return n; }
template <class Char>
inline uint64_t get64be(const Char*& s) { uint64_t n; get_be(s, n); return n; }

template <class Char>
inline uint8_t  cast8   (const Char* s) { return s[0]; }
template <class Char>
inline uint16_t cast16be(const Char* s) { uint16_t n; cast_be(s, n); return n; }
template <class Char>
inline uint32_t cast32be(const Char* s) { uint32_t n; cast_be(s, n); return n; }
template <class Char>
inline uint64_t cast64be(const Char* s) { uint64_t n; cast_be(s, n); return n; }
inline double   cast_double(const char* s) { double n; cast_be(s, n); return n; }

} // namespace util

#endif // _UTIL_ENDIAN_HPP_
