/* cplx.c - see cplx.h. */

#include "cplx.h"

#include <stdio.h>
#include <string.h>

#include "mathfn.h"
#include "numfmt.h"

cplx_t cx(double re, double im)
{
    cplx_t z = { re, im };

    return z;
}

cplx_t cplx_add(cplx_t a, cplx_t b)
{
    return cx(a.re + b.re, a.im + b.im);
}

cplx_t cplx_sub(cplx_t a, cplx_t b)
{
    return cx(a.re - b.re, a.im - b.im);
}

cplx_t cplx_mul(cplx_t a, cplx_t b)
{
    return cx(a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re);
}

cplx_t cplx_div(cplx_t a, cplx_t b)
{
    double const d = b.re * b.re + b.im * b.im;

    if (d == 0.0)
        return cx(bm_nan(), bm_nan());
    return cx((a.re * b.re + a.im * b.im) / d, (a.im * b.re - a.re * b.im) / d);
}

cplx_t cplx_conj(cplx_t a)
{
    return cx(a.re, -a.im);
}

cplx_t cplx_powi(cplx_t a, int n)
{
    cplx_t r = cx(1.0, 0.0);
    cplx_t base = a;
    int e = n < 0 ? -n : n;

    if (e > 64)
        return cx(bm_nan(), bm_nan());
    for (int i = 0; i < e; i++)
        r = cplx_mul(r, base);
    if (n < 0)
        r = cplx_div(cx(1.0, 0.0), r);
    return r;
}

double cplx_abs(cplx_t a)
{
    return bm_sqrt(a.re * a.re + a.im * a.im);
}

double cplx_arg(cplx_t a)
{
    return bm_atan2(a.im, a.re);
}

cplx_t cplx_polar(double r, double theta)
{
    return cx(r * bm_cos(theta), r * bm_sin(theta));
}

bool cplx_is_nan(cplx_t a)
{
    return !bm_isfinite(a.re) || !bm_isfinite(a.im);
}

/* One part, without its sign; the sign is written by the caller.  Trailing
 * zeros go: a result of 1.5 is "1.5", not "1.50", because the digits after it
 * are an artefact of the rounding, not information. */
static void part(double v, int sig, char *buf, int size)
{
    int n;

    if (numfmt_format_sig(v, sig, buf, size) < 0)
    {
        strcpy(buf, "?");
        return;
    }
    n = (int)strlen(buf);
    if (strchr(buf, '.'))
    {
        while (n > 1 && buf[n - 1] == '0')
            buf[--n] = 0;
        if (n > 1 && buf[n - 1] == '.')
            buf[--n] = 0;
    }
}

int cplx_format(cplx_t a, int sig, char *buf, int size)
{
    char re[32];
    char im[32];

    if (cplx_is_nan(a))
        return snprintf(buf, (size_t)size, "not a number");

    part(a.re, sig, re, (int)sizeof re);
    part(a.im < 0.0 ? -a.im : a.im, sig, im, (int)sizeof im);

    /* A zero real part leaves just the imaginary term; a zero imaginary part
     * leaves just the real one. */
    if (a.im == 0.0)
        return snprintf(buf, (size_t)size, "%s", re);
    if (a.re == 0.0)
    {
        if (a.im == 1.0 || a.im == -1.0)
            return snprintf(buf, (size_t)size, "%si", a.im < 0.0 ? "-" : "");
        return snprintf(buf, (size_t)size, "%s%si", a.im < 0.0 ? "-" : "", im);
    }
    if (a.im == 1.0 || a.im == -1.0)
        return snprintf(buf, (size_t)size, "%s %c i", re, a.im < 0.0 ? '-' : '+');
    return snprintf(buf, (size_t)size, "%s %c %si", re, a.im < 0.0 ? '-' : '+',
                    im);
}
