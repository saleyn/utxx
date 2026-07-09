//----------------------------------------------------------------------------
/// \file   endian.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Endian-handling store/load primitives (no Boost dependency).
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
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

#include <cstdint>
#include <cstring>

#if __cplusplus >= 202002L
#  include <bit>
#endif

namespace utxx {

// ---------------------------------------------------------------------------
// Host <-> network byte-order helpers (portable, no UB, compiles to bswap)
// ---------------------------------------------------------------------------
namespace detail {

template <typename T>
inline T bswap(T v) noexcept {
    static_assert(sizeof(T) == 1 || sizeof(T) == 2 ||
                  sizeof(T) == 4 || sizeof(T) == 8, "unsupported width");
    if constexpr (sizeof(T) == 1) return v;
#if __cplusplus >= 202002L
    else if constexpr (sizeof(T) == 2) return static_cast<T>(std::byteswap(static_cast<uint16_t>(v)));
    else if constexpr (sizeof(T) == 4) return static_cast<T>(std::byteswap(static_cast<uint32_t>(v)));
    else                               return static_cast<T>(std::byteswap(static_cast<uint64_t>(v)));
#else
    else if constexpr (sizeof(T) == 2) return static_cast<T>(__builtin_bswap16(static_cast<uint16_t>(v)));
    else if constexpr (sizeof(T) == 4) return static_cast<T>(__builtin_bswap32(static_cast<uint32_t>(v)));
    else                               return static_cast<T>(__builtin_bswap64(static_cast<uint64_t>(v)));
#endif
}

// True on little-endian systems (all modern x86/x86_64/ARM)
static constexpr bool s_little_endian = (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);

} // namespace detail

// Store T into raw bytes in big-endian order
template <typename T>
inline void store_be(char* s, T n) noexcept {
    if constexpr (detail::s_little_endian)
        n = detail::bswap(n);
    std::memcpy(s, &n, sizeof(T));
}

// Store T into raw bytes in little-endian order
template <typename T>
inline void store_le(char* s, T n) noexcept {
    if constexpr (!detail::s_little_endian)
        n = detail::bswap(n);
    std::memcpy(s, &n, sizeof(T));
}

// Specialisations for double (reinterpret as uint64)
inline void store_be(char* s, double n) noexcept {
    static_assert(sizeof(double) == sizeof(uint64_t));
    uint64_t i; std::memcpy(&i, &n, 8);
    store_be(s, i);
}
inline void store_le(char* s, double n) noexcept {
    static_assert(sizeof(double) == sizeof(uint64_t));
    uint64_t i; std::memcpy(&i, &n, 8);
    store_le(s, i);
}

// Load T from big-endian raw bytes
template <typename T>
inline void cast_be(const char* s, T& n) noexcept {
    std::memcpy(&n, s, sizeof(T));
    if constexpr (detail::s_little_endian)
        n = detail::bswap(n);
}

// Load T from little-endian raw bytes
template <typename T>
inline void cast_le(const char* s, T& n) noexcept {
    std::memcpy(&n, s, sizeof(T));
    if constexpr (!detail::s_little_endian)
        n = detail::bswap(n);
}

// Specialisations for double
inline void cast_be(const char* s, double& n) noexcept {
    static_assert(sizeof(double) == sizeof(uint64_t));
    uint64_t i; cast_be(s, i);
    std::memcpy(&n, &i, 8);
}
inline void cast_le(const char* s, double& n) noexcept {
    static_assert(sizeof(double) == sizeof(uint64_t));
    uint64_t i; cast_le(s, i);
    std::memcpy(&n, &i, 8);
}

// Return-value versions
template <typename T, typename Ch>
inline T cast_be(const Ch* s) { T n; cast_be(reinterpret_cast<const char*>(s), n); return n; }
template <typename T, typename Ch>
inline T cast_le(const Ch* s) { T n; cast_le(reinterpret_cast<const char*>(s), n); return n; }

// Advancing put: write T and advance pointer
template <typename T, typename Ch>
inline void put_be(Ch*& s, T n) { store_be(reinterpret_cast<char*>(s), n); s += sizeof(T); }
template <typename T, typename Ch>
inline void put_le(Ch*& s, T n) { store_le(reinterpret_cast<char*>(s), n); s += sizeof(T); }

// Advancing get: read T and advance pointer
template <typename T>
inline void get_be(const char*& s, T& n) { cast_be(s, n); s += sizeof(T); }
template <typename T, typename Ch>
inline void get_le(const Ch*& s, T& n) { cast_le(reinterpret_cast<const char*>(s), n); s += sizeof(T); }

template <typename T, typename Ch>
inline T get_be(const Ch*& s) { T n; get_be(reinterpret_cast<const char*&>(s), n); return n; }
template <typename T, typename Ch>
inline T get_le(const Ch*& s) { T n; get_le(s, n); return n; }

inline void get_be(const char*& s, double& n) { cast_be(s, n); s += sizeof(double); }
inline void get_le(const char*& s, double& n) { cast_le(s, n); s += sizeof(double); }

// Convenience aliases
template <class Ch> inline void     put8   (Ch*& s, uint8_t  n) { put_be(s, n); }
template <class Ch> inline void     put16be(Ch*& s, uint16_t n) { put_be(s, n); }
template <class Ch> inline void     put32be(Ch*& s, uint32_t n) { put_be(s, n); }
template <class Ch> inline void     put64be(Ch*& s, uint64_t n) { put_be(s, n); }
template <class Ch> inline void     put16le(Ch*& s, uint16_t n) { put_le(s, n); }
template <class Ch> inline void     put32le(Ch*& s, uint32_t n) { put_le(s, n); }
template <class Ch> inline void     put64le(Ch*& s, uint64_t n) { put_le(s, n); }

template <class Ch> inline uint8_t  get8   (const Ch*& s) { uint8_t  n; get_be(s, n); return n; }
template <class Ch> inline uint16_t get16be(const Ch*& s) { uint16_t n; get_be(s, n); return n; }
template <class Ch> inline uint32_t get32be(const Ch*& s) { uint32_t n; get_be(s, n); return n; }
template <class Ch> inline uint64_t get64be(const Ch*& s) { uint64_t n; get_be(s, n); return n; }
template <class Ch> inline uint16_t get16le(const Ch*& s) { uint16_t n; get_le(s, n); return n; }
template <class Ch> inline uint32_t get32le(const Ch*& s) { uint32_t n; get_le(s, n); return n; }
template <class Ch> inline uint64_t get64le(const Ch*& s) { uint64_t n; get_le(s, n); return n; }

template <class Ch> inline uint8_t  cast8   (const Ch* s) { return static_cast<uint8_t>(s[0]); }
template <class Ch> inline uint16_t cast16be(const Ch* s) { uint16_t n; cast_be(reinterpret_cast<const char*>(s), n); return n; }
template <class Ch> inline uint32_t cast32be(const Ch* s) { uint32_t n; cast_be(reinterpret_cast<const char*>(s), n); return n; }
template <class Ch> inline uint64_t cast64be(const Ch* s) { uint64_t n; cast_be(reinterpret_cast<const char*>(s), n); return n; }
inline double cast_double_be(const char* s) { double n; cast_be(s, n); return n; }

template <class Ch> inline uint16_t cast16le(const Ch* s) { uint16_t n; cast_le(reinterpret_cast<const char*>(s), n); return n; }
template <class Ch> inline uint32_t cast32le(const Ch* s) { uint32_t n; cast_le(reinterpret_cast<const char*>(s), n); return n; }
template <class Ch> inline uint64_t cast64le(const Ch* s) { uint64_t n; cast_le(reinterpret_cast<const char*>(s), n); return n; }
inline double cast_double_le(const char* s) { double n; cast_le(s, n); return n; }

} // namespace utxx
