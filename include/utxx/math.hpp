//----------------------------------------------------------------------------
/// \file  math.hpp
//----------------------------------------------------------------------------
/// \brief This file contains some generic math functions.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-31
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
#ifndef _UTXX_MATH_HPP_
#define _UTXX_MATH_HPP_

#include <boost/assert.hpp>
#include <boost/cstdint.hpp>
#include <utxx/error.hpp>
#include <utxx/bits.hpp>

namespace utxx {
namespace math {

/// Calculate <tt>a</tt> raised to the power of <tt>b</tt>.
template <typename T>
T inline power(T a, size_t b) {
    if (a==0) return 0;
    if (b==0) return 1;
    if (b==1) return a;
    return (b & 1) == 0 ? power(a*a, b >> 1) : a*power(a*a, b >> 1);
}

/// Calculate logarithm of \a n using \a base.
/// If n is 0 or is less than base, the function returns 0.
int inline log(unsigned long n, uint8_t base = 2) {
    // return n <= 1 ? 0 : 1+log(n/base, base);   // This won't inline
    if (n <= 1) return 0;
    int k;
    for (k = 0; n > 1; k++, n /= base);
    return k;
}

/// Calculate logarithm of \a n using \a base.
/// If n is 0 or is less than base, the function returns 0.
int inline log2(unsigned long n) {
    BOOST_ASSERT(!(n & (n-1)));
    return bits::bit_scan_forward(n);
}

/// Calculate logarithm of \a n using \a base.
/// If n is 0 or is less than base, the function returns 0.
template <uint8_t Base>
int inline upper_log(size_t n) {
    int k = log(n, Base);
    return power(k, Base) == n ? k : k+1;
}

int inline upper_log2(size_t n) {
    return (n & (n-1)) == 0 ? log2(n) : log(n,2)+1;
}

/// Returns power of \a base equal or greater than number \a n.
template <typename T>
T inline upper_power(T n, size_t base) {
    int m = log(n, base);
    T   r = power(base, m);
    return r == n ? n : r*base;
}

} // namespace math
} // namespace utxx

#endif // _UTXX_MATH_HPP_

