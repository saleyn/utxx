//----------------------------------------------------------------------------
/// \file stream_io.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// Defines functions for stream input/output
//----------------------------------------------------------------------------
// Copyright (C) 2016 Serge Aleynikov <saleyn@gmail.com>
// Created: 2016-02-26
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is a part of utxx open-source project.

Copyright (C) 2016 Serge Aleynikov <saleyn@gmail.com>

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

#include <iosfwd>
#include <utxx/convert.hpp>
#include <utxx/container/stack_container.hpp>

namespace utxx {

//------------------------------------------------------------------------------
/// Read \a a_cnt values from the input stream.
/// @param in       input stream.
/// @param a_output array of \a a_cnt double values.
/// @param a_fields array of \a a_cnt field positions (1-based, ascending order).
///                 If this value is NULL, the values are read from the input
///                 stream in consequitive order disregarding field positions.
/// @param a_cnt    count of values to read (must be > 0).
/// @param a_convert lambda used to convert a string value to type \a T:
///   <code>const char* (const char* begin, const char* end, T& outout);</code>
///   The function must return NULL if conversion is unsuccessful, or a pointer
///   past last successfully parsed character otherwise.
/// @param a_delims array of delimiting characters (default: " \t")
/// @return true if successfully read \a a_cnt values.
//------------------------------------------------------------------------------
template <typename T = double, int StrSize = 256, class Convert>
bool read_values(std::istream& in, T* a_output, int* a_fields, int a_cnt,
                 const Convert& a_convert, const char a_delim[] = " \t")
{
    assert(a_cnt);

    basic_stack_string<StrSize> str;

    auto& line = str.container();

    if (a_fields == nullptr) {
        for (int i=0; i < a_cnt; ++i) {
            if (in.eof())  return false;
            in >> *a_output++;
            if (in.fail()) {
                in.clear();
                std::getline(in, line);
                return false;
            }
        }
    }
    else if (!std::getline(in, line))
        return false;
    else {
        const char* p = &*line.begin();
        const char* e = &*line.end();

        int fld = 0;

        auto ws      = [=](char c)    { return strchr(a_delim, c);    };
        auto skip_ws = [&p, e, &ws]() { while (ws(*p) && p != e) p++; };

        for (int i=0; i < a_cnt; ++i, ++a_fields, ++a_output) {
            while (p != e) {
                skip_ws();
                if (++fld == *a_fields)
                    break;
                while (!ws(*p) && p != e) p++;
            }

            if (fld != *a_fields)
                return false;

            p = a_convert(p, e, *a_output);

            if (!p)
                return false;
        }
    }

    return true;
}

} // namespace utxx