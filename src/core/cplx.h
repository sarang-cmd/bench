/* cplx.h - complex numbers (V2 F6).
 *
 * Spec: the V2 master prompt, F6.  The arithmetic a student needs: add,
 * subtract, multiply, divide, conjugate, modulus, argument, polar form and a
 * whole power.  No complex logarithm or square root: those belong to a course,
 * not to a keypad.
 *
 * Plain C11, no gint. */

#ifndef BENCH_CPLX_H
#define BENCH_CPLX_H

#include <stdbool.h>

typedef struct {
    double re;
    double im;
} cplx_t;

cplx_t cx(double re, double im);

cplx_t cplx_add(cplx_t a, cplx_t b);
cplx_t cplx_sub(cplx_t a, cplx_t b);
cplx_t cplx_mul(cplx_t a, cplx_t b);
/* Dividing by zero gives NaN, not an exception: the screen says "not a
 * number" rather than dividing the calculator by zero. */
cplx_t cplx_div(cplx_t a, cplx_t b);
cplx_t cplx_conj(cplx_t a);
/* a^n for a whole n, by repeated multiplication (|n| <= 64). */
cplx_t cplx_powi(cplx_t a, int n);

double cplx_abs(cplx_t a);
/* In radians, -pi..pi. */
double cplx_arg(cplx_t a);
/* r(cos t + i sin t) */
cplx_t cplx_polar(double r, double theta);

bool cplx_is_nan(cplx_t a);

/* "3 + 4i", "3 - 4i", "-i", "3", "1.5 + 0.5i" with `sig` significant digits.
 * Returns the length, or a negative error code. */
int cplx_format(cplx_t a, int sig, char *buf, int size);

#endif /* BENCH_CPLX_H */
