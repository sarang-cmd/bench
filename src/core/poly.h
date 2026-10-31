/* poly.h - polynomials with real coefficients (V2 F3).
 *
 * Spec: the V2 master prompt, F3.  Full computer algebra is out of scope; this
 * is the arithmetic a student needs: add, subtract, multiply, expand a
 * binomial, divide with a quotient and a remainder, a quadratic with real or
 * complex roots, and factoring over the rationals up to degree 6.
 *
 * Coefficients are doubles, indexed from the constant term up:
 *     2x^3 - 5x + 1  ->  c = {1, -5, 0, 2}, deg = 3
 * The zero polynomial has deg = 0 and c[0] = 0.
 */

#ifndef BENCH_POLY_H
#define BENCH_POLY_H

#define POLY_MAX_DEG 8
#define POLY_COEFFS (POLY_MAX_DEG + 1)

typedef struct {
    int deg;                        /* 0 for a constant */
    double c[POLY_COEFFS];          /* c[0] is the constant term */
} poly_t;

#define POLY_OK 0
#define POLY_ERR_SYNTAX (-1)
#define POLY_ERR_DEGREE (-2)        /* too high for POLY_MAX_DEG */
#define POLY_ERR_DIV_ZERO (-3)      /* dividing by the zero polynomial */
#define POLY_ERR_SIZE (-4)          /* output buffer too small */

/* Text to polynomial: "2x^3 - 5x + 1", "x^2", "x", "5", "-x + 2".
 * Spaces are optional; the power may be missing (x) or 0..8. */
int poly_parse(char const *text, poly_t *out);
/* "2x^3 - 5x + 1", "0" for zero. */
int poly_format(poly_t const *p, char *buf, int size);

void poly_zero(poly_t *p);
void poly_set(poly_t *p, int deg, double const *c);
/* Value at x (Horner). */
double poly_eval(poly_t const *p, double x);

void poly_add(poly_t const *a, poly_t const *b, poly_t *out);
void poly_sub(poly_t const *a, poly_t const *b, poly_t *out);
int poly_mul(poly_t const *a, poly_t const *b, poly_t *out);   /* POLY_ERR_DEGREE */
/* (a*x + b)^n, n <= POLY_MAX_DEG. */
int poly_binomial(double a, double b, int n, poly_t *out);

/* Long division: a = q*b + r, deg r < deg b. */
int poly_div(poly_t const *a, poly_t const *b, poly_t *q, poly_t *r);

/* A quadratic.  disc = b^2 - 4ac; the roots are (re +/- im*i).  With a == 0
 * this is a line: one root, im = 0, and POLY_ERR_SYNTAX is returned. */
int poly_quadratic(double a, double b, double c, double *disc, double *r1re,
                   double *r1im, double *r2re, double *r2im);

/* Rational roots of a polynomial with whole-number coefficients: every root
 * p/q found goes into roots[0..*count), and `rest` is what is left.  The
 * search is the rational root theorem with divisors up to 4000. */
#define POLY_MAX_ROOTS POLY_MAX_DEG
int poly_factor(poly_t const *p, double roots[POLY_MAX_ROOTS], int *count,
                poly_t *rest);

#endif /* BENCH_POLY_H */
