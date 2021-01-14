//----------------------------------------------------------------------------
/// \file   enumu.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief This file defines enum with assignable unique values
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
#include <boost/preprocessor/tuple/push_back.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <utxx/detail/enum_helper.hpp>
#include <utxx/string.hpp>
#include <cassert>
#include <map>

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
/// The difference between enumu.hpp and enumv.hpp is that UTXX_ENUMU
/// allows the enum to have specific enumerated unique constants, whereas
/// UTXX_ENUMV permits the use of non-unique constants. The enum name lookup by
/// value is faster for UTXX_ENUMU, since it uses a play array with lookup by
/// index, whereas UTXX_ENUMV uses a map, so ```enum X { A=1, B=2 }``` is
/// equivalent to ```UTXX_ENUMU(X, int, (A, 1)(B, 2))```, and name lookups are
/// implemented using:
///       ```
///          switch(val) {
///             case 1: return "A";
///             case 2: return "B";
///             ...
///          }
///       ```
/// `UTXX_ENUMU(EnumName, Opts, Enums)`
/// * EnumName - the name of ENUM
/// * Opts - can be one of three formats:
///     * Type                      - Enum of Type. Adds DEF_NAME=0.
///     * (Type,DefValue)           - Enum of Type. Adds DEF_NAME=DefValue.
///     * (Type,UndefName,DefValue) - Enum of Type. Adds UndefName=DefValue.
///     * (Type,UndefName,DefValue,FirstVal) - Ditto. Start enum with FirstVal.
/// * Enums is either a variadic list or sequence of arguments:
///     * Enum1, Enum2, ...
///     * (Enum1)(Enum2)...
///     where each `EnumN` can be given a Value associated with each EnumName,
///     and may optionally specify a symbolic value label:
///         * EnumName
///         * (EnumName,Value)              - EnumName's code()  = Value
///         * (EnumName,Value,"EnumValue")  - EnumName's value() = "EnumValue"
//------------------------------------------------------------------------------
/// NOTE: DefValue as well as values given to EnumName's must be distinct!
//------------------------------------------------------------------------------
/// Enum declaration:
/// ```
/// #include <utxx/enumu.hpp>
///
/// UTXX_ENUMU(MyEnumT,
///    (                    // This parameter is a tuple of up to four elements:
///     char,               //   This is enum storage type
///     NIL,                //   Name of the undefined type
///     ' ',                //   NIL's value
///     'a'                 //   Value assigned to the first item (Orange)
///    ),
///    (Orange)             // Orange's value will be set to 'a'
///    (Apple, 'x', "Fuji") // Item with a value and with name string "Fuji"
///    (Pear,  'y')         // Item with value 'y' (name defaults to "Pear")
/// // (Fig,   'y')         // NOTE: this would result in compile-time error!
///    (Grape)              // Grape's value will be equal to 'y'+1
/// );
///
/// Sample usage:
/// MyEnumT val = MyEnumT::from_string("Pear");
/// std::cout << "Value: " << to_string(val) << std::endl;
/// std::cout << "Value: " << val            << std::endl;
//------------------------------------------------------------------------------
#define UTXX_ENUMU(ENUM, TYPE, ...)                                            \
        UTXX_ENUMU__(                                                          \
            ENUM,                                                              \
            UTXX_ENUM_GET_TYPE(TYPE),                                          \
            UTXX_ENUM_GET_UNDEF_NAME(TYPE),                                    \
            UTXX_ENUM_GET_UNDEF_VAL(TYPE),                                     \
            UTXX_ENUM_GET_FIRST_VAL(TYPE),                                     \
            BOOST_PP_TUPLE_ENUM(                                               \
                BOOST_PP_IF(                                                   \
                    BOOST_PP_EQUAL(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), 1),    \
                        BOOST_PP_IF(BOOST_PP_IS_BEGIN_PARENS(__VA_ARGS__),     \
                            (__VA_ARGS__),                                     \
                            ((__VA_ARGS__))),                                  \
                        (BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))))              \
        )

#define UTXX_ENUMU__(ENUM, TYPE, DEF_NAME, DEF_VAL, FIRST_VAL, ...)            \
    struct ENUM {                                                              \
        using value_type = TYPE;                                               \
                                                                               \
        enum type : TYPE {                                                     \
            DEF_NAME = (DEF_VAL),                                              \
            _START_  = FIRST_VAL-1,                                            \
            BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(                          \
                UTXX_ENUM_INTERNAL_GET_NAMEVAL__, _,                           \
                BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)))                    \
        };                                                                     \
                                                                               \
        using meta_type  = std::tuple<type, std::string, std::string>;         \
                                                                               \
        explicit  ENUM(long v) noexcept : m_val(type(v))   {}                  \
        constexpr ENUM()       noexcept : m_val(DEF_NAME)  {}                  \
        constexpr ENUM(type v) noexcept : m_val(v)         {}                  \
                                                                               \
        ENUM(ENUM&&)                 = default;                                \
        ENUM(ENUM const&)            = default;                                \
                                                                               \
        ENUM& operator=(ENUM const&) = default;                                \
        ENUM& operator=(ENUM&&)      = default;                                \
                                                                               \
        static constexpr const char*   class_name() { return #ENUM;        }   \
        constexpr operator  type()     const { return m_val; }                 \
        constexpr bool      empty()    const { return m_val == DEF_NAME;   }   \
        void                clear()          { m_val =  DEF_NAME;          }   \
                                                                               \
        static    constexpr bool is_enum()   { return true;                }   \
        static    constexpr bool is_flags()  { return false;               }   \
                                                                               \
        const std::string& name()    const { return std::get<1>(meta(m_val));} \
        const std::string& value()   const { return std::get<2>(meta(m_val));} \
        TYPE               code()    const { return TYPE(m_val);           }   \
                                                                               \
        const std::string& to_string() const { return value();             }   \
        static const std::string&                                              \
                           to_string(type a) { return ENUM(a).to_string(); }   \
        const char*        c_str()     const { return to_string().c_str(); }   \
        static const char* c_str(type a)     { return to_string(a).c_str();}   \
                                                                               \
        /* Returns true if the given value is a valid value for this enum */   \
        static constexpr bool valid(TYPE v)  {                                 \
            if (v == DEF_NAME) return true;                                    \
            bool found = false;                                                \
            auto f = [&found, v](type t, auto&) {                              \
                if (t == v) { found = true; return false; }                    \
                return true;                                                   \
            };                                                                 \
            for_each(f);                                                       \
            return found;                                                      \
        }                                                                      \
                                                                               \
        static ENUM                                                            \
        from_string(const char* a, bool a_nocase=false, bool as_name=false)  { \
            auto f = a_nocase ? &strcasecmp : &strcmp;                         \
            for (auto& t : metas())                                            \
                if (!f((as_name ? std::get<1>(t) : std::get<2>(t)).c_str(),a)) \
                    return ENUM(std::get<0>(t));                               \
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
                                                                               \
        template <typename Visitor>                                            \
        static void for_each(const Visitor& a_fun) {                           \
            int i=0;                                                           \
            for (auto& m : metas())                                            \
                if (i++ && !a_fun(std::get<0>(m), m))                          \
                    break;                                                     \
        }                                                                      \
                                                                               \
    private:                                                                   \
        static const meta_type& null_meta() {                                  \
            static const meta_type s_val =                                     \
                std::make_tuple(DEF_NAME, BOOST_PP_STRINGIZE(DEF_NAME),        \
                                          BOOST_PP_STRINGIZE(DEF_NAME));       \
            return s_val;                                                      \
        }                                                                      \
        static const size_t s_size =                                           \
            1+BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__));    \
        static const std::array<meta_type, s_size>& metas() {                  \
            static const std::array<meta_type, s_size> s_metas{{               \
                null_meta(),                                                   \
                BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(                      \
                    UTXX_INTERNAL_ENUMU_META_TUPLE__, _,                       \
                    BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)))                \
            }};                                                                \
            return s_metas;                                                    \
        }                                                                      \
                                                                               \
        static const meta_type& meta(type n) {                                 \
            switch (n) {                                                       \
                case DEF_NAME: return metas()[0];                              \
                BOOST_PP_SEQ_FOR_EACH_I_R(1,                                   \
                    UTXX_INTERNAL_ENUMU_CASE__, _,                             \
                    BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__))                 \
                default:        return metas()[0];                             \
            }                                                                  \
        }                                                                      \
                                                                               \
        UTXX__ENUM_FRIEND_SERIALIZATION__;                                     \
                                                                               \
        type m_val;                                                            \
    }

#define UTXX_INTERNAL_ENUMU_META_TUPLE__(x, _, val)                            \
    std::make_tuple(                                                           \
        BOOST_PP_TUPLE_ELEM(0, val),                                           \
        BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, val)),                       \
        BOOST_PP_IIF(                                                          \
            BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(val), 2),                     \
            BOOST_PP_TUPLE_ELEM(2, BOOST_PP_TUPLE_PUSH_BACK(val,_)),           \
            BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, val))))

#define UTXX_INTERNAL_ENUMU_CASE__(x, _, i, item)                              \
            case BOOST_PP_TUPLE_ELEM(0, item): return metas()[BOOST_PP_INC(i)];
