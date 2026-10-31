/* mathfn.c - see mathfn.h.  No libm, no gint: bit tricks and short series. */

#include "mathfn.h"

#define BM_SQRT2 1.41421356237309504880
#define BM_INV_LN2 1.4426950408889634074
#define BM_LN10 2.30258509299404568402
#define BM_MIN_NORMAL 2.2250738585072014e-308

typedef union {
    double d;
    unsigned long long u;
} bits_t;

/* Splits a positive normal double into m in [1, 2) and x = m * 2^e. */
static bool split(double x, double *m, int *e)
{
    bits_t v;

    if (!bm_isfinite(x) || x <= 0.0)
        return false;
    v.d = x;
    *e = (int)((v.u >> 52) & 0x7FFu) - 1023;
    v.u = (v.u & 0x000FFFFFFFFFFFFFULL) | 0x3FF0000000000000ULL;
    *m = v.d;
    return true;
}

double bm_nan(void)
{
    return 0.0 / 0.0;
}

double bm_inf(void)
{
    bits_t v;

    v.u = 0x7FF0000000000000ULL;
    return v.d;
}

bool bm_isnan(double x)
{
    bits_t v;

    v.d = x;
    return ((v.u >> 52) & 0x7FFu) == 0x7FFu && (v.u & 0x000FFFFFFFFFFFFFULL);
}

bool bm_isfinite(double x)
{
    bits_t v;

    v.d = x;
    return ((v.u >> 52) & 0x7FFu) != 0x7FFu;
}

double bm_pow2(int n)
{
    bits_t v;

    if (n > 1023)
        return bm_inf();
    if (n < -1074)
        return 0.0;
    if (n >= -1022)
    {
        v.u = (unsigned long long)(n + 1023) << 52;
        return v.d;
    }
    v.u = 1ULL << (n + 1074);   /* subnormal */
    return v.d;
}

double bm_deg_to_rad(double deg)
{
    return deg * (BM_PI / 180.0);
}

double bm_rad_to_deg(double rad)
{
    return rad * (180.0 / BM_PI);
}

/* ---- square root -------------------------------------------------------- */

double bm_sqrt(double x)
{
    double m, g;
    int e, k, i;

    if (bm_isnan(x) || x < 0.0)
        return bm_nan();
    if (x == 0.0 || !bm_isfinite(x))
        return x;

    /* Subnormals have no exponent to halve: scale up by 2^54 and halve the
     * seed instead. */
    e = 0;
    if (x < BM_MIN_NORMAL)
    {
        x *= 18014398509481984.0;   /* 2^54 */
        e = -54;
    }
    if (!split(x, &m, &k))
        return bm_nan();
    e += k;
    if (e & 1)
    {
        m *= 2.0;
        e -= 1;
    }

    /* m is now in [1, 4): 1.5 is within a factor of 1.34 of the root, and each
     * Newton step doubles the correct digits. */
    g = 1.5;
    for (i = 0; i < 8; i++)
        g = 0.5 * (g + m / g);
    return g * bm_pow2(e / 2);
}

/* ---- exponential and logarithm ------------------------------------------ */

double bm_exp(double x)
{
    double r, term, sum;
    int k, i;

    if (bm_isnan(x))
        return x;
    if (x > 709.782712893384)
        return bm_inf();
    if (x < -745.1332191019411)
        return 0.0;

    k = (int)(x * BM_INV_LN2 + (x >= 0.0 ? 0.5 : -0.5));
    r = x - (double)k * BM_LN2;     /* |r| <= ln2 / 2 */
    term = 1.0;
    sum = 1.0;
    for (i = 1; i <= 18; i++)
    {
        term *= r / (double)i;
        sum += term;
    }
    /* Split the last power of two so exp(709.7) cannot round to infinity. */
    return (sum * bm_pow2(k - 1)) * 2.0;
}

double bm_ln(double x)
{
    double m, t, t2, s;
    int e, k, i;

    if (bm_isnan(x))
        return x;
    if (x < 0.0)
        return bm_nan();
    if (x == 0.0)
        return -bm_inf();
    if (!bm_isfinite(x))
        return bm_inf();

    e = 0;
    if (x < BM_MIN_NORMAL)
    {
        x *= 18014398509481984.0;   /* 2^54 */
        e = -54;
    }
    if (!split(x, &m, &k))
        return bm_nan();
    e += k;
    /* Fold to [1, sqrt 2) so the series argument is small. */
    if (m > BM_SQRT2)
    {
        m *= 0.5;
        e += 1;
    }

    /* ln m = 2 (t + t^3/3 + t^5/5 + ...), t = (m-1)/(m+1), |t| < 0.1716. */
    t = (m - 1.0) / (m + 1.0);
    t2 = t * t;
    s = 0.0;
    for (i = 10; i >= 1; i--)
        s = 1.0 / (double)(2 * i + 1) + t2 * s;
    return (double)e * BM_LN2 + 2.0 * t * (1.0 + t2 * s);
}

double bm_log10(double x)
{
    return bm_ln(x) / BM_LN10;
}

double bm_pow(double x, double y)
{
    if (bm_isnan(x) || bm_isnan(y))
        return bm_nan();
    if (y == 0.0)
        return 1.0;
    if (x == 0.0)
        return y > 0.0 ? 0.0 : bm_inf();

    /* A whole-number exponent stays exact and works for negative bases. */
    if (bm_isfinite(y) && y > -1e9 && y < 1e9 && y == (double)(long long)y)
    {
        long long n = (long long)(y < 0.0 ? -y : y);
        double b = x;
        double r = 1.0;

        while (n)
        {
            if (n & 1)
                r *= b;
            b *= b;
            n >>= 1;
        }
        return y < 0.0 ? 1.0 / r : r;
    }
    if (x < 0.0)
        return bm_nan();
    return bm_exp(y * bm_ln(x));
}

/* ---- trigonometry -------------------------------------------------------- */

/* sin and cos on |r| <= pi/4, by their Taylor series.  The series needs to
 * reach the 21st power for sin and the 20th for cos: at pi/4 the terms fall
 * slowly (r^14/14! is still 4e-13), and eleven of them put the result inside a
 * double's last two bits instead. */
static double sin_small(double r)
{
    double const x2 = r * r;
    double s = 1.0 / 51090942171709440000.0;   /* 1/21! */

    s = -1.0 / 121645100408832000.0 + x2 * s;  /* 1/19! */
    s = 1.0 / 355687428096000.0 + x2 * s;      /* 1/17! */
    s = -1.0 / 1307674368000.0 + x2 * s;       /* 1/15! */
    s = 1.0 / 6227020800.0 + x2 * s;           /* 1/13! */
    s = -1.0 / 39916800.0 + x2 * s;            /* 1/11! */
    s = 1.0 / 362880.0 + x2 * s;               /* 1/9!  */
    s = -1.0 / 5040.0 + x2 * s;                /* 1/7!  */
    s = 1.0 / 120.0 + x2 * s;                  /* 1/5!  */
    s = -1.0 / 6.0 + x2 * s;                   /* 1/3!  */
    s = 1.0 + x2 * s;
    return r * s;
}

static double cos_small(double r)
{
    double const x2 = r * r;
    double s = 1.0 / 2432902008176640000.0;    /* 1/20! */

    s = -1.0 / 6402373705728000.0 + x2 * s;    /* 1/18! */
    s = 1.0 / 20922789888000.0 + x2 * s;       /* 1/16! */
    s = -1.0 / 87178291200.0 + x2 * s;         /* 1/14! */
    s = 1.0 / 479001600.0 + x2 * s;            /* 1/12! */
    s = -1.0 / 3628800.0 + x2 * s;             /* 1/10! */
    s = 1.0 / 40320.0 + x2 * s;                /* 1/8!  */
    s = -1.0 / 720.0 + x2 * s;                 /* 1/6!  */
    s = 1.0 / 24.0 + x2 * s;                   /* 1/4!  */
    s = -0.5 + x2 * s;                         /* 1/2!  */
    s = 1.0 + x2 * s;
    return s;
}

void bm_sincos(double x, double *s, double *c)
{
    double r, sr, cr;
    int n, q;

    if (!bm_isfinite(x))
    {
        if (s)
            *s = bm_nan();
        if (c)
            *c = bm_nan();
        return;
    }
    /* Fold into [-pi/4, pi/4] and remember the quadrant. */
    n = (int)(x / BM_HALF_PI + (x >= 0.0 ? 0.5 : -0.5));
    r = x - (double)n * BM_HALF_PI;
    q = n & 3;
    if (q < 0)
        q += 4;
    sr = sin_small(r);
    cr = cos_small(r);

    switch (q)
    {
    case 0: *s = sr; *c = cr; break;
    case 1: *s = cr; *c = -sr; break;
    case 2: *s = -sr; *c = -cr; break;
    default: *s = -cr; *c = sr; break;
    }
}

double bm_sin(double x)
{
    double s, c;

    bm_sincos(x, &s, &c);
    return s;
}

double bm_cos(double x)
{
    double s, c;

    bm_sincos(x, &s, &c);
    return c;
}

double bm_tan(double x)
{
    double s, c;

    bm_sincos(x, &s, &c);
    if (c > -1e-13 && c < 1e-13)
        return bm_nan();        /* an odd multiple of pi/2 */
    return s / c;
}

/* ---- inverse trigonometry ------------------------------------------------ */

/* atan on |x| <= 1: halve the argument twice with
 * atan(x) = 2 atan(x / (1 + sqrt(1 + x^2))), then the series, which now needs
 * only ten terms for |x| <= tan(pi/16). */
static double atan_small(double x)
{
    double const x2 = x * x;
    double p = 1.0 / 21.0;      /* (-1)^10 x^21 / 21 */
    int k;

    for (k = 9; k >= 0; k--)
        p = ((k & 1) ? -1.0 : 1.0) / (double)(2 * k + 1) + x2 * p;
    return x * p;
}

double bm_atan(double x)
{
    bool neg = x < 0.0;
    bool inv = false;
    double r;

    if (bm_isnan(x))
        return x;
    if (!bm_isfinite(x))
        return neg ? -BM_HALF_PI : BM_HALF_PI;
    if (neg)
        x = -x;
    if (x > 1.0)
    {
        inv = true;
        x = 1.0 / x;
    }
    r = x / (1.0 + bm_sqrt(1.0 + x * x));
    r = r / (1.0 + bm_sqrt(1.0 + r * r));
    r = 4.0 * atan_small(r);
    if (inv)
        r = BM_HALF_PI - r;
    return neg ? -r : r;
}

double bm_asin(double x)
{
    if (bm_isnan(x) || x < -1.0 || x > 1.0)
        return bm_nan();
    if (x == 1.0)
        return BM_HALF_PI;
    if (x == -1.0)
        return -BM_HALF_PI;
    return bm_atan(x / bm_sqrt(1.0 - x * x));
}

double bm_acos(double x)
{
    if (bm_isnan(x) || x < -1.0 || x > 1.0)
        return bm_nan();
    return BM_HALF_PI - bm_asin(x);
}

/* ---- hyperbolics --------------------------------------------------------- */

double bm_sinh(double x)
{
    double const e = bm_exp(x);

    if (x > 710.0)
        return bm_inf();
    if (x < -710.0)
        return -bm_inf();
    return (e - 1.0 / e) * 0.5;
}

double bm_cosh(double x)
{
    double const e = bm_exp(x);

    if (x > 710.0 || x < -710.0)
        return bm_inf();
    return (e + 1.0 / e) * 0.5;
}

double bm_tanh(double x)
{
    if (x > 20.0)
        return 1.0;
    if (x < -20.0)
        return -1.0;
    {
        double const e = bm_exp(2.0 * x);

        return (e - 1.0) / (e + 1.0);
    }
}

/* ---- rounding ------------------------------------------------------------ */

#define BM_WHOLE_LIMIT 4.6116860184273879e18   /* 2^62 */

double bm_floor(double x)
{
    long long t;

    if (!bm_isfinite(x) || x >= BM_WHOLE_LIMIT || x <= -BM_WHOLE_LIMIT)
        return x;
    t = (long long)x;
    if ((double)t > x)
        t--;
    return (double)t;
}

double bm_ceil(double x)
{
    long long t;

    if (!bm_isfinite(x) || x >= BM_WHOLE_LIMIT || x <= -BM_WHOLE_LIMIT)
        return x;
    t = (long long)x;
    if ((double)t < x)
        t++;
    return (double)t;
}

double bm_atan2(double y, double x)
{
    if (x > 0.0)
        return bm_atan(y / x);
    if (x < 0.0)
        return y >= 0.0 ? bm_atan(y / x) + BM_PI : bm_atan(y / x) - BM_PI;
    /* x == 0: straight up or down, and (0, 0) has no angle. */
    if (y > 0.0)
        return BM_HALF_PI;
    if (y < 0.0)
        return -BM_HALF_PI;
    return 0.0;
}
