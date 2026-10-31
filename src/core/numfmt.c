/* numfmt.c - exact decimal conversion and rendering.  See numfmt.h.
 *
 * No printf, no long double, no libm.  The only arithmetic is integer
 * arithmetic on a small big-integer, which keeps the results identical on the
 * PC and on the SuperH build. */

#include "numfmt.h"

#include <string.h>

/* Worst case exact digit count: a subnormal double (m * 5^1074) needs 767
 * decimal digits.  800 leaves room. */
#define EXACT_DIGITS_MAX 800

_Static_assert(sizeof(double) == 8, "numfmt expects IEEE-754 binary64 doubles");

/* ---- small big-integer -------------------------------------------------- */

/* 96 limbs = 3072 bits.  m * 5^1074 needs about 2548 bits. */
#define BIG_LIMBS 96

typedef struct {
    int n;                      /* limbs in use, 0 means zero */
    unsigned int limb[BIG_LIMBS];
} big_t;

static void big_zero(big_t *b)
{
    b->n = 0;
    memset(b->limb, 0, sizeof b->limb);
}

static void big_from_u64(big_t *b, unsigned long long v)
{
    big_zero(b);
    while (v) {
        b->limb[b->n++] = (unsigned int)(v & 0xFFFFFFFFu);
        v >>= 32;
    }
}

static void big_mul_small(big_t *b, unsigned int m)
{
    unsigned long long carry = 0;
    int i;

    if (b->n == 0)
        return;
    for (i = 0; i < b->n; i++) {
        unsigned long long t = (unsigned long long)b->limb[i] * m + carry;
        b->limb[i] = (unsigned int)(t & 0xFFFFFFFFu);
        carry = t >> 32;
    }
    while (carry && b->n < BIG_LIMBS) {
        b->limb[b->n++] = (unsigned int)(carry & 0xFFFFFFFFu);
        carry >>= 32;
    }
}

static void big_shl(big_t *b, int bits)
{
    int words, shift, i;

    if (b->n == 0 || bits <= 0)
        return;
    words = bits / 32;
    shift = bits % 32;

    if (shift) {
        unsigned int carry = 0;
        for (i = 0; i < b->n; i++) {
            unsigned int v = b->limb[i];
            b->limb[i] = (v << shift) | carry;
            carry = (unsigned int)((unsigned long long)v >> (32 - shift));
        }
        if (carry && b->n < BIG_LIMBS)
            b->limb[b->n++] = carry;
    }
    if (words) {
        if (b->n + words > BIG_LIMBS)
            b->n = BIG_LIMBS - words; /* cannot happen for binary64 input */
        for (i = b->n - 1; i >= 0; i--)
            b->limb[i + words] = b->limb[i];
        for (i = 0; i < words; i++)
            b->limb[i] = 0;
        b->n += words;
    }
}

/* b /= d, returns the remainder. */
static unsigned int big_divmod_small(big_t *b, unsigned int d)
{
    unsigned long long rem = 0;
    int i;

    for (i = b->n - 1; i >= 0; i--) {
        unsigned long long cur = (rem << 32) | b->limb[i];
        b->limb[i] = (unsigned int)(cur / d);
        rem = cur % d;
    }
    while (b->n > 0 && b->limb[b->n - 1] == 0)
        b->n--;
    return (unsigned int)rem;
}

/* Writes the decimal digits of b (no leading zeros) into out, returns length.
 * b must be non-zero. */
static int big_to_decimal(big_t const *b, char *out)
{
    big_t t = *b;
    char tmp[EXACT_DIGITS_MAX];
    int len = 0, pos = 0;

    while (t.n > 0) {
        unsigned int group = big_divmod_small(&t, 1000000000u);
        int k;
        for (k = 0; k < 9; k++) {
            tmp[len++] = (char)('0' + (group % 10u));
            group /= 10u;
        }
    }
    while (len > 1 && tmp[len - 1] == '0')
        len--;
    while (len > 0)
        out[pos++] = tmp[--len];
    out[pos] = '\0';
    return pos;
}

/* ---- exact conversion --------------------------------------------------- */

/* Splits a finite double into sign, mantissa and binary exponent:
 *     |x| = m * 2^e,  m < 2^53. */
static bool double_split(double x, bool *neg, unsigned long long *m, int *e,
                         bool *zero)
{
    unsigned long long bits;
    int exp_bits;
    unsigned long long frac;

    memcpy(&bits, &x, sizeof bits);
    *neg = (bits >> 63) != 0;
    exp_bits = (int)((bits >> 52) & 0x7FFu);
    frac = bits & 0xFFFFFFFFFFFFFULL;

    if (exp_bits == 0x7FF)
        return false; /* NaN or infinity */
    if (exp_bits == 0) {
        *zero = (frac == 0);
        *m = frac;
        *e = -1074;
    } else {
        *zero = false;
        *m = frac | (1ULL << 52);
        *e = exp_bits - 1075;
    }
    if (*zero)
        *neg = false; /* no negative zero in this model */
    return true;
}

/* Writes the exact decimal digits of a finite double: value = digits * 10^p,
 * trailing zeros removed.  Returns the digit count, or -1 on error. */
static int exact_digits(double x, char *digits, int *p, bool *neg)
{
    bool zero = false, negative = false;
    unsigned long long m = 0;
    int e = 0, len;

    if (!double_split(x, &negative, &m, &e, &zero))
        return -1;
    *neg = negative;

    if (zero) {
        digits[0] = '0';
        digits[1] = '\0';
        *p = 0;
        return 1;
    }

    big_t b;
    big_from_u64(&b, m);

    if (e >= 0) {
        big_shl(&b, e);
        *p = 0;
    } else {
        int k = -e;
        /* 5^13 = 1220703125 fits in 32 bits: fewer passes over the limbs. */
        while (k >= 13) {
            big_mul_small(&b, 1220703125u);
            k -= 13;
        }
        while (k-- > 0)
            big_mul_small(&b, 5u);
        *p = e;
    }

    len = big_to_decimal(&b, digits);

    /* Remove trailing zeros, folding them into the power of ten. */
    while (len > 1 && digits[len - 1] == '0') {
        len--;
        (*p)++;
    }
    digits[len] = '\0';
    return len;
}

/* ---- construction ------------------------------------------------------- */

void numfmt_dec_zero(numfmt_dec_t *d)
{
    if (!d)
        return;
    memset(d, 0, sizeof *d);
    d->ndigits = 1;
    d->digit[0] = 0;
    d->zero = true;
}

static void dec_trim(numfmt_dec_t *d)
{
    while (d->ndigits > 1 && d->digit[d->ndigits - 1] == 0) {
        d->ndigits--;
        d->exp10++;
    }
}

static void dec_set_digits(numfmt_dec_t *d, char const *digits, int len,
                           int p, bool neg)
{
    int i;

    memset(d, 0, sizeof *d);
    if (len <= 0) {
        numfmt_dec_zero(d);
        return;
    }
    if (len > NUMFMT_DIGITS)
        len = NUMFMT_DIGITS;
    for (i = 0; i < len; i++)
        d->digit[i] = (signed char)(digits[i] - '0');
    d->ndigits = (signed char)len;
    d->exp10 = (short)p;
    d->neg = neg && (len > 0);
    d->zero = false;
    dec_trim(d);
    if (d->ndigits == 1 && d->digit[0] == 0) {
        d->zero = true;
        d->neg = false;
        d->exp10 = 0;
    }
}

/* Increments the first `n` digits in place.  Returns true on carry out. */
static bool dec_inc_digits(signed char *digit, int n)
{
    int i;
    for (i = n - 1; i >= 0; i--) {
        if (digit[i] < 9) {
            digit[i]++;
            return false;
        }
        digit[i] = 0;
    }
    return true;
}

int numfmt_dec_from_double(double x, int sig, numfmt_dec_t *d)
{
    char digits[EXACT_DIGITS_MAX];
    int p = 0, len;
    bool neg = false;

    if (!d || sig < 1 || sig > NUMFMT_DIGITS)
        return NUMFMT_ERR_ARG;

    len = exact_digits(x, digits, &p, &neg);
    if (len < 0)
        return NUMFMT_ERR_ARG;

    if (len <= sig) {
        dec_set_digits(d, digits, len, p, neg);
        return NUMFMT_OK;
    }

    /* Keep `sig` digits and round half-up on the first dropped one. */
    {
        bool up = digits[sig] >= '5';
        int i;
        memset(d, 0, sizeof *d);
        for (i = 0; i < sig; i++)
            d->digit[i] = (signed char)(digits[i] - '0');
        d->ndigits = (signed char)sig;
        d->exp10 = (short)(p + (len - sig));
        d->neg = neg;
        d->zero = false;
        if (up && dec_inc_digits(d->digit, sig)) {
            d->digit[0] = 1;
            d->ndigits = 1;
            d->exp10 = (short)(p + len);
        }
        dec_trim(d);
    }
    return NUMFMT_OK;
}

int numfmt_dec_negate(numfmt_dec_t *d)
{
    if (!d)
        return NUMFMT_ERR_ARG;
    if (!d->zero)
        d->neg = !d->neg;
    return NUMFMT_OK;
}

/* ---- rounding ----------------------------------------------------------- */

int numfmt_dec_round_sig(numfmt_dec_t *d, int sig)
{
    int drop;
    bool up;

    if (!d || sig < 1 || sig > NUMFMT_DIGITS)
        return NUMFMT_ERR_ARG;
    if (d->zero || d->ndigits <= sig)
        return NUMFMT_OK;

    drop = d->ndigits - (int)sig;
    up = d->digit[sig] >= 5;
    d->ndigits = (signed char)sig;
    d->exp10 = (short)(d->exp10 + drop);
    if (up && dec_inc_digits(d->digit, sig)) {
        d->digit[0] = 1;
        d->ndigits = 1;
        d->exp10 = (short)(d->exp10 + sig);
    }
    dec_trim(d);
    return NUMFMT_OK;
}

int numfmt_dec_round_place(numfmt_dec_t *d, int decimals)
{
    int target_p, drop, keep;
    bool up = false;

    if (!d)
        return NUMFMT_ERR_ARG;
    if (d->zero)
        return NUMFMT_OK;

    target_p = -decimals;
    if (d->exp10 >= target_p)
        return NUMFMT_OK; /* already at or coarser than the requested place */

    drop = target_p - (int)d->exp10;
    keep = (int)d->ndigits - drop;

    if (drop < d->ndigits)
        up = d->digit[keep] >= 5; /* first dropped digit */
    else if (drop == d->ndigits)
        up = d->digit[0] >= 5; /* the whole value is the dropped part */
    /* else: |value| < 0.1 unit at this place, rounds to zero */

    if (keep <= 0) {
        if (!up) {
            numfmt_dec_zero(d);
            return NUMFMT_OK;
        }
        d->ndigits = 1;
        d->digit[0] = 1;
        d->exp10 = (short)target_p;
        d->zero = false;
        return NUMFMT_OK;
    }

    d->ndigits = (signed char)keep;
    d->exp10 = (short)target_p;
    if (up && dec_inc_digits(d->digit, keep)) {
        d->digit[0] = 1;
        d->ndigits = 1;
        d->exp10 = (short)(target_p + keep);
    }
    dec_trim(d);
    return NUMFMT_OK;
}

/* ---- inspection --------------------------------------------------------- */

int numfmt_dec_place(numfmt_dec_t const *d)
{
    if (!d || d->zero)
        return 0;
    return d->exp10 < 0 ? -(int)d->exp10 : 0;
}

int numfmt_dec_significant(numfmt_dec_t const *d)
{
    if (!d)
        return 0;
    return d->zero ? 0 : (int)d->ndigits;
}

int numfmt_dec_lead_digit(numfmt_dec_t const *d)
{
    if (!d || d->zero)
        return 0;
    return d->digit[0];
}

int numfmt_dec_sci_exponent(numfmt_dec_t const *d)
{
    if (!d || d->zero)
        return 0;
    return (int)d->exp10 + (int)d->ndigits - 1;
}

bool numfmt_dec_is_zero(numfmt_dec_t const *d)
{
    return !d || d->zero;
}

bool numfmt_dec_is_negative(numfmt_dec_t const *d)
{
    return d && !d->zero && d->neg;
}

/* 10^k as a double, by squaring so no libm is needed. */
static double pow10_double(int k)
{
    double result = 1.0, base = 10.0;
    int n = k < 0 ? -k : k;

    while (n) {
        if (n & 1)
            result *= base;
        base *= base;
        n >>= 1;
    }
    return k < 0 ? 1.0 / result : result;
}

double numfmt_dec_to_double(numfmt_dec_t const *d)
{
    double v = 0.0;
    int i;

    if (!d || d->zero)
        return 0.0;
    for (i = 0; i < d->ndigits; i++)
        v = v * 10.0 + (double)d->digit[i];
    v *= pow10_double(d->exp10);
    return d->neg ? -v : v;
}

bool numfmt_dec_equal(numfmt_dec_t const *a, numfmt_dec_t const *b)
{
    if (!a || !b)
        return false;
    if (a->zero || b->zero)
        return a->zero && b->zero;
    return a->neg == b->neg && a->ndigits == b->ndigits &&
           a->exp10 == b->exp10 && memcmp(a->digit, b->digit,
                                          (size_t)a->ndigits) == 0;
}

/* ---- rendering ---------------------------------------------------------- */

typedef struct {
    char *buf;
    int size;
    int len;
    bool ok;
} writer_t;

static void w_char(writer_t *w, char c)
{
    if (!w->ok)
        return;
    if (w->len + 1 >= w->size) { /* reserve room for the NUL */
        w->ok = false;
        return;
    }
    w->buf[w->len++] = c;
}

static void w_digits(writer_t *w, numfmt_dec_t const *d, int from, int to)
{
    int i;
    for (i = from; i < to; i++)
        w_char(w, (char)('0' + d->digit[i]));
}

static void w_zeros(writer_t *w, int n)
{
    while (n-- > 0)
        w_char(w, '0');
}

static int render_plain(numfmt_dec_t const *d, int min_dec, int max_dec,
                        char *buf, int size)
{
    numfmt_dec_t t = *d;
    writer_t w = { buf, size, 0, true };
    int n_int, decimals, i;

    if (max_dec >= 0)
        (void)numfmt_dec_round_place(&t, max_dec);

    if (t.zero) {
        w_char(&w, '0');
        if (min_dec > 0) {
            w_char(&w, '.');
            w_zeros(&w, min_dec);
        }
        if (!w.ok)
            return NUMFMT_ERR_SIZE;
        buf[w.len] = '\0';
        return w.len;
    }

    if (t.neg)
        w_char(&w, '-');

    n_int = (int)t.ndigits + (int)t.exp10;

    if (n_int <= 0) {
        w_char(&w, '0');
        w_char(&w, '.');
        w_zeros(&w, -n_int);
        w_digits(&w, &t, 0, t.ndigits);
        decimals = -n_int + t.ndigits;
    } else if (n_int >= t.ndigits) {
        w_digits(&w, &t, 0, t.ndigits);
        w_zeros(&w, n_int - t.ndigits);
        decimals = 0;
    } else {
        w_digits(&w, &t, 0, n_int);
        w_char(&w, '.');
        w_digits(&w, &t, n_int, t.ndigits);
        decimals = t.ndigits - n_int;
    }

    for (i = decimals; i < min_dec; i++) {
        if (decimals == 0 && i == 0)
            w_char(&w, '.');
        w_char(&w, '0');
    }

    if (!w.ok)
        return NUMFMT_ERR_SIZE;
    buf[w.len] = '\0';
    return w.len;
}

int numfmt_dec_render(numfmt_dec_t const *d, int min_dec, int max_dec,
                      char *buf, int size)
{
    if (!d || !buf || size <= 0 || min_dec < 0)
        return NUMFMT_ERR_ARG;
    if (max_dec >= 0 && max_dec < min_dec)
        return NUMFMT_ERR_ARG;
    return render_plain(d, min_dec, max_dec, buf, size);
}

int numfmt_dec_render_sci(numfmt_dec_t const *d, int sig, char *buf, int size)
{
    numfmt_dec_t t;
    writer_t w = { buf, size, 0, true };
    int exp, i;

    if (!d || !buf || size <= 0 || sig < 1)
        return NUMFMT_ERR_ARG;

    t = *d;
    if (numfmt_dec_round_sig(&t, sig) != NUMFMT_OK)
        return NUMFMT_ERR_ARG;

    if (t.zero) {
        w_char(&w, '0');
        if (!w.ok)
            return NUMFMT_ERR_SIZE;
        buf[w.len] = '\0';
        return w.len;
    }

    if (t.neg)
        w_char(&w, '-');
    w_char(&w, (char)('0' + t.digit[0]));
    if (sig > 1) {
        w_char(&w, '.');
        for (i = 1; i < sig; i++)
            w_char(&w, i < t.ndigits ? (char)('0' + t.digit[i]) : '0');
    }

    exp = (int)t.exp10 + (int)t.ndigits - 1;
    if (exp != 0) {
        char tmp[8];
        int n = 0;
        bool eneg = exp < 0;
        unsigned int e = (unsigned int)(eneg ? -exp : exp);

        do {
            tmp[n++] = (char)('0' + (e % 10u));
            e /= 10u;
        } while (e && n < (int)sizeof tmp);
        w_char(&w, 'e');
        if (eneg)
            w_char(&w, '-');
        while (n > 0)
            w_char(&w, tmp[--n]);
    }

    if (!w.ok)
        return NUMFMT_ERR_SIZE;
    buf[w.len] = '\0';
    return w.len;
}

int numfmt_dec_render_fit(numfmt_dec_t const *d, int min_dec, int max_dec,
                          int max_width, char *buf, int size)
{
    int n;

    if (!d || !buf || size <= 0)
        return NUMFMT_ERR_ARG;

    n = render_plain(d, min_dec, max_dec, buf, size);
    if (n != NUMFMT_ERR_SIZE && n <= max_width)
        return n;

    /* Too wide (or did not fit the buffer at all): fall back to scientific,
     * dropping significant digits until the text fits max_width as well. */
    {
        int sig = d->zero ? 1 : (int)d->ndigits;
        int got;
        if (sig > 16)
            sig = 16;
        do {
            got = numfmt_dec_render_sci(d, sig, buf, size);
            if (got < 0 || got <= max_width || sig == 1)
                return got;
            sig--;
        } while (1);
    }
}

/* ---- double helpers ----------------------------------------------------- */

int numfmt_round_sig(double x, int sig, double *out)
{
    numfmt_dec_t d;
    int err = numfmt_dec_from_double(x, sig, &d);
    if (err != NUMFMT_OK)
        return err;
    if (out)
        *out = numfmt_dec_to_double(&d);
    return NUMFMT_OK;
}

int numfmt_round_place(double x, int decimals, double *out)
{
    numfmt_dec_t d;
    int err = numfmt_dec_from_double(x, NUMFMT_DEFAULT_SIG, &d);
    if (err != NUMFMT_OK)
        return err;
    err = numfmt_dec_round_place(&d, decimals);
    if (err != NUMFMT_OK)
        return err;
    if (out)
        *out = numfmt_dec_to_double(&d);
    return NUMFMT_OK;
}

int numfmt_format_fixed(double x, int decimals, char *buf, int size)
{
    numfmt_dec_t d;
    int err = numfmt_dec_from_double(x, NUMFMT_DEFAULT_SIG, &d);
    if (err != NUMFMT_OK)
        return err;
    return numfmt_dec_render(&d, decimals, decimals, buf, size);
}

int numfmt_format_sig(double x, int sig, char *buf, int size)
{
    numfmt_dec_t d;
    int err = numfmt_dec_from_double(x, sig, &d);
    int decimals, n;

    if (err != NUMFMT_OK)
        return err;

    /* Show all `sig` figures, padding with zeros: 2 at 2 figures is "2.0". */
    decimals = d.zero ? 0 : sig - (int)d.ndigits - (int)d.exp10;
    if (decimals < 0)
        decimals = 0;
    if (decimals > 12)
        decimals = 12;

    n = render_plain(&d, decimals, -1, buf, size);
    if (n != NUMFMT_ERR_SIZE && n <= 16)
        return n;
    return numfmt_dec_render_sci(&d, sig, buf, size);
}
