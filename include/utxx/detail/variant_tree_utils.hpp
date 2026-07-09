//----------------------------------------------------------------------------
/// \file  variant_tree_utils.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Various utils for varian_tree parsing
//----------------------------------------------------------------------------
// Created: 2014-02-22
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2014 Serge Aleynikov <saleyn@gmail.com>

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

#ifndef _UTXX_VARIANT_TREE_UTILS_HPP_
#define _UTXX_VARIANT_TREE_UTILS_HPP_

#include <utxx/config.h>
#include <utxx/variant_tree.hpp>

namespace utxx {

    namespace detail {

        // Replicate boost info_parser::create_escapes: escape special chars
        template<class Ch>
        std::basic_string<Ch> create_escapes(const std::basic_string<Ch>& s) {
            std::basic_string<Ch> result;
            result.reserve(s.size());
            for (Ch c : s) {
                switch (c) {
                    case Ch('\\'): result += Ch('\\'); result += Ch('\\'); break;
                    case Ch('"') : result += Ch('\\'); result += Ch('"');  break;
                    case Ch('\n'): result += Ch('\\'); result += Ch('n');  break;
                    case Ch('\r'): result += Ch('\\'); result += Ch('r');  break;
                    case Ch('\t'): result += Ch('\\'); result += Ch('t');  break;
                    default:      result += c;                             break;
                }
            }
            return result;
        }

        // A key is "simple" if it contains no whitespace, quotes, braces,
        // equal signs, or hash signs (mirrors boost info_parser::is_simple_key)
        template<class Ch>
        bool is_simple_key(const std::basic_string<Ch>& key) {
            if (key.empty()) return false;
            for (Ch c : key) {
                if (c == Ch(' ')  || c == Ch('\t') || c == Ch('\n') ||
                    c == Ch('\r') || c == Ch('"')  || c == Ch('{')  ||
                    c == Ch('}')  || c == Ch('=')  || c == Ch('#'))
                    return false;
            }
            return true;
        }
    }

} // namespace utxx

#endif // _UTXX_VARIANT_TREE_UTILS_HPP_