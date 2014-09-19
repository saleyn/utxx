//----------------------------------------------------------------------------
/// \file  convert.cpp
//----------------------------------------------------------------------------
/// \brief Implementation of conversion functions.
//----------------------------------------------------------------------------
// Copyright (c) 2011 Serge Aleynikov <saleyn@gmail.com>
// Created: 2011-09-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the utxx project.

Copyright (C) 2011 Serge Aleynikov <saleyn@gmail.com>

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

#include <utxx/convert.hpp>
#include <utxx/compiler_hints.hpp>
#include <stdexcept>

namespace utxx {
/* For internal use by ftoa_fast() */
static inline char* find_first_trailing_zero(char* p)
{
    for (; *(p-1) == '0'; --p);
    if (*(p-1) == '.') ++p;
    return p;
}

static const double cs_rnd = 0.55555555555555555;
static const double cs_pow10_rnd[] = {
    cs_rnd / 1ll, cs_rnd / 10ll, cs_rnd / 100ll, cs_rnd / 1000ll,
    cs_rnd / 10000ll, cs_rnd / 100000ll, cs_rnd / 1000000ll,
    cs_rnd / 10000000ll, cs_rnd / 100000000ll, cs_rnd / 1000000000ll,
    cs_rnd / 10000000000ll, cs_rnd / 100000000000ll,
    cs_rnd / 1000000000000ll, cs_rnd / 10000000000000ll,
    cs_rnd / 100000000000000ll, cs_rnd / 1000000000000000ll,
    cs_rnd / 10000000000000000ll, cs_rnd / 100000000000000000ll,
    cs_rnd / 1000000000000000000ll
};

namespace {
    enum {
          FRAC_SIZE     = 52
        , EXP_SIZE      = 11
        , EXP_MASK      = (1ll << EXP_SIZE) - 1
        , FRAC_MASK     = (1ll << FRAC_SIZE) - 1
        , FRAC_MASK2    = (1ll << (FRAC_SIZE + 1)) - 1
        , MAX_FLOAT     = 1ll << (FRAC_SIZE+1)
        , MAX_DECIMALS  = sizeof(cs_pow10_rnd) / sizeof(cs_pow10_rnd[0])
    };
}

/// @param width is the width of the buffer to fill (there has to be enough room)
void ftoa_right(double f, char *buffer, int width, int precision, char lpad)
    throw (std::invalid_argument)
{
    bool neg;
    double fr;
    union { long long L; double F; } x;
    int  int_width = width - (precision ? precision+1 : 0);
    if (unlikely(precision < 0 || int_width < 0))
        throw std::invalid_argument("Incorrect width or precision");

    /* Round the number to given decimal places. The number of 5's in the
     * constant 0.55... is chosen such that adding any more 5's doesn't
     * change the double precision of the number, i.e.:
     * 1> term_to_binary(0.55555555555555555, [{minor_version, 1}]).
     * <<131,70,63,225,199,28,113,199,28,114>>
     * 2> term_to_binary(0.555555555555555555, [{minor_version, 1}]).
     * <<131,70,63,225,199,28,113,199,28,114>>
     */
    if (f >= 0) {
        neg = false;
        fr  = precision < MAX_DECIMALS ? (f + cs_pow10_rnd[precision]) : f;
        x.F = fr;
    } else {
        neg = true;
        fr  = precision < MAX_DECIMALS ? (f - cs_pow10_rnd[precision]) : f;
        x.F = -fr;
    }

    short exp = (x.L >> FRAC_SIZE) & EXP_MASK;
    int64_t mantissa  = x.L & FRAC_MASK;

    if (unlikely(exp == EXP_MASK)) {
        char* end = buffer + width - (neg ? 4 : 3);
        if (unlikely(end < buffer))
            throw std::invalid_argument("Insufficient width");

        for (char* q = buffer; q < end; *q++ = lpad);
        if (mantissa == 0) {
            if (neg)
                *end++ = '-';
            *end++ = 'i';
            *end++ = 'n';
            *end++ = 'f';
        } else {
            *end++ = 'n';
            *end++ = 'a';
            *end++ = 'n';
        }
        return;
    }

    exp      -= EXP_MASK >> 1;
    mantissa |= (1ll << FRAC_SIZE);
    int64_t int_part = 0, frac_part = 0;

    /* Don't bother with optimizing too large numbers or too large precision */
    if (unlikely(x.F > MAX_FLOAT || precision >= MAX_DECIMALS)) {
        int len = snprintf(buffer, width+1, "%*.*f", width, precision, f);
        if (len >= width)
            throw std::invalid_argument("Incorrect width or precision");
        return;
    } else if (exp >= FRAC_SIZE) {
        int_part  = mantissa << (exp - FRAC_SIZE);
    } else if (exp >= 0) {
        int_part  = mantissa >> (FRAC_SIZE - exp);
        frac_part = (mantissa << (exp + 1)) & FRAC_MASK2;
    } else /* if (exp < 0) */ {
        frac_part = (mantissa & FRAC_MASK2) >> -(exp + 1);
    }

    char* p = buffer + int_width;

    // Write fractional part
    if (precision) {
        char* q = p;
        *q++ = '.';

        for (int i = 0; i < precision; i++) {
            frac_part = (frac_part << 3) + (frac_part << 1);  /* equiv. *= 10; */
            *q++      = (char)((frac_part >> (FRAC_SIZE + 1)) + '0');
            frac_part &= FRAC_MASK2;
        }
    }

    // Write decimal part
    if (!int_part) {
        *(--p) = '0';
        if (neg)
            *(--p) = '-';
    } else {
        char* end = p;
        while (int_part) {
            int j = int_part / 10;
            *(--p) = (char)(int_part - ((j << 3) + (j << 1)) + '0');
            int_part = j;
        }
        /* Reverse string */
        int wid = end - p;
        for (int i = 0, n = wid >> 1; i < n; i++) {
            int64_t j = wid - i - 1;
            char    c = buffer[i];
            buffer[i] = buffer[j];
            buffer[j] = c;
        }
        if (neg)
            *(--p) = '-';
    }
    if (unlikely(p < buffer))
        throw std::invalid_argument("Insufficient width");
    // Left-pad
    for (char* q = buffer; q != p; *q++ = lpad);
}

// width is a signed value: negative for left adjustment
int ftoa_left(double f, char *buffer, int buffer_size, int precision, bool compact)
{
    bool neg;
    double fr;
    union { long long L; double F; } x;
    char *p = buffer;

    if (unlikely(precision < 0))
        return -1;

    /* Round the number to given decimal places. The number of 5's in the
     * constant 0.55... is chosen such that adding any more 5's doesn't
     * change the double precision of the number, i.e.:
     * 1> term_to_binary(0.55555555555555555, [{minor_version, 1}]).
     * <<131,70,63,225,199,28,113,199,28,114>>
     * 2> term_to_binary(0.555555555555555555, [{minor_version, 1}]).
     * <<131,70,63,225,199,28,113,199,28,114>>
     */
    if (f >= 0) {
        neg = false;
        fr  = precision < MAX_DECIMALS ? (f + cs_pow10_rnd[precision]) : f;
        x.F = fr;
    } else {
        neg = true;
        fr  = precision < MAX_DECIMALS ? (f - cs_pow10_rnd[precision]) : f;
        x.F = -fr;
    }

    short exp = (x.L >> FRAC_SIZE) & EXP_MASK;
    int64_t mantissa  = x.L & FRAC_MASK;

    if (unlikely(exp == EXP_MASK)) {
        if (mantissa == 0) {
            if (neg)
                *p++ = '-';
            *p++ = 'i';
            *p++ = 'n';
            *p++ = 'f';
        } else {
            *p++ = 'n';
            *p++ = 'a';
            *p++ = 'n';
        }
        *p = '\0';
        return p - buffer;
    }

    exp      -= EXP_MASK >> 1;
    mantissa |= (1ll << FRAC_SIZE);
    int64_t int_part = 0, frac_part = 0;

    /* Don't bother with optimizing too large numbers or too large precision */
    if (unlikely(x.F > MAX_FLOAT || precision >= MAX_DECIMALS)) {
        int len = snprintf(buffer, buffer_size, "%.*f", precision, f);
        char* p = buffer + len;
        if (len >= buffer_size)
            return -1;
        /* Delete trailing zeroes */
        if (compact)
            p = find_first_trailing_zero(p);
        *p = '\0';
        return p - buffer;
    } else if (exp >= FRAC_SIZE) {
        int_part  = mantissa << (exp - FRAC_SIZE);
    } else if (exp >= 0) {
        int_part  = mantissa >> (FRAC_SIZE - exp);
        frac_part = (mantissa << (exp + 1)) & FRAC_MASK2;
    } else /* if (exp < 0) */ {
        frac_part = (mantissa & FRAC_MASK2) >> -(exp + 1);
    }

    if (!int_part) {
        if (neg)
            *p++ = '-';
        *p++ = '0';
    } else {
        while (int_part != 0) {
            int j = int_part / 10;
            *p++ = (char)(int_part - ((j << 3) + (j << 1)) + '0');
            int_part = j;
        }
        if (neg)
            *p++ = '-';
        /* Reverse string */
        int ret = p - buffer;
        for (int i = 0, n = ret >> 1; i < n; i++) {
            int64_t j = ret - i - 1;
            char    c = buffer[i];
            buffer[i] = buffer[j];
            buffer[j] = c;
        }
    }

    if (precision > 0) {
        *p++ = '.';

        int max = buffer_size - (p - buffer) - 1 /* leave room for trailing '\0' */;

        if (precision > max)
            return -1;  /* the number is not large enough to fit in the buffer */

        max = precision;

        for (int i = 0; i < max; i++) {
            frac_part = (frac_part << 3) + (frac_part << 1);  /* equiv. *= 10; */
            *p++      = (char)((frac_part >> (FRAC_SIZE + 1)) + '0');
            frac_part &= FRAC_MASK2;
        }

        /* Delete trailing zeroes */
        if (compact)
            p = find_first_trailing_zero(p);
    }
    *p = '\0';
    return p - buffer;
}

} // namespace utxx
