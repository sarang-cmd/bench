/* percent.h - the percentage questions that come up in papers.
 *
 * Spec: the request for "a bunch of more maths tools".  Four things are asked
 * over and over, and all four are one multiplication once written down:
 *
 *   percentage change     "the price went from 40 to 46, by what per cent?"
 *   reverse percentage    "after 15% off it costs 34, what did it cost?"
 *   compound growth       "£500 at 4.5% for 7 years, with interest added yearly"
 *   depreciation          the same with a negative rate
 *
 * The rules here are about what counts as an answer rather than the arithmetic:
 * a percentage change from zero is undefined (not infinite), a reverse
 * percentage from a 100% discount has no original, and a growth period count of
 * zero means the amount has not changed - each of those is a status, so the
 * screen can say why instead of printing 0 or an inf.
 *
 * Plain C11, no gint, no libm. */

#ifndef BENCH_PERCENT_H
#define BENCH_PERCENT_H

typedef enum {
    PC_OK = 0,
    PC_ERR_ARG,        /* nothing to divide by, or a NULL out */
    PC_ERR_FROM_ZERO,  /* percentage change measured from nothing */
    PC_ERR_NO_ORIGINAL,/* reverse percentage of 0 or of a 100% discount */
    PC_ERR_TOO_BIG,    /* the answer overflows what a double can show */
    PC_ERR_PERIODS     /* negative or absurd number of periods */
} pc_status_t;

char const *pc_error_text(pc_status_t status);

/* Percentage change from `from` to `to`: 100 (to - from)/from.  Negative when
 * it is a fall.  PC_ERR_FROM_ZERO when `from` is 0. */
pc_status_t pc_change(double from, double to, double *out);

/* The value after applying `pct` per cent: v (1 + pct/100).  `pct` may be
 * negative. */
pc_status_t pc_apply(double v, double pct, double *out);

/* The original value when `now` is the value after `pct` per cent:
 * now / (1 + pct/100).  PC_ERR_NO_ORIGINAL at exactly -100%. */
pc_status_t pc_reverse(double now, double pct, double *out);

/* `principal` grown at `pct` per cent a year for `years` years, interest added
 * each year: principal (1 + pct/100)^years.  `pct` negative is depreciation.
 * `years` is a whole number, 0 upwards; PC_ERR_PERIODS otherwise. */
pc_status_t pc_compound(double principal, double pct, int years, double *out);

#endif /* BENCH_PERCENT_H */
