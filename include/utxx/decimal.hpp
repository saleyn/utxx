//------------------------------------------------------------------------------
/// \file   decimal.hpp
/// \author Serge Aleynikov
//------------------------------------------------------------------------------
/// \brief This file defines enum stringification declaration macro.
//------------------------------------------------------------------------------
// Copyright (c) 2016 Serge Aleynikov <saleyn@gmail.com>
// Created: 2016-06-01
//------------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx open-source project.

Copyright (C) 2016 Serge Aleynikov <saleyn@gmail.com>

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

#if (__GNUC__ >= 5) && (__GNUC_MINOR__ >= 0)
# define CONSTEXPR constexpr
#elif !defined(CONSTEXPR)
# define CONSTEXPR
#endif

#if __SIZEOF_LONG__ < 8
#   warning "utxx::decimal() not supported!"
#else

#include <limits>
#include <cmath>
#include <utxx/compiler_hints.hpp>
#include <utxx/convert.hpp>
#include <utxx/print.hpp>

namespace utxx {

//------------------------------------------------------------------------------
/// Decimal number representation
//------------------------------------------------------------------------------
/// NOTE: in this implementation the mantissa is limitted to 17 digits of
/// precision (counting digits before and after the decimal separator), and
/// exponent is limitted to 126.  Exponent 127 is reserved for NAN value.
//------------------------------------------------------------------------------
class decimal
{
    int8_t m_exp :  8;
    long   m_mant: 56;

    template <typename T> using nlimits = std::numeric_limits<T>;
    static constexpr int     nullexp()   { return nlimits<int8_t>::max();      }

public:
    static constexpr double  nan()       { return nlimits<double>::quiet_NaN();}
    static const decimal&    null_value(){ static decimal s(nullptr); return s;}

    constexpr decimal() noexcept : decimal(0, 0) {
        static_assert(sizeof(decimal) == sizeof(long), "Invalid size");
    }
    explicit
    constexpr decimal(std::nullptr_t) noexcept : m_exp(nullexp()), m_mant(0) {}
    explicit
    CONSTEXPR decimal(int  m) noexcept : m_exp(0), m_mant(m) { normalize(); }
    explicit
    CONSTEXPR decimal(long m) noexcept : m_exp(0), m_mant(m) { normalize(); }
    constexpr decimal(int e, long m) noexcept : m_exp(e), m_mant(m) {
        //assert((e>=-63 && e<=63) || (e==nullexp() && m == 0));
    }

    decimal(decimal&&      a) : m_exp(a.m_exp), m_mant(a.m_mant) {}
    decimal(decimal const& a) : m_exp(a.m_exp), m_mant(a.m_mant) {}

    /// Construct a decimal from a double.
    decimal(double x, uint precision)       { from_double(x, precision); }

    void operator=(decimal const& a)        { *(long*)this = *(long*)&a; }
    void operator=(decimal&& a)             { *(long*)this = *(long*)&a; }

    bool operator==(decimal const& a) const { return *(long*)this==*(long*)&a; }
    bool operator!=(decimal const& a) const { return !operator==(a);           }

    int    exp()       const { return m_exp;  }
    long   mantissa()  const { return m_mant; }
    double value()     const { return is_null() ? nan() : pow10(m_exp)*m_mant; }

    bool   is_null()   const { return *this == null_value();   }
    void   set_null()        { *this = null_value();           }
    void   clear()           { *(long*)this = 0l;              }

    static double pow10(int a_exp) {
        static double s_pow10[] = {
            1.0e-63, 1.0e-62, 1.0e-61, 1.0e-60, 1.0e-59, 1.0e-58, 1.0e-57, 1.0e-56,
            1.0e-55, 1.0e-54, 1.0e-53, 1.0e-52, 1.0e-51, 1.0e-50, 1.0e-49, 1.0e-48,
            1.0e-47, 1.0e-46, 1.0e-45, 1.0e-44, 1.0e-43, 1.0e-42, 1.0e-41, 1.0e-40,
            1.0e-39, 1.0e-38, 1.0e-37, 1.0e-36, 1.0e-35, 1.0e-34, 1.0e-33, 1.0e-32,
            1.0e-31, 1.0e-30, 1.0e-29, 1.0e-28, 1.0e-27, 1.0e-26, 1.0e-25, 1.0e-24,
            1.0e-23, 1.0e-22, 1.0e-21, 1.0e-20, 1.0e-19, 1.0e-18, 1.0e-17, 1.0e-16,
            1.0e-15, 1.0e-14, 1.0e-13, 1.0e-12, 1.0e-11, 1.0e-10, 1.0e-9,  1.0e-8,
            1.0e-7,  1.0e-6,  1.0e-5 , 1.0e-4 , 1.0e-3,  1.0e-2 , 1.0e-1,  1.0e0,
            1.0e+1,  1.0e+2,  1.0e+3,  1.0e+4,  1.0e+5,  1.0e+6,  1.0e+7,  1.0e+8 ,
            1.0e+9 , 1.0e+10, 1.0e+11, 1.0e+12, 1.0e+13, 1.0e+14, 1.0e+15, 1.0e+16,
            1.0e+17, 1.0e+18, 1.0e+19, 1.0e+20, 1.0e+21, 1.0e+22, 1.0e+23, 1.0e+24,
            1.0e+25, 1.0e+26, 1.0e+27, 1.0e+28, 1.0e+29, 1.0e+30, 1.0e+31, 1.0e+32,
            1.0e+33, 1.0e+34, 1.0e+35, 1.0e+36, 1.0e+37, 1.0e+38, 1.0e+39, 1.0e+40,
            1.0e+41, 1.0e+42, 1.0e+43, 1.0e+44, 1.0e+45, 1.0e+46, 1.0e+47, 1.0e+48,
            1.0e+49, 1.0e+50, 1.0e+51, 1.0e+52, 1.0e+53, 1.0e+54, 1.0e+55, 1.0e+56,
            1.0e+57, 1.0e+58, 1.0e+59, 1.0e+60, 1.0e+61, 1.0e+62, 1.0e+63
        };
        return UNLIKELY(a_exp < -63 || a_exp > 63)
             ? std::pow(10.0, a_exp) : s_pow10[a_exp + 63];
    }

    // Addition / Subtraction: performed on the exponent and mantissa separately

    decimal& operator+=(decimal const& a_rhs) {
        m_exp  += a_rhs.m_exp;
        m_mant += a_rhs.m_mant;
        return *this;
    }

    decimal& operator-=(decimal const& a_rhs) {
        m_exp  -= a_rhs.m_exp;
        m_mant -= a_rhs.m_mant;
        return *this;
    }

    decimal operator+  (decimal const& a_rhs) const {
        auto e = m_exp  + a_rhs.m_exp;
        auto m = m_mant + a_rhs.m_mant;
        return decimal(e, m);
    }

    decimal operator-  (decimal const& a_rhs) const {
        auto e = m_exp  - a_rhs.m_exp;
        auto m = m_mant - a_rhs.m_mant;
        return decimal(e, m);
    }

    explicit
    operator double() const { return value(); }

    template <typename StreamT>
    StreamT& print(StreamT& out) const {
        double v = value();
        if (exp() >= 0) {
            out << long(v + 0.5);
            return out;
    }
    char buf[128];
        if (ftoa_left<true>(v, buf, sizeof(buf), -m_exp, true) < 0)
            out << v;
        else
            out << buf;
        return out;
    }

    std::string to_string() const {
        utxx::buffered_print s; print(s);
        return s.to_string();
    }

    template <typename StreamT> inline friend
    StreamT& operator<<(StreamT& out, decimal const& d) { return d.print(out); }

    /// Construct a decimal from a double.
    void from_double(double x, int precision)
    {
        if (std::isnan(x)) {
            set_null();
            return;
        }

        auto   v = x * pow10(precision);
        auto   m = (long)(v >= 0 ? v + 0.5 : v - 0.5);
        int    e = precision;

        // Normalize
        for(;  m && m%10 == 0; --e, m /= 10);
        m_exp  = -e;
        m_mant = m;
    }

    /// \param a_const_exp can be either const or initial value of the exponent
    CONSTEXPR
    decimal& normalize(int a_const_exp = 0) {
        if (!a_const_exp) {
            while (m_mant != 0 && m_mant % 10 == 0) {
                m_mant /= 10;
                m_exp  += 1;
            }

            if (m_mant == 0)
                m_exp  =  0;
        } else {
            // Adjust m_mant and m_exp if the exponent is required to be const
            auto   diff =  m_exp - a_const_exp;
            if  (  diff == 0) return *this;
            for (; diff >  0; --diff) m_mant *= 10;
            for (; diff <  0; ++diff) m_mant /= 10;
            m_exp = a_const_exp;
        }
        return *this;
    }
};

} // namespace utxx

#endif