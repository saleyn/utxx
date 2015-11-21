//----------------------------------------------------------------------------
/// \file   enum.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief This file defines enum stringification declaration macro.
//----------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-04-11
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>

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
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/variadic_seq_to_seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <utxx/string.hpp>
#include <cassert>
#include <map>

// The difference between enum.hpp and enumx.hpp is that UTXX_ENUMX
// allows to assign specific values to the enumerated constants.
//
// Enum declaration:
//  #include <utxx/enumx.hpp>
//
//  UTXX_ENUMX(MyEnumT,
//               char,          // This is enum storage type
//               ' '            // This is an "UNDEFINED" value
//               (Apple, 'x')
//               (Pear,  'y')
//               (Grape)        // Grape will be equal to 'y'+1
//            );
//
// Sample usage:
//  MyEnumT val = MyEnumT::from_string("Pear");
//  std::cout << "Value: " << to_string(val) << std::endl;
//  std::cout << "Value: " << val            << std::endl;
//
#define UTXX_ENUMX(ENUM, TYPE, UndefValue, ...)                              \
    struct ENUM {                                                            \
        enum type : TYPE {                                                   \
            UNDEFINED = (UndefValue),                                        \
            BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(                        \
                UTXX_INTERNAL_ENUM_VAL, _,                                   \
                BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)))                  \
        };                                                                   \
                                                                             \
        explicit  ENUM(long v) : m_val(type(v))   {}                         \
        constexpr ENUM()       : m_val(UNDEFINED) {}                         \
        constexpr ENUM(type v) : m_val(v) {}                                 \
                                                                             \
        ENUM(ENUM&&)                 = default;                              \
        ENUM(ENUM const&)            = default;                              \
                                                                             \
        ENUM& operator=(ENUM const&) = default;                              \
        ENUM& operator=(ENUM&&)      = default;                              \
                                                                             \
        constexpr operator type()      const { return m_val; }               \
        constexpr bool     empty()     const { return m_val == UNDEFINED; }  \
        const std::string& to_string() const {                               \
            auto it = names().find(m_val);                                   \
            return (it == names().end() ? null_pair() : *it).second;         \
        }                                                                    \
                                                                             \
        static const std::string&                                            \
                           to_string(type a){ return ENUM(a).to_string();  } \
        const char*        c_str()     const { return to_string().c_str(); } \
        static const char* c_str(type a)    { return to_string(a).c_str(); } \
                                                                             \
        static ENUM from_string(const char* a, bool a_nocase=false){         \
            auto f = a_nocase ? &strcasecmp : &strcmp;                       \
            for (auto& t : names())                                          \
                if (f(t.second.c_str(), a) == 0)                             \
                    return t.first;                                          \
            return UNDEFINED;                                                \
        }                                                                    \
                                                                             \
        static ENUM from_string(const std::string& a, bool a_nocase=false) { \
            return from_string(a.c_str(), a_nocase);                         \
        }                                                                    \
                                                                             \
        inline friend std::ostream& operator<< (std::ostream& out, ENUM a) { \
            return out << ENUM::to_string(a);                                \
        }                                                                    \
                                                                             \
        static constexpr size_t size()  { return s_size-1; }                 \
                                                                             \
        template <typename Visitor>                                          \
        static void for_each(const Visitor& a_fun) {                         \
            for (auto it = ++names().begin(), e = names().end(); it!=e; ++it)\
                if (!a_fun(it->first))                                       \
                    break;                                                   \
        }                                                                    \
                                                                             \
    private:                                                                 \
        static const std::pair<const type, std::string>& null_pair() {       \
            static const std::pair<const type, std::string> s_val =          \
                std::make_pair(UNDEFINED, "UNDEFINED");                      \
            return s_val;                                                    \
        }                                                                    \
        static const size_t s_size =                                         \
            1+BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__));  \
        static const std::map<type, std::string>& names() {                  \
            static const std::map<type, std::string> s_names{                \
                null_pair(),                                                 \
                BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(                    \
                    UTXX_INTERNAL_ENUM_PAIR, _,                              \
                    BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)                \
                ))                                                           \
            };                                                               \
            return s_names;                                                  \
        }                                                                    \
                                                                             \
        type m_val;                                                          \
    }

// DEPRECATED!!! Use UTXX_ENUMX!
//
// Same as UTXX_ENUMX except that the enum is untyped.
//
// The difference between enum.hpp and enumx.hpp is that UTXX_DEFINE_ENUMX
// allows to assign specific values to the enumerated constants.
//
// Enum declaration:
//  #include <utxx/enumx.hpp>
//
//  UTXX_DEFINE_ENUMX(MyEnumT,  ' '  /* This is an "UNDEFINED" value */,
//                      (Apple, 'x')
//                      (Pear,  'y')
//                      (Grape)      /* Grape will be equal to 'y'+1 */
//                   );
//
// Sample usage:
//  MyEnumT val = MyEnumT::from_string("Pear");
//  std::cout << "Value: " << to_string(val) << std::endl;
//  std::cout << "Value: " << val            << std::endl;
//
#define UTXX_DEFINE_ENUMX(ENUM, UndefValue, ...)                             \
    struct [[deprecated]] ENUM {                                             \
        enum etype {                                                         \
            UNDEFINED = (UndefValue),                                        \
            BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(                        \
                UTXX_INTERNAL_ENUM_VAL, _,                                   \
                BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)))                  \
        };                                                                   \
                                                                             \
    private:                                                                 \
        static const std::pair<const etype, std::string>& null_pair() {      \
            static const std::pair<const etype, std::string> s_val =         \
                std::make_pair(UNDEFINED, "UNDEFINED");                      \
            return s_val;                                                    \
        }                                                                    \
        static const size_t s_size =                                         \
            1+BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__));  \
        static const std::map<etype, std::string>& names() {                 \
            static const std::map<etype, std::string> s_names{               \
                null_pair(),                                                 \
                BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(                    \
                    UTXX_INTERNAL_ENUM_PAIR, _,                              \
                    BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)                \
                ))                                                           \
            };                                                               \
            return s_names;                                                  \
        }                                                                    \
                                                                             \
        etype m_val;                                                         \
                                                                             \
    public:                                                                  \
        explicit  ENUM(long v)  : m_val(etype(v))  {}                        \
        constexpr ENUM()        : m_val(UNDEFINED) {}                        \
        constexpr ENUM(etype v) : m_val(v) {}                                \
                                                                             \
        ENUM(ENUM&&)                 = default;                              \
        ENUM(ENUM const&)            = default;                              \
                                                                             \
        ENUM& operator=(ENUM const&) = default;                              \
        ENUM& operator=(ENUM&&)      = default;                              \
                                                                             \
        constexpr operator etype()     const { return m_val; }               \
        constexpr bool     empty()     const { return m_val == UNDEFINED; }  \
        const std::string& to_string() const {                               \
            auto it = names().find(m_val);                                   \
            return (it == names().end() ? null_pair() : *it).second;         \
        }                                                                    \
                                                                             \
        static const std::string&                                            \
                           to_string(etype a){ return ENUM(a).to_string();  }\
        const char*        c_str()     const { return to_string().c_str();  }\
        static const char* c_str(etype a)    { return to_string(a).c_str(); }\
                                                                             \
        static ENUM from_string(const char* a, bool a_nocase=false){         \
            auto f = a_nocase ? &strcasecmp : &strcmp;                       \
            for (auto& t : names())                                          \
                if (f(t.second.c_str(), a) == 0)                             \
                    return t.first;                                          \
            return UNDEFINED;                                                \
        }                                                                    \
                                                                             \
        static ENUM from_string(const std::string& a, bool a_nocase=false) { \
            return from_string(a.c_str(), a_nocase);                         \
        }                                                                    \
                                                                             \
        inline friend std::ostream& operator<< (std::ostream& out, ENUM a) { \
            return out << ENUM::to_string(a);                                \
        }                                                                    \
                                                                             \
        static constexpr size_t size()  { return s_size-1; }                 \
                                                                             \
        template <typename Visitor>                                          \
        static void for_each(const Visitor& a_fun) {                         \
            for (auto it = ++names().begin(), e = names().end(); it!=e; ++it)\
                if (!a_fun(it->first))                                       \
                    break;                                                   \
        }                                                                    \
    }

// Internal macro for supporting BOOST_PP_SEQ_TRANSFORM
#define UTXX_INTERNAL_ENUM_VAL(x, _, val)                                    \
    BOOST_PP_TUPLE_ELEM(0, val)                                              \
    BOOST_PP_IF(                                                             \
        BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(val), 1),                       \
        = BOOST_PP_TUPLE_ELEM(1, val),                                       \
        BOOST_PP_EMPTY())
#define UTXX_INTERNAL_ENUM_PAIR(x, _, val)                                   \
    {std::make_pair(BOOST_PP_TUPLE_ELEM(0, val),                             \
                    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, val)))}
