/* graph.h - plotting measured points with their errors (V2 feature F8).
 *
 * Spec: the V2 master prompt, F8.  A lab graph is a scatter of points with error
 * bars, a line of best fit, and the two numbers that go in the conclusion: the
 * gradient and its uncertainty.  The uncertainty is the standard error of the
 * fit, not a guess:
 *
 *      s      = sqrt( sum of (y - fit)^2 / (n - 2) )
 *      dm     = s / sqrt( Sxx )            Sxx = sum of (x - mean x)^2
 *      dc     = s sqrt( 1/n + mean x^2 / Sxx )
 *
 * Two points leave nothing to check the fit against, so the errors come back as
 * zero and `n < 3` is reported by graph_fit() as a warning flag.
 *
 * No gint, no libm.
 */

#ifndef BENCH_GRAPH_H
#define BENCH_GRAPH_H

#include <stdbool.h>

#define GRAPH_MAX_POINTS 12

typedef struct {
    double x, y, err;   /* err is the half-height of the bar */
} graph_point_t;

typedef struct {
    double slope, intercept;
    double r2;
    double slope_err, intercept_err;
    int n;
    bool errors_known;  /* false when n < 3: no residual to measure */
} graph_fit_t;

bool graph_fit(graph_point_t const *pts, int n, graph_fit_t *out);

/* A round axis range that contains [lo, hi]: the smallest of 1, 2, 2.5, 5 x
 * 10^k steps that gives at most `max_ticks` intervals.  A zero-width range is
 * widened by one. */
void graph_range(double lo, double hi, int max_ticks, double *out_lo,
                 double *out_hi, double *out_step);

/* Where a value sits between the two ends of an axis, in pixels: 0 at lo, len
 * at hi.  Used by the screen so the mapping is in core, and testable. */
int graph_project(double value, double lo, double hi, int len);

/* The best and worst a measured y can be, for the ends of an error bar. */
double graph_bar_low(graph_point_t const *p);
double graph_bar_high(graph_point_t const *p);

#endif /* BENCH_GRAPH_H */
