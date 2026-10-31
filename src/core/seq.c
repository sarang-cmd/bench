/* seq.c - see seq.h. */

#include "seq.h"

#include <stddef.h>

#include "mathfn.h"

double seq_arith_nth(double a, double d, int n)
{
    return a + d * (double)(n - 1);
}

double seq_arith_sum(double a, double d, int n)
{
    if (n <= 0)
        return 0.0;
    /* n/2 (first + last), the form that does not lose the small terms. */
    return 0.5 * (double)n * (a + seq_arith_nth(a, d, n));
}

double seq_geom_nth(double a, double r, int n)
{
    return a * bm_pow(r, (double)(n - 1));
}

double seq_geom_sum(double a, double r, int n)
{
    if (n <= 0)
        return 0.0;
    if (r == 1.0)
        return a * (double)n;
    return a * (1.0 - bm_pow(r, (double)n)) / (1.0 - r);
}

bool seq_geom_infinite(double a, double r, double *out)
{
    if (r <= -1.0 || r >= 1.0)
        return false;
    if (out)
        *out = a / (1.0 - r);
    return true;
}

static int64_t gcd64(int64_t a, int64_t b)
{
    while (b)
    {
        int64_t const t = a % b;

        a = b;
        b = t;
    }
    return a < 0 ? -a : a;
}

bool seq_binom(int n, int k, int64_t *out)
{
    int64_t v = 1;

    if (n < 0 || n > 62 || k < 0 || k > n)
        return false;
    if (k > n - k)
        k = n - k;
    for (int i = 1; i <= k; i++)
    {
        /* Cancel before multiplying: v * (n-k+i) alone would overflow even
         * when the answer fits. */
        int64_t num = (int64_t)(n - k + i);
        int64_t den = (int64_t)i;
        int64_t g = gcd64(v, den);

        v /= g;
        den /= g;
        g = gcd64(num, den);
        num /= g;
        den /= g;
        if (v > INT64_MAX / num)
            return false;
        v = v * num / den;
        if (v < 0)
            return false;
    }
    if (out)
        *out = v;
    return true;
}

bool seq_pascal_row(int n, int64_t *row, int size)
{
    if (!row || n < 0 || n > SEQ_PASCAL_MAX || size < n + 1)
        return false;
    for (int k = 0; k <= n; k++)
        if (!seq_binom(n, k, &row[k]))
            return false;
    return true;
}

void seq_arith_terms(double a, double d, int n, double *out)
{
    for (int i = 0; i < n; i++)
        out[i] = a + d * (double)i;
}

void seq_geom_terms(double a, double r, int n, double *out)
{
    double v = a;

    for (int i = 0; i < n; i++)
    {
        out[i] = v;
        v *= r;
    }
}
