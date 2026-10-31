/* numparse.c - see numparse.h. */

#include "numparse.h"

#include <stdint.h>

/* 10^n for n = 0..22, the exact ones; beyond that the value is not exact and
 * the range check below has already rejected it. */
static double pow10_int(int n)
{
    static double const tab[] = {
        1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
        1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22,
    };

    if (n < 0)
        return 0.0;
    if (n > 22)
        return 1e308;            /* clamped: the caller checks the magnitude */
    return tab[n];
}

int numparse_double(char const *text, int len, double *out)
{
    int i = 0;
    int ndigits = 0;
    int decimals = 0;
    int exponent = 0;
    bool neg = false;
    bool seen_digit = false;
    bool after_point = false;
    uint64_t mantissa = 0;

    if (!text || !out || len <= 0)
        return NUMPARSE_ERR_EMPTY;

    while (i < len && (text[i] == ' ' || text[i] == '\t'))
        i++;
    if (i < len && (text[i] == '-' || text[i] == '+'))
    {
        neg = text[i] == '-';
        i++;
    }

    for (; i < len; i++)
    {
        char c = text[i];

        if (c >= '0' && c <= '9')
        {
            if (++ndigits > 18)
                return NUMPARSE_ERR_RANGE;
            /* Accumulate only while it fits; extra low digits are dropped and
             * counted by ndigits so precision stays sane. */
            if (mantissa < 100000000000000000ull)
            {
                mantissa = mantissa * 10u + (uint64_t)(c - '0');
                if (after_point)
                    decimals++;
            }
            seen_digit = true;
            continue;
        }
        if (c == '.')
        {
            if (after_point)
                return NUMPARSE_ERR_SYNTAX;
            after_point = true;
            continue;
        }
        break;
    }

    if (!seen_digit)
        return i < len ? NUMPARSE_ERR_SYNTAX : NUMPARSE_ERR_EMPTY;

    if (i < len && (text[i] == 'e' || text[i] == 'E'))
    {
        bool eneg = false;
        int edigits = 0;
        int ev = 0;

        i++;
        if (i < len && (text[i] == '-' || text[i] == '+'))
        {
            eneg = text[i] == '-';
            i++;
        }
        for (; i < len && text[i] >= '0' && text[i] <= '9'; i++)
        {
            if (ev < 4000)
                ev = ev * 10 + (text[i] - '0');
            edigits++;
        }
        if (edigits == 0)
            return NUMPARSE_ERR_SYNTAX;
        exponent = eneg ? -ev : ev;
    }

    while (i < len && (text[i] == ' ' || text[i] == '\t'))
        i++;
    if (i != len)
        return NUMPARSE_ERR_SYNTAX;

    /* value = mantissa * 10^(exponent - decimals) */
    exponent -= decimals;
    {
        double value = (double)mantissa;

        if (exponent > 0)
        {
            if (exponent > 308)
                return NUMPARSE_ERR_RANGE;
            while (exponent > 22)
            {
                value *= 1e22;
                exponent -= 22;
            }
            value *= pow10_int(exponent);
        }
        else if (exponent < 0)
        {
            if (exponent < -340)
                value = 0.0;
            while (exponent < -22)
            {
                value /= 1e22;
                exponent += 22;
            }
            if (exponent < 0)
                value /= pow10_int(-exponent);
        }
        if (neg)
            value = -value;
        /* No isinf() here: the build may not link libm, so a magnitude check
         * does the same job for the values this parser can produce. */
        if (value > 1.7e308 || value < -1.7e308)
            return NUMPARSE_ERR_RANGE;
        *out = value;
    }
    return NUMPARSE_OK;
}

int numparse_cstr(char const *text, double *out)
{
    int len = 0;

    if (!text)
        return NUMPARSE_ERR_EMPTY;
    while (text[len])
        len++;
    return numparse_double(text, len, out);
}

bool numparse_ok(char const *text, int len)
{
    double v;

    return numparse_double(text, len, &v) == NUMPARSE_OK;
}
