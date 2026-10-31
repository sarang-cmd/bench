/* numfmt.h - decimal formatting without float printf.
 *
 * Why this module exists (docs/plan.md section 6.2 and spike question 3):
 * the calculator build may not print floats.  This module never calls printf,
 * never uses long double and never depends on the C library's float formatting.
 * It converts a double to the exact decimal digits of its binary value with a
 * small big-integer, then rounds those decimal digits half-up.  That gives the
 * "half-up on decimal digits, not on the binary float" rule in the plan.
 *
 * Value model: a decimal number is
 *     (negative ? -1 : +1) * int(digit[0..ndigits)) * 10^exp10
 * with no trailing zero digits and no leading zero digits.  Zero is a special
 * case with zero = true.  Examples:
 *     15.0   -> digit {1,5},  ndigits 2, exp10  0
 *     0.15   -> digit {1,5},  ndigits 2, exp10 -2
 *     2.5e10 -> digit {2,5},  ndigits 2, exp10 10
 *
 * Plain C11, no gint. */

#ifndef BENCH_NUMFMT_H
#define BENCH_NUMFMT_H

#include <stdbool.h>

#define NUMFMT_DIGITS 24        /* significant digits kept in a numfmt_dec_t */
#define NUMFMT_DEFAULT_SIG 15   /* digits used when canonicalising a double */
#define NUMFMT_BUFSIZE 40       /* a rendered number never needs more than this */

#define NUMFMT_OK 0
#define NUMFMT_ERR_ARG (-1)
#define NUMFMT_ERR_SIZE (-2)

typedef struct {
    bool neg;
    bool zero;
    signed char ndigits;                    /* 1..NUMFMT_DIGITS when !zero */
    signed char digit[NUMFMT_DIGITS];       /* most significant first, 0..9 */
    short exp10;                            /* weight of the least significant */
} numfmt_dec_t;

/* ---- construction ------------------------------------------------------- */

void numfmt_dec_zero(numfmt_dec_t *d);

/* Exact decimal digits of x, rounded half-up to `sig` significant digits.
 * Returns NUMFMT_ERR_ARG for NaN/infinity or a sig outside 1..NUMFMT_DIGITS. */
int numfmt_dec_from_double(double x, int sig, numfmt_dec_t *d);

/* Copy with sign flipped. */
int numfmt_dec_negate(numfmt_dec_t *d);

/* ---- rounding ----------------------------------------------------------- */

/* Round half-up to `sig` significant digits (no-op when there are fewer). */
int numfmt_dec_round_sig(numfmt_dec_t *d, int sig);

/* Round half-up at decimal place 10^-decimals.  decimals may be negative
 * (round to tens, hundreds...).  Half-up means away from zero on the first
 * dropped digit exactly 5. */
int numfmt_dec_round_place(numfmt_dec_t *d, int decimals);

/* ---- inspection --------------------------------------------------------- */

/* Decimals needed to write the value exactly: 2 for 0.15, 0 for 15, 0 for 300.
 * Never negative. */
int numfmt_dec_place(numfmt_dec_t const *d);

int numfmt_dec_significant(numfmt_dec_t const *d);
int numfmt_dec_lead_digit(numfmt_dec_t const *d);    /* 0 when zero */
int numfmt_dec_sci_exponent(numfmt_dec_t const *d);  /* 0 when zero */
bool numfmt_dec_is_zero(numfmt_dec_t const *d);
bool numfmt_dec_is_negative(numfmt_dec_t const *d);

/* Value as a double.  Display only: the decimal digits are exact, this is not. */
double numfmt_dec_to_double(numfmt_dec_t const *d);

/* Compare two decimals by value, tolerance NUMFMT_DEFAULT_SIG-equivalent. */
bool numfmt_dec_equal(numfmt_dec_t const *a, numfmt_dec_t const *b);

/* ---- rendering --------------------------------------------------------- */

/* Plain decimal text, no exponent.
 *   min_dec  minimum number of decimals (zero padded)
 *   max_dec  maximum number of decimals (rounded half-up), -1 for no limit
 * Returns the length written, or NUMFMT_ERR_SIZE if `size` is too small.
 * The buffer is always NUL terminated on success. */
int numfmt_dec_render(numfmt_dec_t const *d, int min_dec, int max_dec,
                      char *buf, int size);

/* Scientific text: "1.5e-7", "2.5e10".  One digit before the point, `sig`
 * significant digits after rounding.  No exponent for values 1..999
 * (exp10 of the leading digit 0). */
int numfmt_dec_render_sci(numfmt_dec_t const *d, int sig, char *buf, int size);

/* Plain when it fits in `max_width` characters, scientific otherwise. */
int numfmt_dec_render_fit(numfmt_dec_t const *d, int min_dec, int max_dec,
                          int max_width, char *buf, int size);

/* ---- double helpers ----------------------------------------------------- */

int numfmt_round_sig(double x, int sig, double *out);
int numfmt_round_place(double x, int decimals, double *out);
int numfmt_format_fixed(double x, int decimals, char *buf, int size);
int numfmt_format_sig(double x, int sig, char *buf, int size);

#endif /* BENCH_NUMFMT_H */
