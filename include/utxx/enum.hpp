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
#include <boost/preprocessor/tuple/enum.hpp>
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
/// UTXX_ENUM(OpT, char,          ...)  // Adds UNDEFINED=0  default item
/// UTXX_ENUM(OpT, (char, -1),    ...)  // Adds UNDEFINED=-1 default item
/// UTXX_ENUM(OpT, (char,NIL,-1), ...)  // Adds NIL=0        default item
///
/// UTXX_ENUM(MyEnumT, char,       // Automatically gets UNDEFINED=0 item added
///             Apple,
///             Pear,
///             Grape
///          );
/// ```
//------------------------------------------------------------------------------
#define UTXX_ENUM(ENUM, TYPE, ...)                                             \
        UTXX_ENUM2(                                                            \
            ENUM,                                                              \
            UTXX_ENUM_GET_TYPE(TYPE),                                          \
            UTXX_ENUM_GET_UNDEF_NAME(TYPE),                                    \
            UTXX_ENUM_GET_UNDEF_VAL(TYPE),                                     \
            BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)                              \
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
///    (char,           // This is enum storage type
///     UNDEFINED,      // The "name" of the first (i.e. "undefined") item
///     -1,             // "initial" enum value for "UNDEFINED" value
///    )
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
#define UTXX_ENUMZ(ENUM, TYPE, ...)                                            \
        UTXX_ENUM2(                                                            \
            ENUM,                                                              \
            UTXX_ENUM_GET_TYPE(TYPE),                                          \
            UTXX_ENUM_GET_UNDEF_NAME(TYPE),                                    \
            UTXX_ENUM_GET_UNDEF_VAL(TYPE),                                     \
            __VA_ARGS__                                                        \
        )

#define UTXX_ENUM2(ENUM, TYPE, UNDEFINED, INIT, ...)                           \
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
                std::make_pair(BOOST_PP_STRINGIZE(UNDEFINED),                  \
                               BOOST_PP_STRINGIZE(UNDEFINED)),                 \
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

// Internal macro for supporting BOOST_PP_SEQ_TRANSFORM
#define UTXX_INTERNAL_ENUMI_VAL(x, _, val)       BOOST_PP_TUPLE_ELEM(0, val)

#define UTXX_INTERNAL_ENUMI_PAIR(x, _, val)                                    \
    std::make_pair( BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, val)),           \
                    BOOST_PP_IIF(                                              \
                        BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(val), 1),         \
                        BOOST_PP_TUPLE_ELEM(1, val),                           \
                        BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, val))) )

// Undefined option may be in one of three forms:
//      char                // Enum Type
//      (char, 0)           // Enum Type, Default Value
//      (char, Undef, 0)    // Enum Type, UndefinedItemName, Default Value
//
// The following three macros get the {Type, Name, Value} accordingly:
#define UTXX_ENUM_GET_TYPE(type)       UTXX_ENUM_UNDEF_1__(0, type)
#define UTXX_ENUM_GET_UNDEF_NAME(type) UTXX_ENUM_UNDEF_1__(1, type)
#define UTXX_ENUM_GET_UNDEF_VAL(type)  UTXX_ENUM_UNDEF_1__(2, type)

#define UTXX_ENUM_UNDEF_ARG__(N, arg)                                          \
    BOOST_PP_IIF(                                                              \
        BOOST_PP_IS_BEGIN_PARENS(arg),                                         \
        BOOST_PP_IIF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(arg),2),             \
            BOOST_PP_TUPLE_ELEM(N, arg),                                       \
            BOOST_PP_TUPLE_ELEM(N, (BOOST_PP_TUPLE_ELEM(0, arg),               \
                                    UNDEFINED,                                 \
                                    BOOST_PP_TUPLE_ELEM(1, arg)))),            \
        BOOST_PP_TUPLE_ELEM(N, (arg, UNDEFINED, 0)))

#define UTXX_ENUM_UNDEF_1__(N, arg)                                            \
    BOOST_PP_IIF(                                                              \
        BOOST_PP_IS_BEGIN_PARENS(arg),                                         \
        UTXX_ENUM_UNDEF_2__(N, arg),                                           \
        UTXX_ENUM_UNDEF_2__(N, (arg, UNDEFINED, 0)))

#define UTXX_ENUM_UNDEF_2__(N, arg)                                            \
    BOOST_PP_IIF(BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(arg),2),                 \
        BOOST_PP_TUPLE_ELEM(N, (BOOST_PP_TUPLE_ENUM(arg), _)),                 \
        BOOST_PP_TUPLE_ELEM(N, (BOOST_PP_TUPLE_ELEM(0, arg),                   \
                                UNDEFINED,                                     \
                                BOOST_PP_TUPLE_ELEM(1, arg))))
