/* numparse.h - read a decimal number typed on the calculator.
 *
 * Why not strtod(): the calculator's C library may or may not format floats
 * (spike question 3) and the decimal value must match what the user typed, so
 * the app uses this small parser instead.  It accepts what the keypad can
 * produce and nothing else:
 *
 *     [-+]digits[.digits][ (e|E)[-+]digits ]
 *
 * Plain C11, no gint, no libm. */

#ifndef BENCH_NUMPARSE_H
#define BENCH_NUMPARSE_H

#include <stdbool.h>

#define NUMPARSE_OK 0
#define NUMPARSE_ERR_EMPTY (-1)     /* nothing to read */
#define NUMPARSE_ERR_SYNTAX (-2)    /* a character that is not part of a number */
#define NUMPARSE_ERR_RANGE (-3)     /* more than 18 digits, or |x| > 1e308 */
#define NUMPARSE_ERR_NOT_FINITE (-4)

/* Parse `len` bytes of `text`.  Trailing spaces are allowed, a NUL inside the
 * range is not.  On error *out is untouched. */
int numparse_double(char const *text, int len, double *out);

/* Parse a NUL terminated string, for convenience in the app. */
int numparse_cstr(char const *text, double *out);

/* True when the text would parse. */
bool numparse_ok(char const *text, int len);

#endif /* BENCH_NUMPARSE_H */
