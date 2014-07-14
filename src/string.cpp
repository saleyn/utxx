//----------------------------------------------------------------------------
/// \file  string.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of general purpose functions for string processing
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-05-06
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

#include <utxx/string.hpp>

namespace utxx {

std::string to_bin_string(const char* buf, size_t sz, bool hex, bool readable) {
    std::stringstream out;
    const char* begin = buf, *end = buf + sz;
    bool printable = readable;
    if (!hex && readable)
        for(const char* p = begin; p != end; ++p)
            if (*p < ' ' || *p == 127 || *p > 254) {
                printable = false;
                break;
            }
    out << "<<" << (printable ? "\"" : "");
    for(const char* p = begin; p != end; ++p) {
        out << (p == begin || printable ? "" : ",");
        if (printable)
            out << *p;
        else {
            if (hex) out << std::hex;
            out << (int)*(unsigned char*)p;
        }
    }
    out << (printable ? "\"" : "") << ">>";
    return out.str();
}

} // namespace utxx
