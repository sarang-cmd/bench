/* numtext.h - small formatting helpers for the screens.
 *
 * Everything that turns a number into text goes through numfmt (core) or the
 * integer helpers below, so the add-in never needs float printf
 * (docs/plan.md section 6.2, spike question 3). */

#ifndef BENCH_NUMTEXT_H
#define BENCH_NUMTEXT_H

#include <stdbool.h>
#include <stdint.h>

/* Fixed decimals, e.g. 1.125 -> "1.13" with 2. */
void nt_fixed(double v, int decimals, char *buf, int size);

/* Significant digits, e.g. 1.125 -> "1.1" with 2. */
void nt_sig(double v, int sig, char *buf, int size);

/* Plain if it fits `max_width` characters, scientific otherwise. */
void nt_fit(double v, int max_dec, int max_width, char *buf, int size);

/* mm:ss for a duration in minutes.  Hours appear as h:mm:ss above 99 minutes.
 * Negative values are clamped to 0. */
void nt_clock(double minutes, char *buf, int size);

/* "11m15s" under an hour, "1h05m" above: short duration for tables. */
void nt_duration(double minutes, char *buf, int size);

/* Unsigned 64-bit decimal, no printf. */
void nt_u64(uint64_t v, char *buf, int size);

/* Signed 64-bit decimal, no printf. */
void nt_i64(int64_t v, char *buf, int size);

/* Append `src` to `dst` if it fits; always NUL terminates. */
void nt_append(char *dst, int size, char const *src);

/* "x 839 x 1471" style factor list, or "prime". */
void nt_factors(uint64_t const *factors, uint8_t const *exponents, int count,
                char *buf, int size);

#endif /* BENCH_NUMTEXT_H */
