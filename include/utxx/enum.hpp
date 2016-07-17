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
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/variadic_seq_to_seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <utxx/string.hpp>
#include <utxx/config.h>
#include <cassert>

#ifdef UTXX_ENUM_SUPPORT_SERIALIZATION
# ifndef UTXX__ENUM_FRIEND_SERIALIZATION__
#   include <boost/serialization/access.hpp>

#   define UTXX__ENUM_FRIEND_SERIALIZATION__ \
    friend class boost::serialization::access
# endif
#else
# ifndef UTXX__ENUM_FRIEND_SERIALIZATION__
#   define UTXX__ENUM_FRIEND_SERIALIZATION__
# endif
#endif

//------------------------------------------------------------------------------
/// Strongly typed reflectable enum declaration
//------------------------------------------------------------------------------
/// \copydoc #UTXX_ENUMZ().
/// ```
/// #include <utxx/enum.hpp>
///
/// UTXX_ENUM(MyEnumT, char,        // Automatically gets UNDEFINED=0 item added
///             Apple,
///             Pear,
///             Grape
///          );
/// ```
//------------------------------------------------------------------------------
#define UTXX_ENUM(ENUM, TYPE, ...)                                             \
        UTXX_ENUMZ(                                                            \
            ENUM, TYPE, UNDEFINED, 0, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)    \
        )

//------------------------------------------------------------------------------
/// Strongly typed reflectable enum declaration
/// This macro allows to customize the name of the "UNDEFINED" element, the
/// initial enum value, as well as optionally give descriptions to enumerated
/// items.
///
/// Note that an item of the enum sequence may optionally have symbolic values.
///
/// A symbolic value is NOT the item's value, but a description that may be
/// obtained by calling the value() method. The item's actual enum value is the
/// starting value of this enum type plus the position of item in the list.
///
/// The value() of an item defaults to its symbolic name.
//------------------------------------------------------------------------------
/// Sample usage:
/// ```
/// #include <utxx/enum.hpp>
///
/// UTXX_ENUMZ(MyEnumT,
///    char,           // This is enum storage type
///    UNDEFINED,      // The "name" of the first (i.e. "undefined") item
///    -1,             // "initial" enum value for "UNDEFINED" value
///
///    (Apple, "Gala") // An item can optionally have an associated string value
///    (Pear)          // String value defaults to item's name (i.e. "Pear")
///    (Grape, "Fuji")
/// );
///
/// MyEnumT v = MyEnumT::from_name("Apple");
/// std::cout << "Value: " << v.name()      << std::endl;    // Out: Apple
/// std::cout << "Value: " << v.value()     << std::endl;    // Out: Gala
/// std::cout << "Value: " << v.code()      << std::endl;    // Out: 0
/// std::cout << "Value: " << int(v)        << std::endl;    // Out: 0
/// std::cout << "Value: " << v.to_string() << std::endl;    // Out: Gala
/// std::cout << "Value: " << v             << std::endl;    // Out: Gala
/// v = MyEnumT::from_string("Fuji");
/// std::cout << "Value: " << v.name()      << std::endl;    // Out: Grape
/// std::cout << "Value: " << v.value()     << std::endl;    // Out: Fuji
/// ```
//------------------------------------------------------------------------------
#define UTXX_ENUMZ(ENUM, TYPE, UNDEFINED, INIT, ...)                           \
    struct ENUM {                                                              \
        using value_type = TYPE;                                               \
                                                                               \
        enum type : TYPE {                                                     \
            UNDEFINED = (INIT),                                                \
            BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(                          \
                UTXX_INTERNAL_ENUMI_VAL, _,                                    \
                BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__))),                   \
            _END_                                                              \
        };                                                                     \
                                                                               \
        explicit  ENUM(TYPE v)   noexcept: m_val(type(v))                      \
                                         { assert(v<int(s_size)); }            \
        constexpr ENUM()         noexcept: m_val(UNDEFINED) {}                 \
        constexpr ENUM(type v)   noexcept: m_val(v) {}                         \
                                                                               \
        ENUM(ENUM&&)                 = default;                                \
        ENUM(ENUM const&)            = default;                                \
                                                                               \
        ENUM& operator=(ENUM const&) = default;                                \
        ENUM& operator=(ENUM&&)      = default;                                \
                                                                               \
        static constexpr const char*   class_name() { return #ENUM; }          \
        constexpr operator type()      const { return m_val; }                 \
        constexpr bool     empty()     const { return m_val == UNDEFINED;  }   \
        void               clear()           { m_val =  UNDEFINED;         }   \
                                                                               \
        static constexpr   bool is_enum()    { return true;                }   \
        static constexpr   bool is_flags()   { return false;               }   \
                                                                               \
        const std::string& name()    const { return meta(TYPE(m_val)).first; } \
        const std::string& value()   const { return meta(TYPE(m_val)).second;} \
        TYPE               code()    const { return TYPE(m_val);             } \
                                                                               \
        const std::string& to_string() const { return value();             }   \
        static const       std::string&                                        \
                           to_string(type a) { return ENUM(a).to_string(); }   \
        const char*        c_str()     const { return to_string().c_str(); }   \
        static const char* c_str(type a)     { return to_string(a).c_str();}   \
                                                                               \
        static ENUM                                                            \
        from_string(const char* a, bool nocase=false, bool as_name=false)  {   \
            auto f = nocase  ? &strcasecmp : &strcmp;                          \
            for (TYPE i=(INIT)+1; i != s_size; i++)                            \
                if(!f((as_name ? meta(i).second : meta(i).first).c_str(),a))   \
                    return ENUM(i);                                            \
            return ENUM(UNDEFINED);                                            \
        }                                                                      \
                                                                               \
        static ENUM from_string(const std::string& a, bool a_nocase=false,     \
                                bool as_name=false)                            \
            { return from_string(a.c_str(), a_nocase, as_name); }              \
                                                                               \
        static ENUM from_string_nc(const std::string& a, bool as_name=false) { \
            return from_string(a, true, as_name);                              \
        }                                                                      \
        static ENUM from_string_nc(const char* a, bool as_name=false) {        \
            return from_string(a, true, as_name);                              \
        }                                                                      \
                                                                               \
        static ENUM from_name(const char* a, bool a_nocase=false) {            \
            return from_string(a, a_nocase, true);                             \
        }                                                                      \
        static ENUM from_value(const char* a, bool a_nocase=false) {           \
            return from_string(a, a_nocase, false);                            \
        }                                                                      \
                                                                               \
        template <typename StreamT>                                            \
        inline friend StreamT& operator<< (StreamT& out, ENUM const& a) {      \
            out << a.value();                                                  \
            return out;                                                        \
        }                                                                      \
                                                                               \
        template <typename StreamT>                                            \
        inline friend StreamT& operator<< (StreamT& out, ENUM::type a)  {      \
            return out << ENUM(a);                                             \
        }                                                                      \
                                                                               \
        static constexpr size_t size()  { return s_size-1; }                   \
        static constexpr ENUM   begin() { return type(INIT+1); }               \
        static constexpr ENUM   end()   { return _END_; }                      \
        static constexpr ENUM   last()  { return type(INIT+size()); }          \
                                                                               \
        template <typename Visitor>                                            \
        static void for_each(const Visitor& a_fun) {                           \
            for (size_t i=1; i < s_size; i++)                                  \
                if (!a_fun(type((INIT)+i)))                                    \
                    break;                                                     \
        }                                                                      \
    private:                                                                   \
        static const size_t s_size =                                           \
            1+BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__));    \
        static const std::pair<std::string,std::string>* names() {             \
            static const std::pair<std::string,std::string> s_names[] = {      \
                std::make_pair(#UNDEFINED, #UNDEFINED),                        \
                BOOST_PP_SEQ_ENUM(                                             \
                    BOOST_PP_SEQ_TRANSFORM(                                    \
                        UTXX_INTERNAL_ENUMI_PAIR, ,                            \
                        BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)              \
                ))                                                             \
            };                                                                 \
            return s_names;                                                    \
        }                                                                      \
                                                                               \
        static const std::pair<std::string,std::string>& meta(TYPE n) {        \
            auto   m = n-(INIT);                                               \
            assert(m >= 0 && m < TYPE(s_size));                                \
            return names()[m];                                                 \
        }                                                                      \
                                                                               \
        UTXX__ENUM_FRIEND_SERIALIZATION__;                                     \
                                                                               \
        type m_val;                                                            \
    }

// DEPRECATED!!!
//
/// Untyped reflectable enum declaration
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
    struct [[deprecated]] ENUM {                                            \
        enum etype {                                                        \
            UNDEFINED,                                                      \
            __VA_ARGS__,                                                    \
            _END_                                                           \
        };                                                                  \
                                                                            \
    private:                                                                \
        static const size_t s_size = 1+BOOST_PP_VARIADIC_SIZE(__VA_ARGS__); \
        static const std::string* names() {                                 \
            static const std::string s_names[] = {                          \
                "UNDEFINED",                                                \
                BOOST_PP_SEQ_ENUM(                                          \
                    BOOST_PP_SEQ_TRANSFORM(                                 \
                        UTXX_INTERNAL_ENUM_STRINGIFY, ,                     \
                        BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)               \
                ))                                                          \
            };                                                              \
            return s_names;                                                 \
        }                                                                   \
                                                                            \
        static const std::string& name(size_t n) {                          \
            assert(n < s_size);                                             \
            return names()[n];                                              \
        }                                                                   \
                                                                            \
        UTXX__ENUM_FRIEND_SERIALIZATION__;                                  \
                                                                            \
        etype  m_val;                                                       \
                                                                            \
    public:                                                                 \
        explicit  ENUM(size_t v) : m_val(etype(v)) { assert(v < s_size); }  \
        constexpr ENUM()         : m_val(UNDEFINED) {}                      \
        constexpr ENUM(etype v)  : m_val(v) {}                              \
                                                                            \
        ENUM(ENUM&&)                 = default;                             \
        ENUM(ENUM const&)            = default;                             \
                                                                            \
        ENUM& operator=(ENUM const&) = default;                             \
        ENUM& operator=(ENUM&&)      = default;                             \
                                                                            \
        constexpr operator etype()     const { return m_val; }              \
        constexpr bool     empty()     const { return m_val == UNDEFINED; } \
        const std::string& to_string() const { return name(size_t(m_val));} \
        static const std::string&                                           \
                           to_string(etype a){ return ENUM(a).to_string();} \
        const char*        c_str()     const { return to_string().c_str(); }\
        static const char* c_str(etype a)    { return to_string(a).c_str();}\
                                                                            \
        static ENUM from_string(const std::string& a, bool a_nocase=false){ \
            auto f = a_nocase ? &strcasecmp : &strcmp;                      \
            for (size_t i=1; i != s_size; i++)                              \
                if (f((names()[i]).c_str(), a.c_str()) == 0) return ENUM(i);\
            return UNDEFINED;                                               \
        }                                                                   \
                                                                            \
        static ENUM from_string(const char* a, bool a_nocase=false){        \
            auto f = a_nocase ? &strcasecmp : &strcmp;                      \
            for (size_t i=1; i != s_size; i++)                              \
                if (f((names()[i]).c_str(), a) == 0) return ENUM(i);        \
            return UNDEFINED;                                               \
        }                                                                   \
                                                                            \
        inline friend std::ostream& operator<< (std::ostream& out, ENUM a) {\
            return out << ENUM::to_string(a);                               \
        }                                                                   \
                                                                            \
        static constexpr size_t size()  { return s_size-1; }                \
        static constexpr ENUM   begin() { return etype(1); }                \
        static constexpr ENUM   end()   { return _END_; }                   \
        static constexpr ENUM   last()  { return etype(size()); }           \
                                                                            \
        template <typename Visitor>                                         \
        static void for_each(const Visitor& a_fun) {                        \
            for (size_t i=1; i < s_size; i++)                               \
                if (!a_fun(etype(i)))                                       \
                    break;                                                  \
        }                                                                   \
    }

// Internal macro for supporting BOOST_PP_SEQ_TRANSFORM
#define UTXX_INTERNAL_ENUMI_VAL(x, _, val)       BOOST_PP_TUPLE_ELEM(0, val)

#define UTXX_INTERNAL_ENUM_STRINGIFY(x, _, item) BOOST_PP_STRINGIZE(item)

#define UTXX_INTERNAL_ENUMI_PAIR(x, _, val)                                    \
    std::make_pair( BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, val)),           \
                    BOOST_PP_IIF(                                              \
                        BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(val), 1),         \
                        BOOST_PP_TUPLE_ELEM(1, val),                           \
                        BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, val))) )
