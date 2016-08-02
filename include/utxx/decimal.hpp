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
    static constexpr int    nullexp()   { return nlimits<int8_t>::max();       }

public:
    static constexpr double nan()       { return nlimits<double>::quiet_NaN(); }
    static decimal   const& null_value(){ static decimal s(nullptr); return s; }

    constexpr decimal() : decimal(0, 0) {
        static_assert(sizeof(decimal) == sizeof(long), "Invalid size");
    }
    explicit
    constexpr decimal(std::nullptr_t)  : m_exp(nullexp()), m_mant(0) {}
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
    /// @throw runtime_exception if |x| >= 10^250
    explicit
    decimal(double x) { from_double(x); }

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
    static constexpr double s_pow10[] = {
        1e-10, 1e-9, 1e-8, 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1,
        1.0,
        1e+1,  1e+2, 1e+3, 1e+4, 1e+5, 1e+6, 1e+7, 1e+8, 1e+9, 1e+10
    };
    return UNLIKELY(a_exp < -10 || a_exp > 10)
                    ? std::pow(10.0, a_exp) : s_pow10[a_exp + 10];
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

    /// \param a_const_exp can be either const or initial value of the exponent
    CONSTEXPR
    void normalize(int a_const_exp = 0) {
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
            if  (  diff == 0) return;
            for (; diff >  0; --diff) m_mant *= 10;
            for (; diff <  0; ++diff) m_mant /= 10;
            m_exp = a_const_exp;
        }
    }

private:

    /// Construct a decimal from a double.
    /// The problem this function solves is that due to imprecision of doubles
    /// they may look internally like this:  `1.00000000000000000000000001`, so
    /// some processing need to happen in order to represent it with a "sane"
    /// exponent.
    ///
    /// At return the decimal's mantissa and exponent satisfy the following:
    ///
    /// 1. |x - m * 10^(-e)| <= 0.5 * 10^(-4)
    /// 2. either x = m = 0 or (m % 10) != 0
    ///
    /// @throw runtime_exception if |x| >= 10^250.
    //
    void from_double(double x)
    {
        if (std::isnan(x)) {
            set_null();
            return;
        }

        int e2;
        int sign;

        if (x >= 0)
            sign = 1;
        else {
            x = -x;
            sign = -1;
        }
        auto md = frexp(x, &e2);
        auto n  = (int64_t)scalbln(md, 53);

        // At this point we have x = 2^(e2 - 53) * n (exactly, no rounding)
        int e;
        if (e2 < 49) { // n * (2^(e2 - 49) * 5^4 is not an integer
            int shift  = 49 - e2;
            if (shift >= 64) { // e.g. in the case of denormals
                clear();
                return;
            }

            n *= 625; // 625 = 5^4 < 2^10 and since n <= 2^53 no overflow here
            auto mask      = (1l << shift) - 1;
            auto remainder = n & mask;
            n >>= shift;
            if ((remainder << 1) >= mask)
                n++;

            if (n == 0) {
                clear();
                return;
            }
            e = 4;
        } else {
            // here e2 >= 49 and n > 0.
            // Since x = 2^(e2-49) * 5^4 * n * 10^-4 : no rounding in this case
            switch (e2) {
            case 49: if (n & 1) { m_exp=4; m_mant = 625*n; } n >>= 1; // no break
            case 50: if (n & 1) { m_exp=3; m_mant = 125*n; } n >>= 1; // no break
            case 51: if (n & 1) { m_exp=2; m_mant = 25 *n; } n >>= 1; // no break
            case 52: if (n & 1) { m_exp=1; m_mant = 5  *n; } n >>= 1; // no break
            case 53: e = 0; break;
            default: {
                e = 0; // in the next while we try to divide "n" by 5 instead
                        // of multiply it by 2 in order to avoid overflow in "n"
                do {
                    auto remainder  = (n % 5);
                    if  (remainder) {
                        int shift = e2 - 53;
                        if (n >= (1l << (63 - shift)))
                            UTXX_THROW_RUNTIME_ERROR
                            ("Overflow in double to decimal conversion: ", x);
                        m_exp  = e;
                        m_mant = sign * (n << shift);
                    }
                    --e;
                    n /= 5;
                }
                while (--e2 > 53);
            }
            }
        }

        // normalize "n"
        for (; (n % 10) == 0; --e, n /= 10);
        m_exp  = e;
        m_mant = sign * n;
    }
};

} // namespace utxx

#endif