/* uncert.c - uncertainty arithmetic and display.  See uncert.h.
 *
 * No libm: finiteness, |x|, integer powers and square roots are written here,
 * so the same code runs on the PC and on the calculator. */

#include "uncert.h"

#include <string.h>

#include "numfmt.h"

/* ---- small numeric helpers (no libm) ------------------------------------ */

static bool dbl_finite(double x)
{
    unsigned long long bits;
    memcpy(&bits, &x, sizeof bits);
    return ((bits >> 52) & 0x7FFu) != 0x7FFu;
}

static double dbl_abs(double x)
{
    return x < 0.0 ? -x : x;
}

static bool dbl_is_integer(double x)
{
    if (!dbl_finite(x) || dbl_abs(x) > 1e9)
        return false;
    double r = x < 0.0 ? -x : x;
    double f = r - (double)(unsigned long long)r;
    return f == 0.0;
}

static double pow_2(int e)
{
    double m = 1.0;
    double b = 2.0;
    bool neg = e < 0;
    unsigned int n = (unsigned int)(neg ? -e : e);

    while (n) {
        if (n & 1u)
            m *= b;
        b *= b;
        n >>= 1;
    }
    return neg ? 1.0 / m : m;
}

/* Integer power by repeated squaring.  Used for A^n with a whole n. */
static double pow_int(double base, long long n)
{
    double result = 1.0, b = base;
    unsigned long long k = (unsigned long long)(n < 0 ? -n : n);

    while (k) {
        if (k & 1ULL)
            result *= b;
        b *= b;
        k >>= 1;
    }
    return n < 0 ? 1.0 / result : result;
}

/* Square root by Newton iteration from a rough seed.  Newton doubles the
 * number of correct bits each step, so 20 steps is far past binary64. */
static double dbl_sqrt(double x)
{
    unsigned long long bits;
    double g;
    int e, i;

    if (!(x > 0.0))
        return 0.0;
    memcpy(&bits, &x, sizeof bits);
    e = (int)((bits >> 52) & 0x7FFu) - 1023;
    g = 1.4 * pow_2(e / 2); /* seed: 1.4 * 2^(e/2), within 2x of the root */
    for (i = 0; i < 20; i++)
        g = 0.5 * (g + x / g);
    return g;
}

/* ---- results ------------------------------------------------------------ */

static uncert_result_t make(double value, double unc, uncert_status_t status)
{
    uncert_result_t r;
    r.q.value = value;
    r.q.unc = unc;
    r.status = status;
    if (status == UNCERT_OK && (!dbl_finite(value) || !dbl_finite(unc)))
        r.status = UNCERT_ERR_OVERFLOW;
    return r;
}

static uncert_result_t fail(uncert_status_t status)
{
    return make(0.0, 0.0, status);
}

/* Common input checks. */
static uncert_status_t check_qty(uncert_qty_t q)
{
    if (!dbl_finite(q.value) || !dbl_finite(q.unc))
        return UNCERT_ERR_ARG;
    if (q.unc < 0.0)
        return UNCERT_ERR_NEG_UNC;
    return UNCERT_OK;
}

char const *uncert_error_text(uncert_status_t status)
{
    switch (status) {
    case UNCERT_OK:
        return "ok";
    case UNCERT_ERR_ARG:
        return "bad input";
    case UNCERT_ERR_NEG_UNC:
        return "uncertainty below 0";
    case UNCERT_ERR_ZERO_A:
        return "first value is 0";
    case UNCERT_ERR_ZERO_B:
        return "second value is 0";
    case UNCERT_ERR_NEG_BASE:
        return "negative base, odd power";
    case UNCERT_ERR_OVERFLOW:
        return "result too large";
    }
    return "error";
}

/* ---- operations --------------------------------------------------------- */

uncert_result_t uncert_add(uncert_qty_t a, uncert_qty_t b)
{
    uncert_status_t st = check_qty(a);
    if (st != UNCERT_OK)
        return fail(st);
    st = check_qty(b);
    if (st != UNCERT_OK)
        return fail(st);
    return make(a.value + b.value, a.unc + b.unc, UNCERT_OK);
}

uncert_result_t uncert_sub(uncert_qty_t a, uncert_qty_t b)
{
    uncert_status_t st = check_qty(a);
    if (st != UNCERT_OK)
        return fail(st);
    st = check_qty(b);
    if (st != UNCERT_OK)
        return fail(st);
    return make(a.value - b.value, a.unc + b.unc, UNCERT_OK);
}

uncert_result_t uncert_mul(uncert_qty_t a, uncert_qty_t b)
{
    uncert_status_t st = check_qty(a);
    double aa, ab, r, rel;

    if (st != UNCERT_OK)
        return fail(st);
    st = check_qty(b);
    if (st != UNCERT_OK)
        return fail(st);

    aa = dbl_abs(a.value);
    ab = dbl_abs(b.value);
    if (aa == 0.0)
        return fail(UNCERT_ERR_ZERO_A);
    if (ab == 0.0)
        return fail(UNCERT_ERR_ZERO_B);

    r = a.value * b.value;
    rel = a.unc / aa + b.unc / ab;
    return make(r, dbl_abs(r) * rel, UNCERT_OK);
}

uncert_result_t uncert_div(uncert_qty_t a, uncert_qty_t b)
{
    uncert_status_t st = check_qty(a);
    double aa, ab, r, rel;

    if (st != UNCERT_OK)
        return fail(st);
    st = check_qty(b);
    if (st != UNCERT_OK)
        return fail(st);

    aa = dbl_abs(a.value);
    ab = dbl_abs(b.value);
    if (aa == 0.0)
        return fail(UNCERT_ERR_ZERO_A);
    if (ab == 0.0)
        return fail(UNCERT_ERR_ZERO_B);

    r = a.value / b.value;
    rel = a.unc / aa + b.unc / ab;
    return make(r, dbl_abs(r) * rel, UNCERT_OK);
}

uncert_result_t uncert_pow(uncert_qty_t a, double n)
{
    uncert_status_t st = check_qty(a);
    double aa, r;

    if (st != UNCERT_OK)
        return fail(st);
    if (!dbl_finite(n))
        return fail(UNCERT_ERR_ARG);

    aa = dbl_abs(a.value);
    if (aa == 0.0) {
        /* 0^n is only meaningful for a positive exponent; either way the
         * relative uncertainty is undefined. */
        return fail(UNCERT_ERR_ZERO_A);
    }
    if (a.value < 0.0 && !dbl_is_integer(n)) {
        /* A negative base with a fractional power has no real value, except for
         * the square-root-and-inverse pair, which is not a real number either. */
        return fail(UNCERT_ERR_NEG_BASE);
    }

    if (dbl_is_integer(n)) {
        r = pow_int(a.value, (long long)n);
    } else if (n == 0.5) {
        if (a.value < 0.0)
            return fail(UNCERT_ERR_NEG_BASE);
        r = dbl_sqrt(a.value);
    } else if (n == -0.5) {
        if (a.value < 0.0)
            return fail(UNCERT_ERR_NEG_BASE);
        r = 1.0 / dbl_sqrt(a.value);
    } else {
        /* Anything else needs a libm exp/log, which this build does not have
         * (see docs/STATUS.md, open question on libm). */
        return fail(UNCERT_ERR_ARG);
    }

    return make(r, dbl_abs(r) * dbl_abs(n) * (a.unc / aa), UNCERT_OK);
}

uncert_result_t uncert_scale(uncert_qty_t a, double k)
{
    uncert_status_t st = check_qty(a);
    if (st != UNCERT_OK)
        return fail(st);
    if (!dbl_finite(k))
        return fail(UNCERT_ERR_ARG);
    return make(k * a.value, dbl_abs(k) * a.unc, UNCERT_OK);
}

uncert_result_t uncert_apply(uncert_op_t op, uncert_qty_t a, uncert_qty_t b)
{
    switch (op) {
    case UNCERT_OP_ADD:
        return uncert_add(a, b);
    case UNCERT_OP_SUB:
        return uncert_sub(a, b);
    case UNCERT_OP_MUL:
        return uncert_mul(a, b);
    case UNCERT_OP_DIV:
        return uncert_div(a, b);
    case UNCERT_OP_POW:
        return uncert_pow(a, b.value);
    case UNCERT_OP_SCALE:
        return uncert_scale(a, b.value);
    }
    return fail(UNCERT_ERR_ARG);
}

/* ---- display ------------------------------------------------------------ */

static int sig_for(numfmt_dec_t const *unc, uncert_sig_mode_t mode)
{
    if (mode == UNCERT_SIG_ONE_OR_TWO && numfmt_dec_lead_digit(unc) == 1)
        return 2;
    return 1;
}

static void copy_text(char *dst, size_t size, char const *src)
{
    if (size == 0)
        return;
    while (*src && size > 1) {
        *dst++ = *src++;
        size--;
    }
    *dst = '\0';
}

uncert_status_t uncert_display(uncert_qty_t q, uncert_sig_mode_t mode,
                               uncert_display_t *out)
{
    numfmt_dec_t ud, vd;
    int decimals, sig;
    double unc_value;

    if (!out)
        return UNCERT_ERR_ARG;
    {
        uncert_status_t st = check_qty(q);
        if (st != UNCERT_OK)
            return st;
    }
    if (mode != UNCERT_SIG_ONE && mode != UNCERT_SIG_ONE_OR_TWO)
        return UNCERT_ERR_ARG;

    if (numfmt_dec_from_double(q.unc, NUMFMT_DEFAULT_SIG, &ud) != NUMFMT_OK)
        return UNCERT_ERR_ARG;
    if (numfmt_dec_from_double(q.value, NUMFMT_DEFAULT_SIG, &vd) != NUMFMT_OK)
        return UNCERT_ERR_ARG;

    sig = sig_for(&ud, mode);
    if (numfmt_dec_round_sig(&ud, sig) != NUMFMT_OK)
        return UNCERT_ERR_ARG;

    if (numfmt_dec_is_zero(&ud)) {
        /* An exact value: show its own decimals, capped so the field stays
         * readable on the 384 pixel screen. */
        decimals = numfmt_dec_place(&vd);
        if (decimals > 6)
            decimals = 6;
    } else {
        decimals = numfmt_dec_place(&ud);
    }

    if (numfmt_dec_round_place(&vd, decimals) != NUMFMT_OK)
        return UNCERT_ERR_ARG;

    if (numfmt_dec_render_fit(&vd, decimals, decimals, 16, out->value,
                              UNCERT_BUFSIZE) < 0)
        return UNCERT_ERR_ARG;
    if (numfmt_dec_render_fit(&ud, decimals, decimals, 16, out->unc,
                              UNCERT_BUFSIZE) < 0)
        return UNCERT_ERR_ARG;

    out->decimals = decimals;
    out->unc_sig = sig;
    out->pct_valid = false;

    if (q.value == 0.0) {
        copy_text(out->pct, sizeof out->pct, "n/a");
        return UNCERT_OK;
    }

    unc_value = numfmt_dec_to_double(&ud);
    {
        char buf[UNCERT_BUFSIZE];
        double pct = 100.0 * unc_value / dbl_abs(q.value);
        if (numfmt_format_sig(pct, 2, buf, sizeof buf) < 0)
            return UNCERT_ERR_ARG;
        copy_text(out->pct, sizeof out->pct, buf);
        out->pct_valid = true;
    }
    return UNCERT_OK;
}
