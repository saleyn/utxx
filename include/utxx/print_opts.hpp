//----------------------------------------------------------------------------
/// \file  print_opts.hpp
//----------------------------------------------------------------------------
/// \brief This file contains implementation of printing functions.
//----------------------------------------------------------------------------
// Copyright (c) 2016 Serge Aleynikov <saleyn@gmail.com>
// Created: 2016-08-04
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

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

namespace utxx {

enum class print_opts {
    dec,
    hex,
    printable_string,
    printable_or_hex,
    printable_or_dec
};

template <class StreamT, class Char = char>
inline StreamT& output(StreamT& out, const Char* p, const Char* end,
                       print_opts a_opts = print_opts::printable_or_dec,
                       const char* a_sep = ",", const char* a_hex_prefix = "",
                       const char* a_printable_quote = "",
                       const char* a_out_prefix = "",
                       const char* a_out_suffix = "")
{
    bool printable = true;
    auto hex       = [=, &out](auto ch) {
        static const char s_hex[] = "0123456789abcdef";
        uint8_t c = uint8_t(ch);
        out << a_hex_prefix << (s_hex[(c & 0xF0) >> 4]) << (s_hex[(c & 0x0F)]);
    };
    if (a_out_prefix[0] != '\0')
        out << a_out_prefix;
    if (a_opts == print_opts::hex) goto HEX;
    if (a_opts == print_opts::dec) goto DEC;
    if (a_opts == print_opts::printable_string) {
        out << a_printable_quote;
        for (auto q = (const char*)p; q != (const char*)end; ++q)
            switch (*q) {
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default  :
                    out << ((*q < ' ' || *q > '~') ? '.' : *q);
                    break;
            }
        out << a_printable_quote;
        goto DONE;
    }
    for (auto q = (const char*)p; q != (const char*)end; ++q)
        if ((*q != '\t' && *q != '\n' && *q < ' ') || *q  > '~') {
            printable = false;
            break;
        }

    if (printable && (a_opts == print_opts::printable_or_hex||
                      a_opts == print_opts::printable_or_dec)) {
        out << a_printable_quote;
        while (p != end) out << *p++;
        out << a_printable_quote;
        goto  DONE;
    }
    if (a_opts == print_opts::printable_or_hex) goto HEX;
    if (a_opts == print_opts::printable_string) goto DONE;
    goto DEC;

DEC:
    if    (p != end)   out <<          (int)*(uint8_t*)p++;
    while (p != end) { out << a_sep << (int)*(uint8_t*)p++; }
    goto DONE;

HEX:
    if    (p != end)                 hex(*p++);
    while (p != end) { out << a_sep; hex(*p++); }

DONE:
    if (a_out_suffix[0] != '\0')
        out << a_out_suffix;
    return out;
}

} // namespace utxx