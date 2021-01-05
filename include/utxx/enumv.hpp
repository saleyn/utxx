//----------------------------------------------------------------------------
/// \file   enumv.hpp
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
/// The difference between enum.hpp and enumv.hpp is that UTXX_ENUMV
/// allows to assign specific values to the enumerated constants.
//------------------------------------------------------------------------------
/// NOTE: Make sure that UndefValue is distinct from other values in this enum!
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
///    (char,               // This is enum storage type
///     ' '),               // This is an "UNDEFINED" value
///    (Apple, 'x', "Fuji") // Item with a value and with name string "Fuji"
///    (Pear,  'y')         // Item with value 'y' (name defaults to "Pear")
///    (Grape)              // Grape's value will be equal to 'y'+1
/// );
///
/// Sample usage:
/// MyEnumT val = MyEnumT::from_string("Pear");
/// std::cout << "Value: " << to_string(val) << std::endl;
/// std::cout << "Value: " << val            << std::endl;
//------------------------------------------------------------------------------
#define UTXX_ENUMV(ENUM, TYPE, ...)                                            \
        UTXX_ENUMV__(                                                          \
            ENUM,                                                              \
            UTXX_ENUM_GET_TYPE(TYPE),                                          \
            UTXX_ENUM_GET_UNDEF_NAME(TYPE),                                    \
            UTXX_ENUM_GET_UNDEF_VAL(TYPE),                                     \
            __VA_ARGS__)

#define UTXX_ENUMV__(ENUM, TYPE, UNDEFINED, UndefValue, ...)                   \
    struct ENUM {                                                              \
        using value_type = TYPE;                                               \
                                                                               \
        enum type : TYPE {                                                     \
            UNDEFINED = (UndefValue),                                          \
            BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(                          \
                UTXX_ENUM_INTERNAL_GET_NAMEVAL__, _,                           \
                BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)))                    \
        };                                                                     \
                                                                               \
        using meta_type  = std::tuple<type, std::string, std::string>;         \
                                                                               \
        explicit  ENUM(long v) noexcept : m_val(type(v))   {}                  \
        constexpr ENUM()       noexcept : m_val(UNDEFINED) {}                  \
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
        constexpr bool      empty()    const { return m_val == UNDEFINED;  }   \
        void                clear()          { m_val =  UNDEFINED;         }   \
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
        static ENUM                                                            \
        from_string(const char* a, bool a_nocase=false, bool as_name=false)  { \
            auto f = a_nocase ? &strcasecmp : &strcmp;                         \
            for (auto& t : metas())                                            \
                if (!f((as_name ? std::get<1>(t) : std::get<2>(t)).c_str(),a)) \
                    return ENUM(std::get<0>(t));                               \
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
                                                                               \
        template <typename Visitor>                                            \
        static void for_each(const Visitor& a_fun) {                           \
            int i=0;                                                           \
            for (auto& m : metas())                                            \
                if (i++ && !a_fun(m))                                          \
                    break;                                                     \
        }                                                                      \
                                                                               \
    private:                                                                   \
        static const meta_type& null_meta() {                                  \
            static const meta_type s_val =                                     \
                std::make_tuple(UNDEFINED, "UNDEFINED", "UNDEFINED");          \
            return s_val;                                                      \
        }                                                                      \
        static const size_t s_size =                                           \
            1+BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__));    \
        static const std::array<meta_type, s_size>& metas() {                  \
            static const std::array<meta_type, s_size> s_metas{{               \
                null_meta(),                                                   \
                BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(                      \
                    UTXX_INTERNAL_ENUMV_META_TUPLE__, _,                       \
                    BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)))                \
            }};                                                                \
            return s_metas;                                                    \
        }                                                                      \
                                                                               \
        static const meta_type& meta(type n) {                                 \
            switch (n) {                                                       \
                case UNDEFINED: return metas()[0];                             \
                BOOST_PP_SEQ_FOR_EACH_I_R(1,                                   \
                    UTXX_INTERNAL_ENUMV_CASE__, _,                             \
                    BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__))                 \
                default:        return metas()[0];                             \
            }                                                                  \
        }                                                                      \
                                                                               \
        UTXX__ENUM_FRIEND_SERIALIZATION__;                                     \
                                                                               \
        type m_val;                                                            \
    }

// Internal macro for supporting BOOST_PP_SEQ_TRANSFORM
#define UTXX_ENUM_INTERNAL_GET_NAMEVAL__(x, _, val)                            \
    BOOST_PP_TUPLE_ELEM(0, val)                                                \
    BOOST_PP_IF(                                                               \
        BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(val), 1),                         \
        = BOOST_PP_TUPLE_ELEM(1, val),                                         \
        BOOST_PP_EMPTY())

#define UTXX_INTERNAL_ENUMV_META_TUPLE__(x, _, val)                            \
    std::make_tuple(                                                           \
        BOOST_PP_TUPLE_ELEM(0, val),                                           \
        BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, val)),                       \
        BOOST_PP_IIF(                                                          \
            BOOST_PP_GREATER(BOOST_PP_TUPLE_SIZE(val), 2),                     \
            BOOST_PP_TUPLE_ELEM(2, BOOST_PP_TUPLE_PUSH_BACK(val,_)),           \
            BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, val))))

#define UTXX_INTERNAL_ENUMV_CASE__(x, _, i, item)                              \
            case BOOST_PP_TUPLE_ELEM(0, item): return metas()[BOOST_PP_INC(i)];
