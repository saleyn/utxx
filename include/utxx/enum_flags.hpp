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
#include <boost/algorithm/string.hpp>
#include <utxx/string.hpp>
#include <utxx/error.hpp>
#include <cassert>
#include <vector>

#ifdef UTXX_ENUM_SUPPORT_SERIALIZATION
#include <boost/serialization/access.hpp>
#define UTXX__ENUM_FRIEND_SERIALIZATION__ \
    friend class boost::serialization::access
#endif

/// Strongly typed reflectable enum flags declaration
//  #include <utxx/enum_flags.hpp>
//
//  UTXX_ENUM_FLAGS
//  (MyEnumT, char,
//      Apple,
//      Pear,
//      Grape,
//      Orange
//  );
//
// Sample usage:
//  MyEnumT val = MyEnumT::from_string("Pear|Orange");
//  std::cout << "Value: " << to_string(val) << std::endl;
//  std::cout << "Value: " << val            << std::endl;
//
#define UTXX_ENUM_FLAGS(ENUM, TYPE, ...)                                      \
    struct ENUM {                                                             \
        enum type : TYPE {                                                    \
            NONE,                                                             \
            BOOST_PP_SEQ_ENUM(                                                \
                BOOST_PP_SEQ_FOR_EACH_I(                                      \
                    UTXX_INTERNAL_FLAGS_INIT, _,                              \
                    BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)                     \
            )),                                                               \
            _END_ =  1 << BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),                \
            _ALL_ = size_t(_END_) - 1                                         \
        };                                                                    \
                                                                              \
    private:                                                                  \
        static const size_t s_size = BOOST_PP_VARIADIC_SIZE(__VA_ARGS__);     \
        static const std::string* names() {                                   \
            static const std::string s_names[] = {                            \
                "NONE",                                                       \
                BOOST_PP_SEQ_ENUM(                                            \
                    BOOST_PP_SEQ_TRANSFORM(                                   \
                        UTXX_INTERNAL_FLAGS_STRINGIFY, ,                      \
                        BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)                 \
                ))                                                            \
            };                                                                \
            return s_names;                                                   \
        };                                                                    \
                                                                              \
        static const std::string& name(size_t n) {                            \
            assert(n <= s_size);                                              \
            return names()[n];                                                \
        };                                                                    \
        static constexpr type bor()       { return NONE; }                    \
        static constexpr type bor(type a) { return a;    }                    \
                                                                              \
        template <typename... Args>                                           \
        static constexpr type bor(type a, Args... args)                       \
        { return type(size_t(a) | size_t(bor(args...))); }                    \
                                                                              \
        UTXX__ENUM_FRIEND_SERIALIZATION__;                                    \
                                                                              \
        size_t m_val;                                                         \
                                                                              \
    public:                                                                   \
        constexpr ENUM()                  : m_val(0)         {}               \
        explicit constexpr ENUM(type v)  : m_val(size_t(v))  {}               \
        template <typename... Args>                                           \
        constexpr ENUM(type a, Args... args) : m_val(bor(a, args...)) {}      \
                                                                              \
        ENUM(ENUM&&)                 = default;                               \
        ENUM(ENUM const&)            = default;                               \
                                                                              \
        ENUM& operator=(ENUM const&) = default;                               \
        ENUM& operator=(ENUM&&)      = default;                               \
                                                                              \
        void               clear()             { m_val = 0;     }             \
        void               clear(type  a)      { m_val &= ~size_t(a); }       \
        void               clear(ENUM  a)      { m_val &= ~a.m_val;   }       \
        void               clear(size_t a)     { m_val &= ~a;   }             \
        constexpr operator size_t()      const { return m_val;  }             \
        constexpr operator uint()        const { return m_val;  }             \
        constexpr operator type()        const { return type(m_val); }        \
        constexpr bool     empty()       const { return !m_val; }             \
        constexpr bool     has(type   a) const { return m_val & size_t(a); }  \
        constexpr bool has_any(size_t a) const { return m_val & a;         }  \
        constexpr bool has_all(size_t a) const { return (m_val & a) == a;  }  \
        constexpr bool   valid(size_t a) const { return a < _END_;    }       \
        static constexpr size_t size()         { return s_size;       }       \
                                                                              \
        ENUM operator=  (type   a) { m_val  = size_t(a); return *this; }      \
        ENUM operator=  (size_t a) { m_val  = a;         return *this; }      \
        ENUM operator|= (type   a) { m_val |= size_t(a); return *this; }      \
                                                                              \
        friend constexpr ENUM operator| (ENUM a, ENUM b)  {                   \
            return ENUM(type(a.m_val | b.m_val));                             \
        }                                                                     \
        std::string to_string() const {                                       \
            std::stringstream s;                                              \
            for (size_t i=0; i < s_size; ++i)                                 \
                if (m_val & (1ul << i))                                       \
                    s << (s.rdbuf()->in_avail() ? "|" : "") << name(i+1);     \
            return s.str();                                                   \
        }                                                                     \
        static std::string to_string(type a){ return ENUM(a).to_string();}    \
                                                                              \
        static ENUM from_string(const std::string& a, bool a_nocase=false){   \
            return from_string(a.c_str(), a_nocase);                          \
        }                                                                     \
                                                                              \
        static ENUM from_string(const char* a, bool a_nocase=false){          \
            static const auto s_sep = boost::is_any_of("|,; ");               \
            auto f = a_nocase ? &strcasecmp : &strcmp;                        \
            std::vector<std::string> v; size_t val = 0;                       \
            boost::split(v, a, s_sep, boost::token_compress_on);              \
            for (auto& s : v) {                                               \
                size_t n = 0;                                                 \
                for (size_t i=0; i < s_size; i++)                             \
                    if (f((names()[i+1]).c_str(), s.c_str()) == 0) {          \
                        n |= (1ul << i);                                      \
                        break;                                                \
                    }                                                         \
                if (!n)                                                       \
                    UTXX_THROW_BADARG_ERROR("Invalid flag value: ", s);       \
                val |= n;                                                     \
            }                                                                 \
            return type(val);                                                 \
        }                                                                     \
                                                                              \
        static ENUM from_string_nc(const std::string& a) {                    \
            return from_string(a, true);                                      \
        }                                                                     \
                                                                              \
        static ENUM from_string_nc(const char* a){return from_string(a,true);}\
                                                                              \
        inline friend std::ostream& operator<< (std::ostream& out, ENUM a) {  \
            return out << ENUM::to_string(a);                                 \
        }                                                                     \
                                                                              \
        template <typename Visitor>                                           \
        void for_each(const Visitor& a_fun) {                                 \
            for (size_t i=0; i < s_size; i++) {                               \
                type v = type(1ul << i);                                      \
                if ((m_val & v) && !a_fun(v))                                 \
                    break;                                                    \
            }                                                                 \
        }                                                                     \
    }

// DEPRACATED!!! Use UTXX_ENUM_FLAGS() instead!
/// Untyped enum flags declaration:
//  #include <utxx/enum_flags.hpp>
//
//  UTXX_DEFINE_FLAGS
//  (MyEnumT,
//      Apple,
//      Pear,
//      Grape,
//      Orange
//  );
//
// Sample usage:
//  MyEnumT val = MyEnumT::from_string("Pear|Orange");
//  std::cout << "Value: " << to_string(val) << std::endl;
//  std::cout << "Value: " << val            << std::endl;
//
#define UTXX_DEFINE_FLAGS(ENUM, ...)                                        \
    struct [[deprecated]] ENUM {                                            \
        enum etype {                                                        \
            NONE,                                                           \
            BOOST_PP_SEQ_ENUM(                                              \
                BOOST_PP_SEQ_FOR_EACH_I(                                    \
                    UTXX_INTERNAL_FLAGS_INIT, _,                            \
                    BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)                   \
            )),                                                             \
            _END_ =  1 << BOOST_PP_VARIADIC_SIZE(__VA_ARGS__),              \
            _ALL_ = size_t(_END_) - 1                                       \
        };                                                                  \
                                                                            \
    private:                                                                \
        static const size_t s_size = BOOST_PP_VARIADIC_SIZE(__VA_ARGS__);   \
        static const std::string* names() {                                 \
            static const std::string s_names[] = {                          \
                "NONE",                                                     \
                BOOST_PP_SEQ_ENUM(                                          \
                    BOOST_PP_SEQ_TRANSFORM(                                 \
                        UTXX_INTERNAL_FLAGS_STRINGIFY, ,                    \
                        BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)               \
                ))                                                          \
            };                                                              \
            return s_names;                                                 \
        };                                                                  \
                                                                            \
        static const std::string& name(size_t n) {                          \
            assert(n <= s_size);                                            \
            return names()[n];                                              \
        };                                                                  \
        static constexpr etype bor()        { return NONE; }                \
        static constexpr etype bor(etype a) { return a;    }                \
                                                                            \
        template <typename... Args>                                         \
        static constexpr etype bor(etype a, Args... args)                   \
        { return etype(size_t(a) | size_t(bor(args...))); }                 \
                                                                            \
        UTXX__ENUM_FRIEND_SERIALIZATION__;                                  \
                                                                            \
        size_t m_val;                                                       \
                                                                            \
    public:                                                                 \
        constexpr ENUM()                  : m_val(0)          {}            \
        explicit constexpr ENUM(etype v)  : m_val(size_t(v))  {}            \
        template <typename... Args>                                         \
        constexpr ENUM(etype a, Args... args) : m_val(bor(a, args...)) {}   \
                                                                            \
        ENUM(ENUM&&)                 = default;                             \
        ENUM(ENUM const&)            = default;                             \
                                                                            \
        ENUM& operator=(ENUM const&) = default;                             \
        ENUM& operator=(ENUM&&)      = default;                             \
                                                                            \
        void               clear()             { m_val = 0;     }           \
        void               clear(etype  a)     { m_val &= ~size_t(a); }     \
        void               clear(size_t a)     { m_val &= ~a;   }           \
        constexpr operator size_t()      const { return m_val;  }           \
        constexpr operator uint()        const { return m_val;  }           \
        constexpr operator etype()       const { return etype(m_val); }     \
        constexpr bool     empty()       const { return !m_val; }           \
        constexpr bool     has(etype  a) const { return m_val & size_t(a); }\
        constexpr bool has_any(size_t a) const { return m_val & a;         }\
        constexpr bool has_all(size_t a) const { return (m_val & a) == a;  }\
        constexpr bool   valid(size_t a) const { return a < _END_;    }     \
        static constexpr size_t size()         { return s_size;       }     \
                                                                            \
        ENUM operator=  (etype  a) { m_val  = size_t(a); return *this; }    \
        ENUM operator=  (size_t a) { m_val  = a;         return *this; }    \
        ENUM operator|= (etype  a) { m_val |= size_t(a); return *this; }    \
                                                                            \
        friend constexpr ENUM operator| (ENUM a, ENUM b)  {                 \
            return ENUM(etype(a.m_val | b.m_val));                          \
        }                                                                   \
        std::string to_string() const {                                     \
            std::stringstream s;                                            \
            for (size_t i=0; i < s_size; ++i)                               \
                if (m_val & (1ul << i))                                     \
                    s << (s.rdbuf()->in_avail() ? "|" : "") << name(i+1);   \
            return s.str();                                                 \
        }                                                                   \
        static std::string to_string(etype a){ return ENUM(a).to_string();} \
                                                                            \
        static ENUM from_string(const std::string& a, bool a_nocase=false){ \
            return from_string(a.c_str(), a_nocase);                        \
        }                                                                   \
                                                                            \
        static ENUM from_string(const char* a, bool a_nocase=false){        \
            static const auto s_sep = boost::is_any_of("|,; ");             \
            auto f = a_nocase ? &strcasecmp : &strcmp;                      \
            std::vector<std::string> v; size_t val = 0;                     \
            boost::split(v, a, s_sep, boost::token_compress_on);            \
            for (auto& s : v) {                                             \
                size_t n = 0;                                               \
                for (size_t i=0; i < s_size; i++)                           \
                    if (f((names()[i+1]).c_str(), s.c_str()) == 0) {        \
                        n |= (1ul << i);                                    \
                        break;                                              \
                    }                                                       \
                if (!n)                                                     \
                    UTXX_THROW_BADARG_ERROR("Invalid flag value: ", s);     \
                val |= n;                                                   \
            }                                                               \
            return etype(val);                                              \
        }                                                                   \
                                                                            \
        inline friend std::ostream& operator<< (std::ostream& out, ENUM a) {\
            return out << ENUM::to_string(a);                               \
        }                                                                   \
                                                                            \
        template <typename Visitor>                                         \
        void for_each(const Visitor& a_fun) {                               \
            for (size_t i=0; i < s_size; i++) {                             \
                etype v = etype(1ul << i);                                  \
                if ((m_val & v) && !a_fun(v))                               \
                    break;                                                  \
            }                                                               \
        }                                                                   \
    }

// Internal macro for supporting BOOST_PP_SEQ_TRANSFORM
#define UTXX_INTERNAL_FLAGS_STRINGIFY(x, _, item) BOOST_PP_STRINGIZE(item)
#define UTXX_INTERNAL_FLAGS_INIT(x, _, i, item)   (item = 1ul << i)
