//----------------------------------------------------------------------------
/// \file   variant.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Variant class that represents a subtype of
/// boost::variant that can hold integer/bool/double/string values.
//----------------------------------------------------------------------------
// Created: 2010-07-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

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


#ifndef _UTXX_VARIANT_HPP_
#define _UTXX_VARIANT_HPP_

#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/property_tree/detail/info_parser_utils.hpp>
#include <stdexcept>
#include <string.h>
#include <stdio.h>

namespace utxx {

struct null {};

class variant: public boost::variant<null, bool, long, double, std::string>
{
    typedef boost::variant    <null, bool, long, double, std::string> base;
    typedef boost::mpl::vector<null, bool, long, double, std::string> internal_types;

    struct string_visitor: public boost::static_visitor<std::string> {
        std::string operator () (null v) const { return "<NULL>";             }
        std::string operator () (bool v) const { return v ? "true" : "false"; }
        std::string operator () (long v) const { return std::to_string(v);    }
        std::string operator () (double v) const {
            char buf[128];
            snprintf(buf, sizeof(buf)-1, "%f", v);
            // Remove trailing zeros.
            for (char* p = buf+strlen(buf); p > buf && *(p-1) != '.'; p--)
                if (*p == '0')
                    *p = '\0';
            return buf;
        }
        std::string operator () (const std::string& v) const { return v; }
    };

    template <typename T>
    T              get(T*)       const { return boost::get<T>(*this); }
    const variant& get(variant*) const { return *this; }
    variant&       get(variant*)       { return *this; }
public:
    typedef boost::mpl::vector<
        int,int16_t,long,uint16_t,uint32_t,uint64_t> int_types;

    typedef boost::mpl::joint_view<
        int_types,
        boost::mpl::vector<null,bool,double,std::string>
    > valid_types;

    typedef boost::mpl::joint_view<
        int_types,
        boost::mpl::vector<null,bool,double,std::string,variant>
    > valid_get_types;

    // For integers - cast them all to long type
    template <typename T>
    class normal_type {
        typedef typename boost::mpl::find<int_types, T>::type int_iter;
    public:
        typedef
            typename boost::mpl::if_
                <boost::is_same<boost::mpl::end<int_types>::type, int_iter>,
                 T, long>::type
            type;
    };

    enum value_type {
          TYPE_NULL
        , TYPE_BOOL
        , TYPE_INT
        , TYPE_DOUBLE
        , TYPE_STRING
    };

    variant() : base(null()) {}
    explicit variant(bool       a) : base(a)        {}
    explicit variant(int16_t    a) : base((long)a)  {}
    explicit variant(int        a) : base((long)a)  {}
    explicit variant(long       a) : base(a)        {}
    explicit variant(uint16_t   a) : base((long)a)  {}
    explicit variant(uint32_t   a) : base((long)a)  {}
    explicit variant(uint64_t   a) : base((long)a)  {}
    explicit variant(double     a) : base(a)        {}

    template <class Ch>
    variant(const std::basic_string<Ch>& a) : base(to_std_string<Ch>(a)) {}
    variant(const char*                  a) : base(std::string(a)) {}
    variant(const std::string&           a) : base(a) {}

    variant(const variant& a) : base((const base&)a) {}

#if __cplusplus >= 201103L
    variant(variant&& a)            { swap(a); }
    variant& operator=(variant&& a) { clear(); swap(a); return *this; }
#endif

    variant(value_type v, const std::string& a) { from_string(v, a); }

    void operator= (const variant& a) { *(base*)this = (const base&)a; }
    //void operator= (null        a)  { *(base*)this = null(); }
    void operator= (int16_t     a)  { *(base*)this = (long)a; }
    void operator= (int         a)  { *(base*)this = (long)a; }
    void operator= (int64_t     a)  { *(base*)this = (long)a; }
    void operator= (uint16_t    a)  { *(base*)this = (long)a; }
    void operator= (uint32_t    a)  { *(base*)this = (long)a; }
    void operator= (uint64_t    a)  { *(base*)this = (long)a; }
    void operator= (const char* a)  { *(base*)this = std::string(a); }
    void operator= (const std::string& a) { *(base*)this = a; }
    void operator= (bool        a)  { variant b(a); *this = b; }

    template <typename T>
    void operator= (T a) {
        typedef typename boost::mpl::find<valid_types, T>::type type_iter;
        BOOST_STATIC_ASSERT(
            (!boost::is_same<
                boost::mpl::end<valid_types>::type,
                type_iter >::value)
        );
        *(base*)this = a;
    }

    void operator+= (int16_t     a)  { *(base*)this = is_null() ? a : to_int() + a; }
    void operator+= (int         a)  { *(base*)this = is_null() ? a : to_int() + a; }
    void operator+= (int64_t     a)  { *(base*)this = is_null() ? a : to_int() + a; }
    void operator+= (uint16_t    a)  { *(base*)this = is_null() ? a : to_int() + a; }
    void operator+= (uint32_t    a)  { *(base*)this = is_null() ? a : to_int() + a; }
    void operator+= (uint64_t    a)  { *(base*)this = is_null() ? (long)a : to_int()+(long)a; }
    void operator+= (double      a)  { *(base*)this = is_null() ? a : to_int() + a; }
    void operator+= (const char* a)  { *(base*)this = is_null() ? a : to_str() + a; }
    void operator+= (const std::string& a) { *(base*)this = is_null() ? a : to_str() + a; }

    template <typename T>
    void operator+= (T a) { throw std::runtime_error("Operation not supported!"); }

    value_type  type()     const { return static_cast<value_type>(which()); }
    const char* type_str() const {
        static const char* s_types[] = { "null", "bool", "int", "double", "string" };
        return s_types[type()];
    }

    /// Set value to null.
    void clear() { *(base*)this = null(); }

    /// Returns true if the value is null.
    bool is_null()      const { return type() == TYPE_NULL; }
    bool is_bool()      const { return type() == TYPE_BOOL; }
    bool is_int()       const { return type() == TYPE_INT;  }
    bool is_double()    const { return type() == TYPE_DOUBLE; }
    bool is_string()    const { return type() == TYPE_STRING; }

    template <typename T>
    bool is_type()      const {
        variant::type_visitor<T> v;
        return boost::apply_visitor(v, *this);
    }

    bool                to_bool()   const { return boost::get<bool>(*this); }
    long                to_int()    const { return boost::get<long>(*this); }
    double              to_float()  const { return boost::get<double>(*this); }
    double              to_double() const { return boost::get<double>(*this); }
    const std::string&  to_str()    const { return boost::get<std::string>(*this); }
    const char*         c_str()     const { return boost::get<std::string>(
                                                    *this).c_str(); }

    /// Returns true if the variant doesn't contain a value or it's an empty string
    bool empty(bool a_check_empty_string = true) const {
        return is_null() || (a_check_empty_string && is_string() && to_str().empty());
    }

    /// Convert value to string.
    std::string to_string() const {
        variant::string_visitor v;
        return boost::apply_visitor(v, *this);
    }

    template <typename T>
    T get() const {
        // Make sure given type is a valid variant type.
        // Note to user: an error raised here indicates that
        // there's a compile-time attempt to convert a variant to 
        // unsupported type.
        typedef typename boost::mpl::find<valid_get_types, T>::type valid_iter;
        BOOST_STATIC_ASSERT(
            (!boost::is_same<boost::mpl::end<valid_get_types>::type, valid_iter>::value));
        // For integers - cast them to long type when fetching from the variant.
        typename normal_type<T>::type* dummy(NULL);
        return get(dummy);
    }

    const variant& get() const { return *this; }

    void from_string(value_type v, const std::string& a) {
        switch (v) {
            case TYPE_NULL:     *this = null(); break;
            case TYPE_BOOL:     *this = a == "true" || a == "yes";      break;
            case TYPE_INT:      *this = boost::lexical_cast<long>(a);   break;
            case TYPE_DOUBLE:   *this = boost::lexical_cast<double>(a); break;
            case TYPE_STRING:   *this = a;
            default:            throw_type_error(type());
        }
    }

    bool operator== (const variant& rhs) const {
        if (type() != rhs.type()) return false;
        switch (type()) {
            case TYPE_NULL:     return true;
            case TYPE_BOOL:     return to_bool()  == rhs.to_bool();
            case TYPE_INT:      return to_int()   == rhs.to_int();
            case TYPE_DOUBLE:   return to_float() == rhs.to_float();
            case TYPE_STRING:   return to_str()   == rhs.to_str();
            default:            throw_type_error(type());
                                return false; // just to pease the compiler
        }
    }

    bool operator< (const variant& rhs) const {
        if (type() < rhs.type()) return true;
        if (type() > rhs.type()) return false;
        switch (type()) {
            case TYPE_NULL:     return false;
            case TYPE_BOOL:     return to_bool()  < rhs.to_bool();
            case TYPE_INT:      return to_int()   < rhs.to_int();
            case TYPE_DOUBLE:   return to_float() < rhs.to_float();
            case TYPE_STRING:   return to_str()   < rhs.to_str();
            default:            throw_type_error(type());
                                return false; // just to pease the compiler
        }
    }

    bool operator> (const variant& rhs) const {
        if (type() > rhs.type()) return true;
        if (type() < rhs.type()) return false;
        switch (type()) {
            case TYPE_NULL:     return false;
            case TYPE_BOOL:     return to_bool()  > rhs.to_bool();
            case TYPE_INT:      return to_int()   > rhs.to_int();
            case TYPE_DOUBLE:   return to_float() > rhs.to_float();
            case TYPE_STRING:   return to_str()   > rhs.to_str();
            default:            throw_type_error(type());
                                return false; // just to pease the compiler
        }
    }

private:
    static void throw_type_error(value_type v) {
        std::stringstream s;
        s << "Unknown type " << v;
        throw std::runtime_error(s.str());
    }

    static std::string to_std_string(const std::basic_string<char>& a) {
        return a;
    }

    template <class Ch>
    static std::string to_std_string(const std::basic_string<Ch>& a) {
        return boost::property_tree::info_parser::convert_chtype<char, Ch>(a);
    }

    template <typename U>
    struct type_visitor: public boost::static_visitor<bool> {
        typedef typename normal_type<U>::type Type;
        bool operator() (Type v) const { return true; }

        template <typename T>
        bool operator() (T v)    const { return false; }
    };
};

static inline std::ostream& operator<< (std::ostream& out, const variant& a) {
    return out << a.to_string();
}

} // namespace utxx

#endif // _UTXX_VARIANT_HPP_
