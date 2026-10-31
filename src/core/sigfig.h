/* sigfig.h - significant figures, standard form and SI prefixes (V2 F9).
 *
 * Spec: the V2 master prompt, F9.  The rounding and the digits themselves live
 * in numfmt.c (exact decimal digits, half-up); this file adds the two things a
 * student is actually asked about: how many significant figures a typed number
 * has, and how to write a value in standard form or with an SI prefix.
 *
 * Counting is done on the text, not the value, because the trailing zeros are
 * the whole point: 4.560 has four and 4.56 has three, and no double knows the
 * difference.
 */

#ifndef BENCH_SIGFIG_H
#define BENCH_SIGFIG_H

/* Significant figures of a typed decimal number: leading zeros never count,
 * trailing zeros count only after a decimal point, and an exponent does not
 * change the count.  Returns -1 when the text is not a number. */
int sf_count_text(char const *text);

/* Standard form: "1.23 x 10^3", "-4.56 x 10^-3", "0" for zero.  `sig` is
 * rounded half-up.  Returns the length, or a negative NUMFMT-style error. */
int sf_standard(double value, int sig, char *buf, int size);

/* The SI prefix for a power of ten that is a multiple of three, -12..12.
 * Returns "" outside that range. */
char const *sf_si_prefix(int power);

/* A value with the nearest engineering prefix: 0.00456 -> "4.56 mA".
 * `snap` decides whether a power exactly on a boundary moves up (0650 -> 0.650
 * k when false, 650 when true).  Returns the length. */
int sf_si(double value, int sig, char *buf, int size);

/* The power of ten a value scaled to [1, 1000) by a multiple of three. */
int sf_si_power(double value);

#endif /* BENCH_SIGFIG_H */
