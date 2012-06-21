#include <util/convert.hpp>

namespace util {

//--------------------------------------------------------------------------------
// Parses floating point numbers with fixed number of decimal digits from string.
//
// atof:
//      09-May-2009 Tom Van Baak (tvb) www.LeapSecond.com
//      http://www.leapsecond.com/tools/fast_atof.c
//      Contributor: 2010-10-15 Serge Aleynikov
//--------------------------------------------------------------------------------
double atof(const char* p, const char* end)
{
    // Skip leading white space, if any.
    while (p < end && (*p == ' ' || *p == '0'))
        ++p;

    // Get sign, if any.

    double sign = 1.0;

    if (*p == '-') {
        sign = -1.0;
        ++p;
    } else if (*p == '+') {
        ++p;
    }

    // Get digits before decimal point or exponent, if any.

    double value = 0.0;
    uint8_t n = *p - '0';
    while (p < end && n < 10u) {
        value = value * 10.0 + n;
        n = *(++p) - '0';
    }

    // Get digits after decimal point, if any.

    if (*p == '.') {
        ++p;

        double pow10 = 1.0;
        double acc   = 0.0;
        n = *p - '0';
        while (p < end && n < 10u) {
            acc = acc * 10.0 + n;
            n = *(++p) - '0';
            pow10 *= 10.0;
        }
        value += acc / pow10;
    }

    // Handle exponent, if any.
    /*
    double scale = 1.0;
    if ((*p == 'e') || (*p == 'E')) {
        unsigned int expon;
        p += 1;

        // Get sign of exponent, if any.

        frac = 0;
        if (*p == '-') {
            frac = 1;
            p += 1;

        } else if (*p == '+') {
            p += 1;
        }

        // Get digits of exponent, if any.

        expon = 0;
        while (valid_digit(*p)) {
            expon = expon * 10 + (*p - '0');
            p += 1;
        }
        if (expon > 308) expon = 308;

        // Calculate scaling factor.

        while (expon >= 50) { scale *= 1E50; expon -= 50; }
        while (expon >=  8) { scale *= 1E8;  expon -=  8; }
        while (expon >   0) { scale *= 10.0; expon -=  1; }
    }

    // Return signed and scaled floating point result.

    return sign * (frac ? (value / scale) : (value * scale));
    */
    return sign * value;
}

/* For internal use by sys_double_to_chars_fast() */
static char* float_first_trailing_zero(char* p)
{
    for (--p; *p == '0' && *(p-1) == '0'; --p);
    if (*(p-1) == '.') ++p;
    return p;
}

int ftoa_fast(double f, char *outbuf, int maxlen, int decimals, bool compact)
{
    enum {
          FRAC_SIZE  = 52
        , EXP_SIZE   = 11
        , EXP_MASK   = (1ll << EXP_SIZE) - 1
        , FRAC_MASK  = (1ll << FRAC_SIZE) - 1
        , FRAC_MASK2 = (1ll << (FRAC_SIZE + 1)) - 1
        , MAX_FLOAT  = 1ll << (FRAC_SIZE+1)
    };

    long long mantissa, int_part, int_part2, frac_part;
    short exp;
    int sign, i, n, m, max;
    double absf;
    union { long long L; double F; } x;
    char c, *p = outbuf;
    int digit, roundup;

    x.F = f;

    exp      = (x.L >> FRAC_SIZE) & EXP_MASK;
    mantissa = x.L & FRAC_MASK;
    sign     = x.L >= 0 ? 1 : -1;
    if (exp == EXP_MASK) {
        if (mantissa == 0) {
            if (sign == -1)
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
        return p - outbuf;
    }

    exp      -= EXP_MASK >> 1;
    mantissa |= (1ll << FRAC_SIZE);
    frac_part = 0;
    int_part  = 0;
    absf      = f * sign;

    /* Don't bother with optimizing too large numbers and decimals */
    if (absf > MAX_FLOAT || decimals > maxlen-17) {
        int len = snprintf(outbuf, maxlen-1, "%.*f", decimals, f);
        p = outbuf + len;
        /* Delete trailing zeroes */
        if (compact)
            p = float_first_trailing_zero(outbuf + len);
        *p = '\0';
        return p - outbuf;
    }

    if (exp >= FRAC_SIZE)
        int_part  = mantissa << (exp - FRAC_SIZE);
    else if (exp >= 0) {
        int_part  = mantissa >> (FRAC_SIZE - exp);
        frac_part = (mantissa << (exp + 1)) & FRAC_MASK2;
    }
    else /* if (exp < 0) */
        frac_part = (mantissa & FRAC_MASK2) >> -(exp + 1);

    if (int_part == 0) {
        if (sign == -1)
            *p++ = '-';
        *p++ = '0';
    } else {
        int ret;
        while (int_part != 0) {
            int_part2 = int_part / 10;
            *p++ = (char)(int_part - ((int_part2 << 3) + (int_part2 << 1)) + '0');
            int_part = int_part2;
        }
        if (sign == -1)
            *p++ = '-';
        /* Reverse string */
        ret = p - outbuf;
        for (i = 0, n = ret/2; i < n; i++) {
            int j = ret - i - 1;
            c = outbuf[i];
            outbuf[i] = outbuf[j];
            outbuf[j] = c;
        }
    }
    if (decimals != 0)
        *p++ = '.';

    max = maxlen - (p - outbuf) - 1 /* leave room for trailing '\0' */;
    if (max > decimals)
        max = decimals;
    for (m = 0; m < max; m++) {
        /* frac_part *= 10; */
        frac_part = (frac_part << 3) + (frac_part << 1);

        *p++ = (char)((frac_part >> (FRAC_SIZE + 1)) + '0');
        frac_part &= FRAC_MASK2;
    }

    roundup = 0;
    /* Rounding - look at the next digit */
    frac_part = (frac_part << 3) + (frac_part << 1);
    digit = (frac_part >> (FRAC_SIZE + 1));
    if (digit > 5)
        roundup = 1;
    else if (digit == 5) {
        frac_part &= FRAC_MASK2;
        if (frac_part != 0) roundup = 1;
    }
    if (roundup) {
        char d;
        int pos = p - outbuf - 1;
        do {
            d = outbuf[pos];
            if (d == '-') break;
            if (d == '.') continue;
            if (++d != ':') {
                outbuf[pos] = d;
                break;
            }
            outbuf[pos] = '0';
        } while (--pos);
    }

    /* Delete trailing zeroes */
    if (compact)
        p = float_first_trailing_zero(--p);
    *p = '\0';
    return p - outbuf;
}

} // namespace util
