//----------------------------------------------------------------------------
/// \file   base64.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief  Functions for base64 encoding/decoding
//----------------------------------------------------------------------------
// Copyright (c) 2018 Serge Aleynikov <saleyn@gmail.com>
// Created: 2018-04-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2018 Serge Aleynikov <saleyn@gmail.com>

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

/*
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
*/
#include <string>
#include <sstream>
#include <cmath>
#include <boost/utility.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/cstdint.hpp>
#include <vector>
#include <utxx/print.hpp>

//-----------------------------------------------------------------------------
// Base64
//-----------------------------------------------------------------------------

namespace utxx {

/*
//------------------------------------------------------------------------------
/// Base64 decoder
//------------------------------------------------------------------------------
inline std::string decode_base64(const std::string& str) {
    using namespace boost::archive::iterators;
    using cit = std::string::const_iterator;
    using it  = transform_width<binary_from_base64<cit>, 8, 6>;
    return boost::algorithm::trim_right_copy_if
        (std::string(it(std::begin(str)), it(std::end(str))), [](char c) {
            return c == '\0';
        });
}

//------------------------------------------------------------------------------
/// Base64 encoder
//------------------------------------------------------------------------------
inline std::string encode_base64(const std::string& str) {
    using namespace boost::archive::iterators;
    using cit = std::string::const_iterator;
    using it  = transform_width<binary_from_base64<cit>, 8, 6>;
    auto  tmp = std::string(it(std::begin(str)), it(std::end(str)));
    auto  n   = 3 - (str.size() & 3);
    return n && n < 3 ? tmp.append(n, '=') : tmp;
}
*/

class base64 : public boost::noncopyable {
public:
    enum encoding { STANDARD, URL };
  
    static unsigned encode_size(unsigned input_sz) { return (((input_sz<<2) / 3) + 3) & ~3; }

    static std::string encode(
        const std::string& s, encoding enc = STANDARD, bool eq_trail = true)
    {
        return encode(
            reinterpret_cast<const uint8_t*>(s.c_str()), s.size(), enc, eq_trail);
    }
  
    static std::string encode(
        const uint8_t* s, unsigned int size, encoding enc = STANDARD, bool eq_trail = true)
    {
        buffered_print os;
        os.reserve(encode_size(size));
        auto n = encode_unchecked(s, size, os.str(), enc, eq_trail);
        os.advance(n);
        return os.to_string();
    }

    static unsigned encode_unchecked(
        const uint8_t* s, unsigned size, char* dest, encoding enc = STANDARD, bool eq_trail = true)
    {
        unsigned j = 0;
  
        for (unsigned i=0; i < size;) {
            auto c1 = s[i++] & 0xff;
            if (i == size) {
                dest[j++] = enctable(enc)[c1 >> 2];
                dest[j++] = enctable(enc)[(c1 & 0x3) << 4];
                if (eq_trail) { dest[j++] = '='; dest[j++] = '='; }
                break;
            }
            auto c2 = s[i++];
            if (i == size) {
                dest[j++] = enctable(enc)[  c1 >> 2];
                dest[j++] = enctable(enc)[((c1 & 0x3) << 4)|((c2 & 0xf0) >> 4)];
                dest[j++] = enctable(enc)[ (c2 & 0xf) << 2];
                if (eq_trail) dest[j++] = '=';
                break;
            }
            auto c3 = s[i++];
            dest[j++] = enctable(enc)[  c1 >> 2];
            dest[j++] = enctable(enc)[((c1 & 0x3) << 4) | ((c2 & 0xf0) >> 4)];
            dest[j++] = enctable(enc)[((c2 & 0xf) << 2) | ((c3 & 0xc0) >> 6)];
            dest[j++] = enctable(enc)[  c3 & 0x3f];
        }
        return j;
    }
  
    static std::vector<char> decode(const std::string& s, encoding enc = STANDARD) {
        std::vector<char> dest;
        decode(s, dest, enc);
        return dest;
    }
  
    static std::string decode_str(const std::string& s, encoding enc = STANDARD) {
        auto v = decode(s, enc);
        return std::string(v.begin(), v.end());
    }

    /// Decode base64 string into "dest"
    /// @param dest can be an STL container or string
    template <typename T>
    static unsigned decode(const std::string& s, T& dest, encoding enc = STANDARD) {
        auto size = s.size();
        auto dest_sz = std::ceil(static_cast<float>(size) * 0.75f);

        dest.clear();
        dest.resize(static_cast<unsigned int>(dest_sz));

        auto len = decode_unchecked(s.c_str(), size, &dest[0], enc);
        dest.resize(len);

        return len;
    }

    template <typename Ch = uint8_t>
    static unsigned decode_unchecked(const char* s, size_t size, Ch* out, encoding enc = STANDARD) {
        auto j = 0u;

        for (uint i=0; i < size;) {
            char c1;
            do {c1 = dectable(enc)[s[i++] & 0xff];} while (i < size && c1 == -1);
            if (c1 == -1) break;
      
            char c2;
            do {c2 = dectable(enc)[s[i++] & 0xff];} while (i < size && c2 == -1);
            if (c2 == -1) break;

            out[j++] = static_cast<Ch>(((c1 << 2)|((c2 & 0x30) >> 4)) & 0xff);

            char c3;
            do {
                c3 = s[i++] & 0xff;
                if (c3 == 61) return j;
                c3 = dectable(enc)[int(c3)];
            } while (i < size && c3 == -1);

            if (c3 == -1) break;

            out[j++] = static_cast<Ch>((((c2 & 0xf) << 4)|((c3 & 0x3c) >> 2)) & 0xff);

            char c4;
            do {
                c4 = s[i++] & 0xff;
                if (c4 == 61) return j;
                c4 = dectable(enc)[int(c4)];
            } while (i < size && c4 == -1);
            if (c4 == -1) break;

            out[j++] = static_cast<Ch>((((c3 & 0x03) << 6)| c4) & 0xff);
        }
        return j;
    }
  
private:
    static const char* enctable(encoding enc) {
        static const char s_enctable[][65] =  {
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/", // STD
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"  // URL
        };
        return s_enctable[enc];
    };
  
    static const char* dectable(encoding enc) {
        static const char s_dectable[][128] = {
            // STD
            {
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
                52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
                -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
                15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
                -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
            },
            // URL
            {
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1,
                52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
                -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
                15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63,
                -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
            }
        };
        return s_dectable[enc];
    }
};

} // namespace utxx
