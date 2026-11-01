/* pi.h - the digits of pi, and how far a guess got.
 *
 * Spec: the "pi game" request.  The game needs two things that are not game
 * logic at all: a trustworthy table of digits, and a way to compare a typed
 * string against it from an arbitrary position.  Both live here so they can be
 * tested without a screen - and the table is checked in tests/test_pi.c against
 * a spigot computation written from the algorithm, not against a copy of
 * itself.
 *
 * Plain C11, no gint. */

#ifndef BENCH_PI_H
#define BENCH_PI_H

/* Digits the table holds, counting the leading 3.  200. */
int pi_digit_count(void);

/* The digit at index `i`, where 0 is the "3" before the point.  -1 past the
 * end, so a caller cannot read whatever follows the table in memory. */
int pi_digit(int i);

/* How many digits of `text` (length `len`) are right when the guess starts at
 * index `from`.  Stops at the first wrong digit; 0 when the first is wrong, the
 * text is empty, or `from` is past the end. */
int pi_match(int from, char const *text, int len);

/* The first `count` digits as a reader expects them: "3.", then the decimals in
 * groups of five - "3.14159 26535 89793".  Returns the length written, or -1 on
 * a bad count or a buffer that is too small (never a truncated string). */
int pi_text(int count, char *out, int size);

#endif /* BENCH_PI_H */
