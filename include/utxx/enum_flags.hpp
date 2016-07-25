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
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/variadic_seq_to_seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/algorithm/string.hpp>
#include <utxx/detail/enum_helper.hpp>
#include <utxx/string.hpp>
#include <utxx/error.hpp>
#include <cassert>
#include <vector>

//------------------------------------------------------------------------------
/// Strongly typed reflectable enum flags declaration
//------------------------------------------------------------------------------
/// ```
/// #include <utxx/enum.hpp>
///
/// UTXX_ENUM_FLAGS                 // Automatically gets NONE=0 item added
/// (MyEnumT, char,
///     Apple,
///     Pear,
///     Grape,
///     Orange
/// );
/// ```
//------------------------------------------------------------------------------
#define UTXX_ENUM_FLAGS(ENUM, TYPE, ...)                                       \
        UTXX_ENUM_FLAGZ(                                                       \
            ENUM, TYPE, NONE, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)            \
        )

//------------------------------------------------------------------------------
/// Strongly typed reflectable enum flags declaration
//------------------------------------------------------------------------------
/// ```
/// #include <utxx/enum_flags.hpp>
///
/// UTXX_ENUM_FLAGZ
/// (MyEnumT, char, Nil,
///     (Apple,  "A")   // Flags can optionally have string value.
///     (Pear,  "PP")
///     (Grape)         // String value defaults to "Grape"
///     (Orange)
/// );
/// ```
/// Sample usage:
/// ```
/// MyEnumT val = MyEnumT::from_string("PP|Orange");
/// std::cout << "Value: " << val.to_string() << std::endl;  // PP|Orange
/// std::cout << "Value: " << val.names()     << std::endl;  // Pear|Orange
/// std::cout << "Value: " << val.values()    << std::endl;  // PP|Orange
/// std::cout << "Value: " << val             << std::endl;  // Pear|Orange
/// ```
//------------------------------------------------------------------------------
#define UTXX_ENUM_FLAGZ(ENUM, TYPE, NONE, ...)                                 \
    struct ENUM {                                                              \
        using value_type = TYPE;                                               \
                                                                               \
        enum type : TYPE {                                                     \
            NONE,                                                              \
            BOOST_PP_SEQ_ENUM(                                                 \
                BOOST_PP_SEQ_FOR_EACH_I(                                       \
                    UTXX_ENUM_INTERNAL_INIT__, _,                              \
                    BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__))),               \
            _END_ =  1 << BOOST_PP_SEQ_SIZE(                                   \
                            BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)),        \
            _ALL_ = std::make_unsigned<TYPE>::type(_END_) - 1                  \
        };                                                                     \
                                                                               \
    public:                                                                    \
        constexpr ENUM()                 : m_val(0)         {}                 \
        explicit constexpr ENUM(type v)  : m_val(size_t(v)) {}                 \
        template <typename... Args>                                            \
        constexpr ENUM(type a, Args... args) : m_val(bor(a, args...)) {}       \
                                                                               \
        ENUM(ENUM&&)                 = default;                                \
        ENUM(ENUM const&)            = default;                                \
                                                                               \
        ENUM& operator=(ENUM const&) = default;                                \
        ENUM& operator=(ENUM&&)      = default;                                \
                                                                               \
        static constexpr const char*   class_name() { return #ENUM; }          \
                                                                               \
        static constexpr bool is_enum()        { return true;   }              \
        static constexpr bool is_flags()       { return true;   }              \
                                                                               \
        void               clear()             { m_val = 0;     }              \
        void               clear(type  a)      { m_val &= ~size_t(a); }        \
        void               clear(ENUM  a)      { m_val &= ~a.m_val;   }        \
        void               clear(size_t a)     { m_val &= ~a;   }              \
                                                                               \
        std::string        names()       const { return to_string("|",true); } \
        std::string        values()      const { return to_string("|",false);} \
                                                                               \
        static const std::string& name(type n) { return meta(n).first;       } \
        static const std::string& value(type n){ return meta(n).second;      } \
                                                                               \
        explicit                                                               \
        constexpr operator size_t()      const { return m_val;  }              \
        explicit                                                               \
        constexpr operator uint()        const { return m_val;  }              \
        constexpr operator type()        const { return type(m_val); }         \
        constexpr bool     empty()       const { return !m_val; }              \
        constexpr bool     has(type   a) const { return m_val & size_t(a); }   \
        constexpr bool has_any(size_t a) const { return m_val & a;         }   \
        constexpr bool has_all(size_t a) const { return (m_val & a) == a;  }   \
        static constexpr bool   valid(size_t a){ return a < _END_;    }        \
        static constexpr size_t size()         { return s_size;       }        \
                                                                               \
        ENUM operator=  (type   a) { m_val  = size_t(a); return *this; }       \
        ENUM operator=  (size_t a) { m_val  = a;         return *this; }       \
        ENUM operator|= (type   a) { m_val |= size_t(a); return *this; }       \
                                                                               \
        friend constexpr ENUM operator| (ENUM a, ENUM b)  {                    \
            return ENUM(type(a.m_val | b.m_val));                              \
        }                                                                      \
                                                                               \
        template <typename StreamT> StreamT&                                   \
        print(StreamT& out, const char* a_delim="|", bool as_name=true) const {\
            bool bp=false;                                                     \
            for (size_t i=0; i < s_size; ++i)                                  \
                if (m_val & (1ul << i)) {                                      \
                    if (bp) out << a_delim;                                    \
                    out << (as_name ? meta(i+1).first : meta(i+1).second);     \
                    bp = true;                                                 \
                }                                                              \
            return out;                                                        \
        }                                                                      \
                                                                               \
        std::string to_string(const char* delim="|", bool as_name=false) const{\
            std::stringstream s;                                               \
            print(s, delim, as_name);                                          \
            return s.str();                                                    \
        }                                                                      \
        static std::string                                                     \
        to_string(type a, const char* a_delim="|", bool as_names=false) {      \
            return ENUM(a).to_string(a_delim, as_names);                       \
        }                                                                      \
                                                                               \
        static ENUM                                                            \
        from_string(const char* a, bool a_nocase=false, bool as_names=false,   \
                    const char* a_delims=nullptr)                              \
        {                                                                      \
            const auto delim = a_delims ? a_delims : "|,; ";                   \
            const auto sep   = boost::is_any_of(delim);                        \
            const auto f     = [=](auto& s1, auto& s2) {                       \
                return (a_nocase ? &strcasecmp:&strcmp)(s1.c_str(),s2.c_str());\
            };                                                                 \
            std::vector<std::string> v; size_t val = 0;                        \
            boost::split(v, a, sep, boost::token_compress_on);                 \
            for (auto& s : v) {                                                \
              size_t n = 0;                                                    \
              for (size_t i=1; i <= s_size; i++)                               \
                if (!f((as_names ? meta(i).first : meta(i).second), s)) {      \
                  n |= (1ul << (i-1));                                         \
                  break;                                                       \
                }                                                              \
                if (!n) UTXX_THROW_BADARG_ERROR("Invalid flag value: ", s);    \
                val |= n;                                                      \
            }                                                                  \
            return type(val);                                                  \
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
        static ENUM from_names(const char* a, bool a_nocase=false)    {        \
            return from_string(a, a_nocase, true);                             \
        }                                                                      \
        static ENUM from_values(const char* a, bool a_nocase=false)   {        \
            return from_string(a, a_nocase, false);                            \
        }                                                                      \
                                                                               \
        template <typename StreamT>                                            \
        inline friend StreamT& operator<< (StreamT& out, ENUM const& a) {      \
            out << a.values();                                                 \
            return out;                                                        \
        }                                                                      \
                                                                               \
        template <typename StreamT>                                            \
        inline friend StreamT& operator<< (StreamT& out, ENUM::type a)  {      \
            return out << ENUM(a);                                             \
        }                                                                      \
                                                                               \
        template <typename Visitor>                                            \
        void for_each(const Visitor& a_fun) {                                  \
            for (size_t i=0; i < s_size; i++) {                                \
                type v = type(1ul << i);                                       \
                if ((m_val & v) && !a_fun(v))                                  \
                    break;                                                     \
            }                                                                  \
        }                                                                      \
    private:                                                                   \
        static const size_t s_size =                                           \
            BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__));      \
        static const std::pair<std::string,std::string>* metas() {             \
            static const std::pair<std::string,std::string> s_metas[] = {      \
                std::make_pair(#NONE, #NONE),                                  \
                BOOST_PP_SEQ_ENUM(                                             \
                    BOOST_PP_SEQ_TRANSFORM(                                    \
                        UTXX_ENUM_INTERNAL_GET_PAIR__, ,                       \
                        BOOST_PP_VARIADIC_SEQ_TO_SEQ(__VA_ARGS__)              \
                ))                                                             \
            };                                                                 \
            return s_metas;                                                    \
        }                                                                      \
                                                                               \
        static const std::pair<std::string,std::string>& meta(type n) {        \
            return meta(TYPE(n));                                              \
        };                                                                     \
        static const std::pair<std::string,std::string>& meta(TYPE n) {        \
            assert(size_t(n) <= s_size);                                       \
            return metas()[n];                                                 \
        };                                                                     \
        static constexpr type bor()       { return NONE; }                     \
        static constexpr type bor(type a) { return a;    }                     \
                                                                               \
        template <typename... Args>                                            \
        static constexpr type bor(type a, Args... args)                        \
        { return type(size_t(a) | size_t(bor(args...))); }                     \
                                                                               \
        UTXX__ENUM_FRIEND_SERIALIZATION__;                                     \
                                                                               \
        size_t m_val;                                                          \
    }

// Internal macro for supporting BOOST_PP_SEQ_TRANSFORM
#define UTXX_ENUM_INTERNAL_INIT__(x, _, i, item)                               \
            (BOOST_PP_TUPLE_ELEM(0, item) = 1ul << i)
