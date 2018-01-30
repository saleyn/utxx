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
#pragma once

#include <cmath>
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

/// Calculate logarithm of \a n using \a base, such that
/// the Base to the power of the returned value is greater or equal to \a n.
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

/// Calculate greatest common denominator of x and y.
/// E.g. gcd(18, 4) = 2
inline long gcd(long x, long y) {
    if (x == 0)
        return y;

    while (y != 0) {
        if (x > y) x = x - y;
        else       y = y - x;
    }

    return x;
}

/// Calculate least common multiple of x and y.
/// E.g. gcd(18, 4) = 36
inline long lcm(long x, long y) { return (x*y)/gcd(x, y); }

/*
/// Check if a number is prime
inline bool is_prime(long x) {
    // 0, 1 are not prime numbers, but 2, 3 are:
    if (x <= 3)
        return x > 1;
    // Even numbers and those divisible by 3 are not primes.
    if ((x & 1) == 0 || (x % 3) == 0)
        return false;
    // Note that all remaining primes are in the form of:
    //  (6*k - 1) or (6*k + 1):  5, 7
    // Iterate through all numbers checking that they are not
    // divisible by the prime numbers in the form stated above:
    for (long i = 5, n = std::sqrt(x); i <= n; i += 6)
        if ((x % i) == 0 || x % (i+2) == 0)
            return false;
    return true;
}
*/

namespace {
    bool div_by(long x, int y) { return x % y == 0; }

    long sqrt_helper(long x)
    {
        if (x < 0) return 0;

        // Use sqrt whenever possible, however the largest int value a double
        // can hold is 1 << 53. So use sqrt() only for smaller ints:
        if (x <= 1l << 53)
            return long(std::sqrt(x));

        // Approximate sqrt that is >= the actual square root.
        // Any 64 bit number can be expressed in terms of x and y as follows:

        // sqrt(n) = sqrt(x) * sqrt(2^32) + z, where
        //   z - some value to account for the "y" portion of the equation
        //   (x+1) << 32 is always greater than (x<<32) | y, hence get sqrt(x+1)
        //   and multiply the result by 2^16 giving a slightly greater sqrt:
        auto   ux    = size_t(x);
        size_t shift = 0;

        for(; ux >= (1ul << 32); ux >>= 32, shift += 32);

        return long(ceil(sqrt((double)(ux + 1)))) << (shift / 2);
    }
}

/// Check if a number is prime
inline bool is_prime(long x)
{
    if (x <= 1)
        return false;

    if (x < 4 || x == 5 || x == 7)
        return true;

    if ((x&1) == 0 || div_by(x, 3) || div_by(x, 5) || div_by(x, 7))
        return false;

    // The remaining primes are in the form 6*k-1 and 6*k+1:
    //   11,13,17,19,23..29,31,..37,41,43...
    // Note that from these we need to eliminate those divisible by 2,3,5.
    // The pattern of divisibility by 3 or 5 repeats from 15 every 30.

    //   03|05|07|09|11|13|15|17|19|21|23|25|27|29|31|33|35|37|39|41|43
    // 0        0     4  6    10 12    16       22 24       30
    // 3        x        x        x        x        x        x        x
    // 5        x              x              x              x

    // NOTE: for end value use +1 to compensate for round-off errors
    for(long d=7, e=sqrt_helper(x)+1; d <= e; d += 30)
        if (div_by(x,d)    || div_by(x,d+4)  || div_by(x,d+6)  || div_by(x,d+10) ||
            div_by(x,d+12) || div_by(x,d+16) || div_by(x,d+22) || div_by(x,d+24))
            return false;
    return true;
}

} // namespace math
} // namespace utxx
