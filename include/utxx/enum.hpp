//----------------------------------------------------------------------------
/// \file   enum.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief This file defines enum stringification declaration macro.
//----------------------------------------------------------------------------
// Copyright (c) 2014 Serge Aleynikov <saleyn@gmail.com>
// Created: 2014-09-31
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
#pragma once

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <utxx/string.hpp>
#include <cassert>

#define UTXX_DEFINE_CUSTOM_ENUM_INFO(name) template<typename> class name
#define UTXX_DEFINE_ENUM_INFO UTXX_DEFINE_CUSTOM_ENUM_INFO(reflection)

// Enum declaration:
//  #include <utxx/enum.hpp>
//
//  UTXX_DEFINE_ENUM_INFO(MyEnumT)
//  UTXX_DEFINE_ENUM(MyEnumT,
//                      Apple,
//                      Pear,
//                      Grape
//                  );
//
// Sample usage:
//  MyEnumT val = reflection<MyEnumT>::from_string("Pear");
//  std::cout << "Value: " << to_string(val) << std::endl;
//  std::cout << "Value: " << val            << std::endl;
//
#define UTXX_DEFINE_ENUM(enum_name, ...)                                    \
    UTXX_DEFINE_CUSTOM_ENUM(reflection, enum_name, __VA_ARGS__)

#define UTXX_DEFINE_CUSTOM_ENUM(einfo, enum_name, ...)                      \
    enum class enum_name {                                                  \
        __VA_ARGS__,                                                        \
        UNDEFINED                                                           \
    };                                                                      \
                                                                            \
    template <>                                                             \
    struct einfo<enum_name> : utxx::enum_info<einfo<enum_name>> {           \
        static const size_t s_size = BOOST_PP_VARIADIC_SIZE(__VA_ARGS__);   \
        static const char*  s_values[s_size];                               \
     };                                                                     \
                                                                            \
    const char* einfo<enum_name>::s_values[] = {                            \
        BOOST_PP_SEQ_ENUM(                                                  \
            BOOST_PP_SEQ_TRANSFORM(                                         \
                UTXX_INTERNAL_ENUM_STRINGIFY,,                              \
                BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)                       \
        ))                                                                  \
    };                                                                      \
                                                                            \
    inline const char* to_string(enum_name a) {                             \
        return einfo<enum_name>::to_string(a);                              \
    }                                                                       \
                                                                            \
    inline std::ostream& operator<< (std::ostream& out, enum_name a) {      \
        return out << einfo<enum_name>::to_string(a);                       \
    }

// Internal macro for supporting BOOST_PP_SEQ_TRANSFORM
#define UTXX_INTERNAL_ENUM_STRINGIFY(x, data, item) BOOST_PP_STRINGIZE(item)

namespace utxx {

    template<typename> struct enum_info;

    template <template<typename> class Derived, typename Enum>
    struct enum_info<Derived<Enum>> {
        using type = Enum;
        static constexpr size_t size()  { return Derived<Enum>::s_size;  }
        static constexpr Enum   begin() { return (Enum)0;                }
        static constexpr Enum   end()   { return Enum::UNDEFINED;        }

        static const char* to_string(Enum a) {
            assert((size_t)a < utxx::length(Derived<Enum>::s_values));
            return Derived<Enum>::s_values[utxx::to_underlying(a)];
        }

        static Enum from_string(const std::string& a_val, bool a_nocase=false) {
            return utxx::find_index<Enum>
                (Derived<Enum>::s_values, a_val, Enum::UNDEFINED, a_nocase);
        }
    };

} // namespace utxx
