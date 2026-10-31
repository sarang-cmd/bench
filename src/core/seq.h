/* seq.h - sequences, series and binomial coefficients (V2 F5).
 *
 * Spec: the V2 master prompt, F5.  Arithmetic and geometric sequences, their
 * nth terms and sums, the sum to infinity, binomial coefficients and Pascal's
 * triangle.  Terms are numbered from 1.
 *
 * Everything is doubles except the binomial coefficient, which is exact: it is
 * refused rather than rounded when it does not fit in 64 bits.
 */

#ifndef BENCH_SEQ_H
#define BENCH_SEQ_H

#include <stdbool.h>
#include <stdint.h>

/* a, a+d, a+2d, ... */
double seq_arith_nth(double a, double d, int n);
double seq_arith_sum(double a, double d, int n);

/* a, ar, ar^2, ... */
double seq_geom_nth(double a, double r, int n);
double seq_geom_sum(double a, double r, int n);
/* The sum to infinity, only when |r| < 1.  False otherwise. */
bool seq_geom_infinite(double a, double r, double *out);

/* n choose k, exact.  False when n < 0, k outside 0..n, or the result does not
 * fit in 64 bits (n above 62). */
bool seq_binom(int n, int k, int64_t *out);

/* Row n of Pascal's triangle into `row` (n+1 entries).  False when n is
 * outside 0..62 or the row does not fit in `size`. */
#define SEQ_PASCAL_MAX 62
bool seq_pascal_row(int n, int64_t *row, int size);

/* The first n terms, for the screen's list.  `out` needs n doubles. */
void seq_arith_terms(double a, double d, int n, double *out);
void seq_geom_terms(double a, double r, int n, double *out);

#endif /* BENCH_SEQ_H */
