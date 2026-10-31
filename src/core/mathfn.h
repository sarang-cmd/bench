/* mathfn.h - the handful of maths the expression engine needs, in plain C.
 *
 * Spec: docs/plan.md (the expression engine in A5) and AGENTS.md: src/core
 * takes no gint and no libm, so the calculator build and the host tests run the
 * same code.  That is why these are here and not <math.h>: the SuperH build
 * links a libm this project would rather not depend on, and a pure-C version is
 * testable with the same harness as everything else in core.
 *
 * Accuracy: every function is good to about 1e-15 relative on the range the
 * add-in uses (|x| < 1e6 for the trigonometric functions).  Domain errors are
 * not exceptions: they return bm_nan() or bm_inf(), and the caller decides
 * what to say.
 */

#ifndef BENCH_MATHFN_H
#define BENCH_MATHFN_H

#include <stdbool.h>

/* Constants, to the digits a double can hold. */
#define BM_PI 3.14159265358979323846
#define BM_HALF_PI 1.57079632679489661923
#define BM_LN2 0.69314718055994530942
#define BM_E 2.71828182845904523536

double bm_nan(void);
double bm_inf(void);
bool bm_isnan(double x);
bool bm_isfinite(double x);

/* 2^n, for -1074 <= n <= 1023; outside that, 0 or bm_inf(). */
double bm_pow2(int n);

double bm_sqrt(double x);   /* negative -> NaN */
double bm_exp(double x);
double bm_ln(double x);     /* x <= 0 -> -Inf for 0, NaN below */
double bm_log10(double x);
double bm_pow(double x, double y);
double bm_sin(double x);
double bm_cos(double x);
double bm_tan(double x);    /* NaN at an odd multiple of pi/2 */

/* Both at once: one range reduction instead of two. */
void bm_sincos(double x, double *s, double *c);

/* Inverse trigonometric functions, in radians.  Outside their domain they
 * return NaN (asin and acos beyond 1) rather than a wrong number. */
double bm_atan(double x);
double bm_asin(double x);
double bm_acos(double x);

/* Hyperbolics, for the expression engine's sinh, cosh and tanh. */
double bm_sinh(double x);
double bm_cosh(double x);
double bm_tanh(double x);

/* Rounding to whole numbers; values beyond 2^62 are returned unchanged,
 * because every double that large is already whole. */
double bm_floor(double x);
double bm_ceil(double x);

/* Degrees and radians are both the add-in's business: the expression engine
 * converts with these so every tool agrees on the definition of a degree. */
double bm_deg_to_rad(double deg);
double bm_rad_to_deg(double rad);

#endif /* BENCH_MATHFN_H */
