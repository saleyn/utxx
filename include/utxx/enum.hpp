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
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/comparison/not_equal.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <utxx/detail/enum_helper.hpp>
#include <utxx/string.hpp>
#include <utxx/config.h>
#include <cassert>

//------------------------------------------------------------------------------
/// Strongly typed reflectable enum declaration
//------------------------------------------------------------------------------
/// This macro allows to define a reflectable enum. It accepts the following:
///
/// `UTXX_ENUM(EnumName, Opts, Enums)`
/// * EnumName - the name of ENUM
/// * Opts - can be one of three formats:
///     * Type                      - Enum of Type. Adds DEF_NAME=0.
///     * (Type,DefValue)           - Enum of Type. Adds DEF_NAME=DefValue.
///     * (Type,UndefName,DefValue) - Enum of Type. Adds UndefName=DefValue.
///     * (Type,UndefName,DefValue,FirstVal) - Ditto. Start enum with FirstVal.
///     * (Type,UndefName,DefValue,FirstVal,Explicit)
///         - Ditto. Boolean Explicit (default=true) enforces explicit
///           conversion from Type.
/// * Enums is either a variadic list or sequence of arguments:
///     * Enum1, Enum2, ...
///     * (Enum1)(Enum2)...
///     where each `EnumN` gets the code of the incremented sequence number
///     assigned, and may optionally specify a symbolic value label:
///         * EnumName
///         * (EnumName,"EnumValue")  - EnumName will have value() = "EnumValue"
///
/// A symbolic value is NOT the member's value, but a description that may be
/// obtained by calling the value() method. The item's actual enum value is the
/// starting value of this enum type plus the position of item in the list.
///
/// The value() of a member defaults to its symbolic name.
/// ```
/// #include <utxx/enum.hpp>
///
/// UTXX_ENUM(Fruits, char,       // Automatically gets DEF_NAME=0 item added
///             Apple,
///             Pear,
///             Grape
///          );
///
/// UTXX_ENUM(Op1, char,          A,B);  // Adds DEF_NAME=0  default item
/// UTXX_ENUM(Op2, (char, -1),    A,B);  // Adds DEF_NAME=-1 default item
/// UTXX_ENUM(Op3, (char,NIL,-1), A,B);  // Adds NIL=0 default item
/// UTXX_ENUM(Op3, (char,NIL,3,0),A,B);  // Adds NIL=3 default item, A=0,B=1
/// UTXX_ENUM(Op4, char,  (A,"a") (B));  // Adds DEF_NAME=0  default item
///
/// UTXX_ENUM(MyEnumT,
///    (char,           // This is enum storage type
///     DEF_NAME,       // The "name" of the first (i.e. "undefined") item
///     -10,            // "DEF_NAME" value
///     0               // Value assigned to the first item (Apple)
///    ),
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
// NOTE: the last "if" statement unifies possible forms of vararg inputs:
//   UTXX_ENUM(XXX, int, A, B, C)       ->  UTXX_ENUMZ(XXX, int, (A)(B)(C))
//   UTXX_ENUM(XXX, int, (A,"a"), B, C) ->  UTXX_ENUMZ(XXX, int, (A,"a")(B)(C))
//   UTXX_ENUM(XXX, int, (A) (B) (C))   ->  UTXX_ENUMZ(XXX, int, (A)(B)(C))
//------------------------------------------------------------------------------
#define UTXX_ENUM(ENUM, TYPE, ...)                                             \
        UTXX_ENUMZ(                                                            \
            ENUM,                                                              \
            UTXX_ENUM_GET_TYPE(TYPE),                                          \
            UTXX_ENUM_GET_UNDEF_NAME(TYPE),                                    \
            UTXX_ENUM_GET_UNDEF_VAL(TYPE),                                     \
            UTXX_ENUM_GET_FIRST_VAL(TYPE),                                     \
            UTXX_ENUM_GET_EXPLICIT(TYPE),                                      \
            BOOST_PP_TUPLE_ENUM(                                               \
                BOOST_PP_IF(                                                   \
                    BOOST_PP_EQUAL(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), 1),    \
                        BOOST_PP_IF(BOOST_PP_IS_BEGIN_PARENS(__VA_ARGS__),     \
                            (__VA_ARGS__),                                     \
                            ((__VA_ARGS__))),                                  \
                        (BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))))              \
        )

//------------------------------------------------------------------------------
// For internal use
//------------------------------------------------------------------------------
#define UTXX_ENUMZ(ENUM, TYPE, DEF_NAME, DEF_VAL, FIRST_VAL, EXPLICIT, ...)    \
    struct ENUM {                                                              \
        using value_type = TYPE;                                               \
                                                                               \
        enum type : TYPE {                                                     \
            DEF_NAME = DEF_VAL,                                                \
            _START_  = (TYPE)(FIRST_VAL-1),                                    \
            BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(                          \
                UTXX_ENUM_INTERNAL_GET_0__, _,                                 \
                BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__))),                   \
            _END_                                                              \
        };                                                                     \
                                                                               \
        constexpr ENUM()         noexcept: m_val(DEF_NAME) {                   \
            static_assert(DEF_VAL < FIRST_VAL || DEF_VAL >= int(_END_),        \
                          "Init value must be outside of first and last!");    \
        }                                                                      \
        constexpr ENUM(type v)   noexcept: m_val(v) {}                         \
        BOOST_PP_IF(UTXX_PP_BOOL_TO_BIT(EXPLICIT),                             \
            explicit, BOOST_PP_EMPTY)                                          \
        constexpr ENUM(TYPE v)   noexcept: m_val(type(v))                      \
                                         { assert(v<int(s_size)); }            \
                                                                               \
        ENUM(ENUM&&)                 = default;                                \
        ENUM(ENUM const&)            = default;                                \
                                                                               \
        ENUM& operator=(ENUM const&) = default;                                \
        ENUM& operator=(ENUM&&)      = default;                                \
                                                                               \
        static constexpr const char*   class_name() { return #ENUM; }          \
        constexpr operator type()      const { return m_val; }                 \
        constexpr bool     empty()     const { return m_val == DEF_NAME;   }   \
        void               clear()           { m_val =  DEF_NAME;          }   \
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
        static constexpr bool valid(TYPE v)  { return v == DEF_NAME ||         \
                                                     (v >= begin()  &&         \
                                                      v <  end());    }        \
        static ENUM                                                            \
        from_string(const char* a, bool nocase=false, bool as_name=false)  {   \
            auto f = nocase  ? &strcasecmp : &strcmp;                          \
            for (auto i=begin(); i < end(); i = inc(i)) {                      \
                if(!f((as_name ? meta(i).first : meta(i).second).c_str(),a))   \
                    return i;                                                  \
            }                                                                  \
            return ENUM(DEF_NAME);                                             \
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
        static ENUM from_name(const std::string& a, bool a_nocase=false) {     \
            return from_string(a.c_str(), a_nocase, true);                     \
        }                                                                      \
        static ENUM from_name(const char* a, bool a_nocase=false) {            \
            return from_string(a, a_nocase, true);                             \
        }                                                                      \
        static ENUM from_value(const std::string& a, bool a_nocase=false) {    \
            return from_string(a.c_str(), a_nocase, false);                    \
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
        static constexpr type   begin() { return type(FIRST_VAL); }            \
        static constexpr type   end()   { return _END_; }                      \
        static constexpr type   last()  { return type(_END_-1); }              \
        static constexpr type   inc(type x) { return type(int(x)+1);   }       \
                                                                               \
        template <typename Visitor>                                            \
        static void for_each(const Visitor& a_fun) {                           \
            for (auto i=begin(); i < end(); i = inc(i))                        \
                if (!a_fun(i))                                                 \
                    break;                                                     \
        }                                                                      \
    private:                                                                   \
        static const size_t s_size =                                           \
            1+BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__));    \
        static const std::pair<std::string,std::string>* names() {             \
            static const std::pair<std::string,std::string> s_names[] = {      \
                std::make_pair(BOOST_PP_STRINGIZE(DEF_NAME),                   \
                               BOOST_PP_STRINGIZE(DEF_NAME)),                  \
                BOOST_PP_SEQ_ENUM(                                             \
                    BOOST_PP_SEQ_TRANSFORM(                                    \
                        UTXX_ENUM_INTERNAL_GET_PAIR__, ,                       \
                        BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)              \
                ))                                                             \
            };                                                                 \
            return s_names;                                                    \
        }                                                                      \
                                                                               \
        static const std::pair<std::string,std::string>& meta(TYPE n) {        \
            auto   m = n == DEF_NAME ? 0 : int(n)-(FIRST_VAL)+1;               \
            assert(m >= 0 && m < int(s_size));                                 \
            return names()[m];                                                 \
        }                                                                      \
                                                                               \
        UTXX__ENUM_FRIEND_SERIALIZATION__;                                     \
                                                                               \
        type m_val;                                                            \
    }
