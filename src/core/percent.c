/* percent.c - see percent.h. */

#include "percent.h"

#include <stddef.h>

char const *pc_error_text(pc_status_t status)
{
    switch (status) {
    case PC_OK:
        return "ok";
    case PC_ERR_ARG:
        return "needs a value to work from";
    case PC_ERR_FROM_ZERO:
        return "no change from zero can be a percentage";
    case PC_ERR_NO_ORIGINAL:
        return "a 100% discount leaves nothing to work back from";
    case PC_ERR_TOO_BIG:
        return "the answer is too large to show";
    case PC_ERR_PERIODS:
        return "years must be a whole number, 0 to 200";
    }
    return "unknown";
}

/* A double that is not a number or has run off the end of what can be shown.
 * Written this way because src/core has no libm: comparing a value with itself
 * is the portable test for NaN. */
static int unusable(double v)
{
    return !(v == v) || v > 1e300 || v < -1e300;
}

pc_status_t pc_change(double from, double to, double *out)
{
    double r;

    if (!out)
        return PC_ERR_ARG;
    if (from == 0.0)
        return PC_ERR_FROM_ZERO;
    r = 100.0 * (to - from) / from;
    if (unusable(r))
        return PC_ERR_TOO_BIG;
    *out = r;
    return PC_OK;
}

pc_status_t pc_apply(double v, double pct, double *out)
{
    double r;

    if (!out)
        return PC_ERR_ARG;
    r = v * (1.0 + pct / 100.0);
    if (unusable(r))
        return PC_ERR_TOO_BIG;
    *out = r;
    return PC_OK;
}

pc_status_t pc_reverse(double now, double pct, double *out)
{
    double const factor = 1.0 + pct / 100.0;
    double r;

    if (!out)
        return PC_ERR_ARG;
    if (factor == 0.0)
        return PC_ERR_NO_ORIGINAL;
    r = now / factor;
    if (unusable(r))
        return PC_ERR_TOO_BIG;
    *out = r;
    return PC_OK;
}

pc_status_t pc_compound(double principal, double pct, int years, double *out)
{
    double factor = 1.0 + pct / 100.0;
    double r = principal;

    if (!out)
        return PC_ERR_ARG;
    if (years < 0 || years > 200)
        return PC_ERR_PERIODS;
    /* Squaring rather than a naive loop: the loop is 200 multiplies and this is
     * eight.  An overflow is a status whether it happens in the running result
     * or in the squaring still to come - an earlier version bailed out of the
     * loop instead and returned the principal, which reads as "no growth". */
    {
        int n = years;

        while (n > 0) {
            if (n & 1) {
                r *= factor;
                if (unusable(r))
                    return PC_ERR_TOO_BIG;
            }
            n >>= 1;
            if (n > 0) {
                factor *= factor;
                if (unusable(factor))
                    return PC_ERR_TOO_BIG;
            }
        }
    }
    *out = r;
    return PC_OK;
}
