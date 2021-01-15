//----------------------------------------------------------------------------
/// \file   enumv.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief This file defines enum with assignable non-unique values
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

#include <cassert>
#include <map>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/variadic_seq_to_seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/push_back.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <utxx/detail/enum_helper.hpp>
#include <utxx/string.hpp>
#include <utxx/enumu.hpp>

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
/// The difference between enum.hpp and enumv.hpp is that UTXX_ENUMV
/// allows to assign specific non-unique values to the enumerated constants.
///
/// `UTXX_ENUMV(EnumName, Opts, Enums)`
/// * EnumName - the name of ENUM
/// * Opts - can be one of three formats:
///     * Type                      - Enum of Type. Adds DEF_NAME=0.
///     * (Type,DefValue)           - Enum of Type. Adds DEF_NAME=DefValue.
///     * (Type,UndefName,DefValue) - Enum of Type. Adds UndefName=DefValue.
///     * (Type,UndefName,DefValue,FirstVal) - Ditto. Start enum with FirstVal.
///     * (Type,UndefName,DefValue,FirstVal,Explicit)
///         - Ditto. Boolean Explicit (default=true) enforces explicit
///           conversion from Type.
///
/// * Enums is either a variadic list or sequence of arguments:
///     * Enum1, Enum2, ...
///     * (Enum1)(Enum2)...
///     where each `EnumN` can be given a Value associated with each EnumName,
///     and may optionally specify a symbolic value label:
///         * EnumName
///         * (EnumName,Value)              - EnumName's code()  = Value
///         * (EnumName,Value,"EnumValue")  - EnumName's value() = "EnumValue"
//------------------------------------------------------------------------------
/// NOTE: DefValue as well as values given to EnumName's may not necessarily be
///       distinct!
/// NOTE: The name lookups in ENUMV happen by using std::map<ENUM, string>. The
///       reason we can't use a switch statement or array is that assigned enum
///       values can be duplicated, e.g.: ```enum X { A = 1, B = 1, C = 2 }```.
///       In this case we must guarantee that both ENUM::from_name("A") and
///       ENUM::from_name("B") resolve to value ENUM::A, and ENUM::B, which
///       happen to be the same value. So if we used a switch statement, that
///       would result in compiler error:
///       ```
///          switch(enum_value) {
///             case ENUM::A: return "A";
///             case ENUM::B: return "B";
///             ...
///          }
///       ```
///
/// Enum declaration:
/// ```
/// #include <utxx/enumv.hpp>
///
/// UTXX_ENUMV(MyEnumT,
///    (                    // This parameter is a tuple of up to four elements:
///     char,               //   This is enum storage type
///     NIL,                //   Name of the undefined type
///     ' ',                //   NIL's value
///     'a'                 //   Value assigned to the first item (Orange)
///    ),
///    (Orange)             // Orange's value will be set to 'a'
///    (Apple, 'x', "Fuji") // Item with a value and with name string "Fuji"
///    (Pear,  'y')         // Item with value 'y' (name defaults to "Pear")
///    (Fig,   'y')         // NOTE: duplicate value won't cause compile error!
///    (Grape)              // Grape's value will be equal to 'y'+1
/// );
///
/// Sample usage:
/// MyEnumT val = MyEnumT::from_string("Pear");
/// std::cout << "Value: " << to_string(val) << std::endl;
/// std::cout << "Value: " << val            << std::endl;
//------------------------------------------------------------------------------
// NOTE: the last "if" statement unifies possible forms of vararg inputs:
//   UTXX_ENUM(XXX, int, A, B, C)       ->  UTXX_ENUMZ(XXX, int, (A)(B)(C))
//   UTXX_ENUM(XXX, int, (A) (B) (C))   ->  UTXX_ENUMZ(XXX, int, (A)(B)(C))
//   UTXX_ENUM(XXX, int, (A,"a"), B, C) ->  UTXX_ENUMZ(XXX, int, (A,"a")(B)(C))
//   UTXX_ENUM(XXX, int, (A,3,"a"), B)  ->  UTXX_ENUMZ(XXX, int, (A,3,"a")(B))
//------------------------------------------------------------------------------
#define UTXX_ENUMV(ENUM, TYPE, ...)                                            \
        UTXX_ENUMV__(                                                          \
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

#define UTXX_ENUMV__(ENUM, TYPE, DEF_NAME, DEF_VAL, FIRST_VAL, EXPLICIT, ...)  \
    struct ENUM {                                                              \
        using value_type = TYPE;                                               \
                                                                               \
        enum type : TYPE {                                                     \
            DEF_NAME = DEF_VAL,                                                \
            _START_  = DEF_VAL-1,                                              \
            BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(                          \
                UTXX_ENUM_INTERNAL_GET_NAMEVAL__, _,                           \
                BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)))                    \
        };                                                                     \
                                                                               \
        constexpr ENUM()       noexcept : m_val(DEF_NAME)  {}                  \
        constexpr ENUM(type v) noexcept : m_val(v)         {}                  \
        BOOST_PP_IF(UTXX_PP_BOOL_TO_BIT(EXPLICIT),                             \
            explicit, /**/)                                                    \
        constexpr ENUM(TYPE v) noexcept : m_val(type(v))   {}                  \
        BOOST_PP_IF(UTXX_PP_BOOL_TO_BIT(EXPLICIT),                             \
            explicit, /**/)                                                    \
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
        const std::string& name()    const { return meta(m_val).first;     }   \
        const std::string& value()   const { return meta(m_val).second;    }   \
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
                if (!f((as_name ? t.second.first : t.second.second).c_str(),a))\
                    return ENUM(t.first);                                      \
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
            for (auto it = ++metas().begin(), e = metas().end(); it!=e; ++it)  \
                if (!a_fun(it->first, it->second))                             \
                    break;                                                     \
        }                                                                      \
                                                                               \
    private:                                                                   \
        static const std::pair<const type,std::pair<std::string,std::string>>& \
        null_pair() {                                                          \
            static const                                                       \
            std::pair<const type, std::pair<std::string,std::string>> s_val =  \
                std::make_pair                                                 \
                    (DEF_NAME, std::make_pair(BOOST_PP_STRINGIZE(DEF_NAME),    \
                                              BOOST_PP_STRINGIZE(DEF_NAME)));  \
            return s_val;                                                      \
        }                                                                      \
        static const size_t s_size =                                           \
            1+BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__));    \
        static const std::map<type, std::pair<std::string,std::string>>&       \
        metas() {                                                              \
            static const std::map<type, std::pair<std::string,std::string>>    \
            s_metas{{                                                          \
                null_pair(),                                                   \
                BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(                      \
                    UTXX_ENUM_INTERNAL_GET_PAIR_AND_PAIR__, _,                 \
                    BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)))                \
            }};                                                                \
            return s_metas;                                                    \
        }                                                                      \
                                                                               \
        static const std::pair<std::string,std::string>& meta(type n) {        \
            auto it = metas().find(n);                                         \
            return (it == metas().end() ? null_pair() : *it).second;           \
        }                                                                      \
                                                                               \
        UTXX__ENUM_FRIEND_SERIALIZATION__;                                     \
                                                                               \
        type m_val;                                                            \
    }

#define UTXX_ENUM_INTERNAL_GET_PAIR_AND_PAIR__(x, _, val)                      \
    std::make_pair(                                                            \
        BOOST_PP_TUPLE_ELEM(0, val),                                           \
        std::make_pair(                                                        \
            BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, val)),                   \
            BOOST_PP_IIF(                                                      \
                BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(val), 2),                 \
                BOOST_PP_TUPLE_ELEM(2, BOOST_PP_TUPLE_PUSH_BACK(val,_)),       \
                BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, val)))))

// Internal macros for supporting BOOST_PP_SEQ_TRANSFORM
#define UTXX_ENUM_INTERNAL_GET_PAIR__(x, _, val)                               \
    std::make_pair( BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, val)),           \
                    BOOST_PP_IIF(                                              \
                        BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(val), 1),         \
                        BOOST_PP_TUPLE_ELEM(1, val),                           \
                        BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, val))) )
