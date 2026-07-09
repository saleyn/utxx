//----------------------------------------------------------------------------
/// \file   variant.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Variant class holding nullptr/bool/long/double/string values.
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
#pragma once

#include <variant>
#include <type_traits>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <codecvt>
#include <locale>

namespace utxx {

using std::nullptr_t;

class variant
{
public:
    using base = std::variant<nullptr_t, bool, long, double, std::string>;

private:
    base m_val;

    struct string_visitor {
        std::string operator()(nullptr_t)          const { return "<NULL>";             }
        std::string operator()(bool v)             const { return v ? "true" : "false"; }
        std::string operator()(long v)             const { return std::to_string(v);    }
        std::string operator()(double v)           const {
            char buf[128];
            snprintf(buf, sizeof(buf)-1, "%f", v);
            for (char* p = buf+strlen(buf); p > buf && *(p-1) != '.'; p--)
                if (*p == '0') *p = '\0';
            return buf;
        }
        std::string operator()(const std::string& v) const { return v; }
    };

    // Helper: is T one of the listed types?
    template <typename T, typename... Ts>
    static constexpr bool is_one_of_v = (std::is_same_v<T, Ts> || ...);

    template <typename T>
    const T& get_as() const { return std::get<T>(m_val); }

    const variant& get(const variant*) const { return *this; }
    variant&       get(variant*)             { return *this; }
    const base&    get(const base*)    const { return m_val; }
    base&          get(base*)                { return m_val; }

    template <typename T>
    T get(const T*) const { return std::get<T>(m_val); }
    double get(const double*) const { return to_double(); }

public:
    // Integer types that are all stored as long
    using int_types = std::tuple<int, int16_t, long, int64_t, uint16_t, uint32_t, uint64_t>;

    // All valid assignment types
    template <typename T>
    static constexpr bool is_int_type_v =
        is_one_of_v<T, int, int16_t, long, int64_t, uint16_t, uint32_t, uint64_t>;

    template <typename T>
    static constexpr bool is_valid_type_v =
        is_one_of_v<T, nullptr_t, bool, long, double, std::string>
        || is_int_type_v<T>;

    // Normalise integer types to long, keep others as-is
    template <typename T>
    using normal_type_t = std::conditional_t<is_int_type_v<T>, long, T>;

    enum value_type {
          TYPE_NULL
        , TYPE_BOOL
        , TYPE_INT
        , TYPE_DOUBLE
        , TYPE_STRING
    };

    static const variant& unassigned() { static variant s_val; return s_val; }

    variant() : m_val(nullptr) {}
    explicit variant(bool              a) : m_val(a)             {}
    explicit variant(long              a) : m_val(a)             {}
    explicit variant(double            a) : m_val(a)             {}
    variant(const char*                a) : m_val(std::string(a)) {}
    variant(const std::string&         a) : m_val(a)             {}
    variant(std::string&&              a) : m_val(std::move(a))  {}

    // Integer types that may or may not alias long depending on the platform
    template <typename T, std::enable_if_t<is_int_type_v<T> && !std::is_same_v<T,long> && !std::is_same_v<T,bool>, int> = 0>
    explicit variant(T a) : m_val(static_cast<long>(a)) {}

    template <class Ch>
    variant(const std::basic_string<Ch>& a) : m_val(to_std_string(a)) {}

    variant(const variant&) = default;
    variant(variant&&)      = default;
    variant& operator=(const variant&) = default;
    variant& operator=(variant&&)      = default;

    variant(value_type v, const std::string& a) { from_string(v, a); }

    void operator=(const char*        a) { m_val = std::string(a); }
    void operator=(const std::string& a) { m_val = a; }
    void operator=(bool               a) { m_val = a; }
    void operator=(long               a) { m_val = a; }
    void operator=(double             a) { m_val = a; }
    void operator=(nullptr_t)            { m_val = nullptr; }

    // Handles int, int16_t, uint16_t, uint32_t, uint64_t, and int64_t when
    // int64_t is a distinct type from long (e.g. Windows / 32-bit).
    template <typename T>
    std::enable_if_t<is_int_type_v<T> && !std::is_same_v<T, long>>
    operator=(T a) { m_val = static_cast<long>(a); }

    void operator+=(long              a) { m_val = is_null() ? a       : to_int() + a; }
    void operator+=(double            a) { m_val = is_null() ? a       : to_int() + a; }
    void operator+=(const char*       a) { m_val = is_null() ? std::string(a) : to_str() + a; }
    void operator+=(const std::string& a){ m_val = is_null() ? a       : to_str() + a; }

    template <typename T>
    std::enable_if_t<is_int_type_v<T> && !std::is_same_v<T, long>>
    operator+=(T a) { m_val = is_null() ? (long)a : to_int() + (long)a; }

    template <typename T>
    void operator+=(T) { throw std::runtime_error("Operation not supported!"); }

    value_type  type()     const { return static_cast<value_type>(m_val.index()); }
    const char* type_str() const { return type_str(type()); }

    static const char* type_str(value_type t) {
        static const char* s_types[] = { "null", "bool", "int", "double", "string" };
        return s_types[t];
    }

    void clear() { m_val = nullptr; }

    bool is_assigned()   const { return *this  != unassigned(); }
    bool is_unassigned() const { return *this  == unassigned(); }
    bool is_null()       const { return type() == TYPE_NULL;    }
    bool is_bool()       const { return type() == TYPE_BOOL;    }
    bool is_int()        const { return type() == TYPE_INT;     }
    bool is_double()     const { return type() == TYPE_DOUBLE;  }
    bool is_string()     const { return type() == TYPE_STRING;  }

    /// @param relaxed when true, allow integral types to be converted to doubles
    template <typename T>
    bool is_type(bool relaxed=false) const {
        if constexpr(std::is_same_v<T, variant> || std::is_same_v<T, base>)
            return true;
        return std::visit(type_visitor<T>{relaxed}, m_val);
    }

    bool                to_bool()   const { return std::get<bool>(m_val); }
    long                to_int()    const { return std::get<long>(m_val); }
    double              to_double() const { return is_int()
                                                 ? double(std::get<long>(m_val))
                                                 : std::get<double>(m_val); }
    double              to_float()  const { return to_double(); }
    const std::string&  to_str()    const { return std::get<std::string>(m_val); }
    const char*         c_str()     const { return std::get<std::string>(m_val).c_str(); }

    /// Convert variant to given type T
    template <typename T> T to() const;

    /// Returns true if the variant doesn't contain a value or it's an empty string
    bool empty(bool a_check_empty_string = true) const {
        return is_null() || (a_check_empty_string && is_string() && to_str().empty());
    }

    /// Convert value to string.
    std::string to_string() const {
        return std::visit(string_visitor{}, m_val);
    }

    template <typename T>
    T get() const {
        static_assert(is_valid_type_v<T> ||
                      std::is_same_v<T, variant> ||
                      std::is_same_v<T, base>,
                      "T is not a valid variant type");
        using NT = normal_type_t<T>;
        NT* dummy = nullptr;
        return get(dummy);
    }

    const variant& get() const { return *this; }

    void from_string(value_type v, const std::string& a) {
        switch (v) {
            case TYPE_NULL:     m_val = nullptr;                          break;
            case TYPE_BOOL:     m_val = (a == "true" || a == "yes");      break;
            case TYPE_INT:      m_val = std::stol(a);                     break;
            case TYPE_DOUBLE:   m_val = std::stod(a);                     break;
            case TYPE_STRING:   m_val = a;                                break;
            default:            throw_type_error(type());
        }
    }

    bool operator==(const variant& rhs) const {
        if (type() != rhs.type()) return false;
        switch (type()) {
            case TYPE_NULL:     return true;
            case TYPE_BOOL:     return to_bool()  == rhs.to_bool();
            case TYPE_INT:      return to_int()   == rhs.to_int();
            case TYPE_DOUBLE:   return to_float() == rhs.to_float();
            case TYPE_STRING:   return to_str()   == rhs.to_str();
            default:            throw_type_error(type()); return false;
        }
    }

    bool operator!=(const variant& rhs) const { return !(operator==(rhs)); }

    bool operator<(const variant& rhs) const {
        if (type() < rhs.type()) return true;
        if (type() > rhs.type()) return false;
        switch (type()) {
            case TYPE_NULL:     return false;
            case TYPE_BOOL:     return to_bool()  < rhs.to_bool();
            case TYPE_INT:      return to_int()   < rhs.to_int();
            case TYPE_DOUBLE:   return to_float() < rhs.to_float();
            case TYPE_STRING:   return to_str()   < rhs.to_str();
            default:            throw_type_error(type()); return false;
        }
    }

    bool operator>(const variant& rhs) const {
        if (type() > rhs.type()) return true;
        if (type() < rhs.type()) return false;
        switch (type()) {
            case TYPE_NULL:     return false;
            case TYPE_BOOL:     return to_bool()  > rhs.to_bool();
            case TYPE_INT:      return to_int()   > rhs.to_int();
            case TYPE_DOUBLE:   return to_float() > rhs.to_float();
            case TYPE_STRING:   return to_str()   > rhs.to_str();
            default:            throw_type_error(type()); return false;
        }
    }

    // Access underlying std::variant (for code that needed boost::variant base)
    const base& as_base() const { return m_val; }
    base&       as_base()       { return m_val; }

private:
    static void throw_type_error(value_type v) {
        std::stringstream s;
        s << "Unknown type " << v;
        throw std::runtime_error(s.str());
    }

    static std::string to_std_string(const std::string& a) { return a; }

    template <class Ch>
    static std::string to_std_string(const std::basic_string<Ch>& a) {
        // Convert wide/char16/char32 string to UTF-8
        std::wstring_convert<std::codecvt_utf8<Ch>, Ch> conv;
        return conv.to_bytes(a);
    }

    template <typename U>
    struct type_visitor {
        type_visitor(bool relaxed = false) : m_relaxed(relaxed) {}

        using Type = normal_type_t<U>;

        bool operator()(double) const {
            return std::is_same_v<Type,double> || (m_relaxed && std::is_same_v<Type,long>);
        }
        template <typename T>
        bool operator()(T) const { return std::is_same_v<Type, T>; }
    private:
        bool m_relaxed;
    };
};

// Specializations of 'to' template function
template<> double       variant::to<double>()       const;
template<> int          variant::to<int>()          const;
template<> int64_t      variant::to<int64_t>()      const;
template<> uint64_t     variant::to<uint64_t>()     const;
template<> uint32_t     variant::to<uint32_t>()     const;
template<> bool         variant::to<bool>()         const;
template<> std::string  variant::to<std::string>()  const;

static inline std::ostream& operator<<(std::ostream& out, const variant& a) {
    return out << a.to_string();
}

} // namespace utxx
