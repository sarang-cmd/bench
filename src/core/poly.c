/* poly.c - see poly.h. */

#include "poly.h"

#include <stdbool.h>
#include <string.h>

#include "mathfn.h"
#include "numfmt.h"
#include "numparse.h"

/* Trim trailing zero coefficients so deg is the real degree. */
static void trim(poly_t *p)
{
    int d = p->deg;

    while (d > 0 && p->c[d] == 0.0)
        d--;
    p->deg = d;
    if (d == 0 && p->c[0] == 0.0)
        p->deg = 0;
}

void poly_zero(poly_t *p)
{
    p->deg = 0;
    for (int i = 0; i < POLY_COEFFS; i++)
        p->c[i] = 0.0;
}

void poly_set(poly_t *p, int deg, double const *c)
{
    poly_zero(p);
    if (deg < 0)
        deg = 0;
    if (deg > POLY_MAX_DEG)
        deg = POLY_MAX_DEG;
    for (int i = 0; i <= deg; i++)
        p->c[i] = c[i];
    p->deg = deg;
    trim(p);
}

double poly_eval(poly_t const *p, double x)
{
    double v = 0.0;

    for (int i = p->deg; i >= 0; i--)
        v = v * x + p->c[i];
    return v;
}

int poly_parse(char const *text, poly_t *out)
{
    int sign = 1;
    int i = 0;
    bool any = false;

    poly_zero(out);
    if (!text)
        return POLY_ERR_SYNTAX;

    for (;;)
    {
        double coef = 0.0;
        int power = 0;
        bool have_coef = false;

        while (text[i] == ' ')
            i++;
        if (!text[i])
            break;

        if (text[i] == '+' || text[i] == '-')
        {
            sign = text[i] == '-' ? -1 : 1;
            i++;
            while (text[i] == ' ')
                i++;
        }
        else if (any)
            return POLY_ERR_SYNTAX;      /* a term must start with a sign */

        /* The coefficient, if the term starts with a digit or a point. */
        if ((text[i] >= '0' && text[i] <= '9') || text[i] == '.')
        {
            int start = i;
            double v = 0.0;

            while ((text[i] >= '0' && text[i] <= '9') || text[i] == '.')
                i++;
            if (numparse_double(text + start, i - start, &v) != NUMPARSE_OK)
                return POLY_ERR_SYNTAX;
            coef = v;
            have_coef = true;
            while (text[i] == ' ')
                i++;
        }

        if (text[i] == 'x' || text[i] == 'X')
        {
            i++;
            power = 1;
            if (text[i] == '^')
            {
                int start;

                i++;
                start = i;
                power = 0;
                while (text[i] >= '0' && text[i] <= '9')
                    power = power * 10 + (text[i++] - '0');
                if (i == start || power > POLY_MAX_DEG)
                    return power > POLY_MAX_DEG ? POLY_ERR_DEGREE
                                                : POLY_ERR_SYNTAX;
            }
            if (!have_coef)
                coef = 1.0;
        }
        else if (!have_coef)
            return POLY_ERR_SYNTAX;

        out->c[power] += (double)sign * coef;
        if (power > out->deg)
            out->deg = power;
        any = true;
        sign = 1;
    }

    if (!any)
        return POLY_ERR_SYNTAX;
    trim(out);
    return POLY_OK;
}

/* Appends `src` to the buffer at `n`; returns the new length, or `size` when
 * it did not fit. */
static int append(char *buf, int n, int size, char const *src)
{
    while (*src)
    {
        if (n + 1 >= size)
            return size;
        buf[n++] = *src++;
    }
    buf[n] = 0;
    return n;
}

int poly_format(poly_t const *p, char *buf, int size)
{
    int n = 0;
    bool wrote = false;

    if (size < 6)
        return POLY_ERR_SIZE;
    buf[0] = 0;

    for (int i = p->deg; i >= 0; i--)
    {
        double v = p->c[i];
        char num[24];
        bool const unit = i > 0 && (v == 1.0 || v == -1.0);

        if (v == 0.0)
            continue;
        if (wrote)
            n = append(buf, n, size, v < 0 ? " - " : " + ");
        else if (v < 0)
            n = append(buf, n, size, "-");
        wrote = true;

        if (!unit)
        {
            numfmt_dec_t d;

            if (numfmt_dec_from_double(v < 0 ? -v : v, NUMFMT_DEFAULT_SIG, &d)
                    != NUMFMT_OK ||
                numfmt_dec_render(&d, 0, -1, num, (int)sizeof num) < 0)
                return POLY_ERR_SIZE;
            n = append(buf, n, size, num);
        }
        if (i > 0)
        {
            char pw[8];
            int k = 0;

            n = append(buf, n, size, "x");
            if (i > 1)
            {
                pw[k++] = '^';
                if (i >= 10)
                    pw[k++] = (char)('0' + i / 10);
                pw[k++] = (char)('0' + i % 10);
                pw[k] = 0;
                n = append(buf, n, size, pw);
            }
        }
        if (n >= size)
            return POLY_ERR_SIZE;
    }
    if (!wrote)
        return append(buf, 0, size, "0");
    return n;
}

void poly_add(poly_t const *a, poly_t const *b, poly_t *out)
{
    int const d = a->deg > b->deg ? a->deg : b->deg;

    poly_zero(out);
    for (int i = 0; i <= d; i++)
        out->c[i] = (i <= a->deg ? a->c[i] : 0.0) + (i <= b->deg ? b->c[i] : 0.0);
    out->deg = d;
    trim(out);
}

void poly_sub(poly_t const *a, poly_t const *b, poly_t *out)
{
    int const d = a->deg > b->deg ? a->deg : b->deg;

    poly_zero(out);
    for (int i = 0; i <= d; i++)
        out->c[i] = (i <= a->deg ? a->c[i] : 0.0) - (i <= b->deg ? b->c[i] : 0.0);
    out->deg = d;
    trim(out);
}

int poly_mul(poly_t const *a, poly_t const *b, poly_t *out)
{
    if (a->deg + b->deg > POLY_MAX_DEG)
        return POLY_ERR_DEGREE;
    poly_zero(out);
    for (int i = 0; i <= a->deg; i++)
        for (int j = 0; j <= b->deg; j++)
            out->c[i + j] += a->c[i] * b->c[j];
    out->deg = a->deg + b->deg;
    trim(out);
    return POLY_OK;
}

int poly_binomial(double a, double b, int n, poly_t *out)
{
    double binom = 1.0;

    if (n < 0 || n > POLY_MAX_DEG)
        return POLY_ERR_DEGREE;
    poly_zero(out);
    /* (ax + b)^n = sum C(n,k) (ax)^k b^(n-k) */
    for (int k = 0; k <= n; k++)
    {
        double term = binom;

        for (int i = 0; i < k; i++)
            term *= a;
        for (int i = 0; i < n - k; i++)
            term *= b;
        out->c[k] = term;
        binom = binom * (double)(n - k) / (double)(k + 1);
    }
    out->deg = n;
    trim(out);
    return POLY_OK;
}

int poly_div(poly_t const *a, poly_t const *b, poly_t *q, poly_t *r)
{
    poly_t rem = *a;

    poly_zero(q);
    poly_zero(r);
    if (b->deg == 0 && b->c[0] == 0.0)
        return POLY_ERR_DIV_ZERO;
    if (a->deg < b->deg)
    {
        *r = *a;
        return POLY_OK;
    }
    for (int i = a->deg - b->deg; i >= 0; i--)
    {
        int const shift = i;
        double const factor = rem.c[shift + b->deg] / b->c[b->deg];

        q->c[shift] = factor;
        for (int j = 0; j <= b->deg; j++)
            rem.c[shift + j] -= factor * b->c[j];
    }
    q->deg = a->deg - b->deg;
    trim(q);
    /* The remainder is what is left below the divisor's degree. */
    for (int i = 0; i < b->deg; i++)
        r->c[i] = rem.c[i];
    r->deg = b->deg > 0 ? b->deg - 1 : 0;
    trim(r);
    return POLY_OK;
}

int poly_quadratic(double a, double b, double c, double *disc, double *r1re,
                   double *r1im, double *r2re, double *r2im)
{
    double const d = b * b - 4.0 * a * c;

    *disc = d;
    if (a == 0.0)
    {
        /* A line, not a parabola. */
        *r1re = b != 0.0 ? -c / b : 0.0;
        *r1im = 0.0;
        *r2re = *r1re;
        *r2im = 0.0;
        return POLY_ERR_SYNTAX;
    }
    if (d >= 0.0)
    {
        double const s = bm_sqrt(d);

        *r1re = (-b + s) / (2.0 * a);
        *r2re = (-b - s) / (2.0 * a);
        *r1im = 0.0;
        *r2im = 0.0;
    }
    else
    {
        double const s = bm_sqrt(-d);

        *r1re = -b / (2.0 * a);
        *r2re = *r1re;
        *r1im = s / (2.0 * a);
        *r2im = -*r1im;
    }
    return POLY_OK;
}

/* ---- factoring ---------------------------------------------------------- */

/* Divisors of |n| into out, up to `max` entries.  Returns the count. */
static int divisors(long n, long *out, int max)
{
    int count = 0;

    if (n < 0)
        n = -n;
    for (long d = 1; d * d <= n && count + 2 <= max; d++)
        if (n % d == 0)
        {
            out[count++] = d;
            if (d != n / d)
                out[count++] = n / d;
        }
    return count;
}

static bool fits_whole(poly_t const *p)
{
    for (int i = 0; i <= p->deg; i++)
    {
        double const v = p->c[i];

        if (v != (double)(long long)v)
            return false;
    }
    return true;
}

int poly_factor(poly_t const *p, double roots[POLY_MAX_ROOTS], int *count,
                poly_t *rest)
{
    poly_t cur = *p;
    int found = 0;
    long ds[64];
    long ns[64];
    int dn;
    int nn;

    *count = 0;
    if (!fits_whole(&cur))
    {
        *rest = *p;
        return POLY_OK;
    }
    dn = divisors((long)(cur.c[cur.deg] < 0 ? -cur.c[cur.deg] : cur.c[cur.deg]),
                  ds, 64);
    nn = divisors((long)(cur.c[0] < 0 ? -cur.c[0] : cur.c[0]), ns, 64);

    while (cur.deg > 0 && found < POLY_MAX_ROOTS)
    {
        bool hit = false;

        for (int i = 0; i < dn && !hit; i++)
            for (int j = 0; j < nn && !hit; j++)
            {
                double const cand = (double)ns[j] / (double)ds[i];
                double const v = poly_eval(&cur, cand);

                if (v > -1e-9 && v < 1e-9)
                {
                    poly_t d, q, r;

                    poly_zero(&d);
                    d.deg = 1;
                    d.c[0] = -cand;
                    d.c[1] = 1.0;
                    if (poly_div(&cur, &d, &q, &r) != POLY_OK)
                        continue;
                    roots[found++] = cand;
                    cur = q;
                    hit = true;
                }
                else if (poly_eval(&cur, -cand) > -1e-9 &&
                         poly_eval(&cur, -cand) < 1e-9)
                {
                    poly_t d, q, r;

                    poly_zero(&d);
                    d.deg = 1;
                    d.c[0] = cand;
                    d.c[1] = 1.0;
                    if (poly_div(&cur, &d, &q, &r) != POLY_OK)
                        continue;
                    roots[found++] = -cand;
                    cur = q;
                    hit = true;
                }
            }
        if (!hit)
            break;
    }
    *count = found;
    *rest = cur;
    return POLY_OK;
}
