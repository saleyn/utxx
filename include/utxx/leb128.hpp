//----------------------------------------------------------------------------
/// \file  leb128.hpp
//----------------------------------------------------------------------------
/// \brief Little Endian Binary integer encoding
/// \see https://en.wikipedia.org/wiki/LEB128
//----------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-08-05
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2015 Serge Aleynikov <saleyn@gmail.com>

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

namespace utxx {

/// Write an unsigned LEB-encoded integer to \a p
template <unsigned Padding = 0>
inline int encode_uleb128(uint64_t a_value, char* p) {
    auto* q = p;
    do {
        uint8_t byte = a_value & 0x7f;
        a_value >>= 7;
        if (a_value != 0 || Padding != 0)
            byte |= 0x80; // This bit means that more bytes will follow.
        *p++ = byte;
    } while (a_value);

    if (Padding) {
        // Pad with 0x80 and null-terminate it.
        for (int n=Padding; n != 1; --n)
            *(uint8_t*)p++ = '\x80';
        *p++ = '\x00';
    }
    return p - q;
}

/// Write a signed LEB128-encoded integer to \a p
inline int encode_sleb128(int64_t a_value, char* p) {
    bool  more;
    auto* q = p;
    do {
        uint8_t byte = a_value & 0x7f;
        a_value >>= 7;
        more = !((!a_value      && (byte & 0x40) == 0) ||
                 (a_value == -1 && (byte & 0x40) != 0));
        if (more)
            byte |= 0x80; // This bit means that more bytes will follow.
        *(uint8_t*)p++ = byte;
    } while (more);

    return p - q;
}

/// Decode an unsigned LEB128-encoded value.
inline uint64_t decode_uleb128(const char* p) {
    uint64_t   value = 0;
    unsigned   shift = 0;
    do {
        value += uint64_t(*(uint8_t*)p & 0x7f) << shift;
        shift += 7;
    } while (*(uint8_t*)p++ >= 128);
    return value;
}

inline uint64_t decode_uleb128(const char* p, int& sz) {
    auto         q = p;
    uint64_t value = 0;
    int      shift = 0;
    do {
        value += uint64_t(*(uint8_t*)p & 0x7f) << shift;
        shift += 7;
    } while (*(uint8_t*)p++ >= 128);
    sz = p - q;
    return value;
}

/// Decode a signed LEB128-encoded value.
inline int64_t decode_sleb128(const char* p) {
    uint64_t value = 0;
    int      shift = 0;
    uint8_t  byte;
    do {
        byte = *(uint8_t*)p++;
        value |= ((byte & 0x7f) << shift);
        shift += 7;
    } while (byte >= 128);
    // Sign extend negative numbers.
    return (byte & 0x40) ? (value |= (-1ull) << shift) : value;
}

/// Decode a signed LEB128-encoded value.
inline int64_t decode_sleb128(const char* p, int& sz) {
    auto         q = p;
    uint64_t value = 0;
    int      shift = 0;
    uint8_t  byte;
    do {
        byte = *(uint8_t*)p++;
        value |= ((byte & 0x7f) << shift);
        shift += 7;
    } while (byte >= 128);
    sz = p - q;
    // Sign extend negative numbers.
    return (byte & 0x40) ? (value |= (-1ull) << shift) : value;
}

/// Get the size of the ULEB128-encoded value.
inline int encoded_uleb128_size(uint64_t a_value) {
    int size = 0;
    do {
        a_value >>= 7;
        size += sizeof(int8_t);
    } while (a_value);
    return size;
}

/// Get the size of the SLEB128-encoded value.
inline int encoded_sleb128_size(int64_t a_value) {
    int  size = 0;
    int  sign = a_value >> (8 * sizeof(a_value) - 1);
    bool is_more;

    do {
        unsigned byte = a_value & 0x7f;
        a_value >>= 7;
        is_more   = a_value != sign || ((byte ^ sign) & 0x40) != 0;
        size     += sizeof(int8_t);
    } while (is_more);
    return size;
}

} // namespace utxx