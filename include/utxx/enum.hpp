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

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <utxx/string.hpp>
#include <cassert>

// Enum declaration:
//  #include <utxx/enum.hpp>
//
//  UTXX_DEFINE_ENUM(MyEnumT,
//                      Apple,
//                      Pear,
//                      Grape
//                  );
//
// Sample usage:
//  MyEnumT val = MyEnumT::from_string("Pear");
//  std::cout << "Value: " << to_string(val) << std::endl;
//  std::cout << "Value: " << val            << std::endl;
//
#define UTXX_DEFINE_ENUM(ENUM, ...)                                         \
    struct ENUM {                                                           \
        enum etype {                                                        \
            UNDEFINED,                                                      \
            __VA_ARGS__,                                                    \
            _END_                                                           \
        };                                                                  \
                                                                            \
    private:                                                                \
        static const size_t s_size = 1+BOOST_PP_VARIADIC_SIZE(__VA_ARGS__); \
        static const char** names() {                                       \
            static const char* s_names[] = {                                \
                "UNDEFINED",                                                \
                BOOST_PP_SEQ_ENUM(                                          \
                    BOOST_PP_SEQ_TRANSFORM(                                 \
                        UTXX_INTERNAL_ENUM_STRINGIFY, ,                     \
                        BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)               \
                ))                                                          \
            };                                                              \
            return s_names;                                                 \
        };                                                                  \
                                                                            \
        static const char* name(size_t n) {                                 \
            assert(n < s_size);                                             \
            return names()[n];                                              \
        };                                                                  \
                                                                            \
        etype  m_val;                                                       \
                                                                            \
        explicit ENUM(size_t v) : m_val(etype(v)) { assert(v < s_size); }   \
    public:                                                                 \
        ENUM()                  : m_val(UNDEFINED) {}                       \
        constexpr ENUM(etype v) : m_val(v) {}                               \
                                                                            \
        operator           etype() const { return m_val; }                  \
        bool               empty() const { return m_val == UNDEFINED; }     \
                                                                            \
        const  char*       to_string()   { return name(size_t(m_val)); }    \
        static const char* to_string(etype a) { return ENUM(a).to_string();}\
                                                                            \
        static ENUM from_string(const std::string& a, bool a_nocase=false){ \
            return utxx::find_index<etype>                                  \
                (names(), s_size, a, etype::UNDEFINED, a_nocase);           \
        }                                                                   \
                                                                            \
        inline friend std::ostream& operator<< (std::ostream& out, ENUM a) {\
            return out << ENUM::to_string(a);                               \
        }                                                                   \
                                                                            \
        static constexpr size_t size()  { return s_size-1; }                \
        static constexpr ENUM   first() { return etype(1); }                \
        static constexpr ENUM   last()  { return etype(size()); }           \
        static constexpr ENUM   end()   { return _END_; }                   \
                                                                            \
        template <typename Visitor>                                         \
        static void for_each(const Visitor& a_fun) {                        \
            for (size_t i=1; i < s_size; i++)                               \
                if (!a_fun(etype(i)))                                       \
                    break;                                                  \
        }                                                                   \
    }

// Internal macro for supporting BOOST_PP_SEQ_TRANSFORM
#define UTXX_INTERNAL_ENUM_STRINGIFY(x, _, item) BOOST_PP_STRINGIZE(item)
