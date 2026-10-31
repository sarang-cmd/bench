/* stats.h - statistics with working (V2 feature F7).
 *
 * Spec: the V2 master prompt, F7.  One list of numbers, the summary a student
 * has to write down, and the regression behind paired data.
 *
 * The quartile method is a setting, so both are here:
 *   STATS_QUART_MEDIAN   the median of each half, halves split through the
 *                        median when n is odd (the "exclusive" method)
 *   STATS_QUART_LINEAR   position (n-1)p + 1, interpolated (the "inclusive"
 *                        method, which is what a spreadsheet does)
 *
 * No gint and no libm: the square roots come from core/mathfn.c.
 */

#ifndef BENCH_STATS_H
#define BENCH_STATS_H

#include <stdbool.h>

/* The list cap.  200 values is the prompt's number and 1.6 KiB of static RAM. */
#define STATS_MAX 200

typedef enum {
    STATS_QUART_MEDIAN = 0,
    STATS_QUART_LINEAR,
} stats_quart_t;

typedef struct {
    int n;
    double sum;
    double mean;
    double median;
    double mode;      /* the value that occurs most often */
    int mode_count;   /* how often; 1 means every value is equally common */
    bool mode_unique;
    double min, max, range;
    double q1, q3, iqr;
    double fence_low, fence_high;  /* q1 - 1.5 IQR, q3 + 1.5 IQR */
    int outliers;
    double sd_pop, sd_sample;
    double var_pop, var_sample;
} stats_summary_t;

/* Fills `out`.  n < 1 leaves every field 0 and returns false.  `values` is
 * left untouched: the caller's order is the student's order. */
bool stats_summary(double const *values, int n, stats_quart_t quart,
                   stats_summary_t *out);

/* Paired data: y = a + bx, Pearson r, and the coefficient of determination. */
typedef struct {
    int n;
    double slope, intercept;
    double r, r2;
    double mean_x, mean_y;
    double sx, sy;      /* sample standard deviations, for the by-hand formula */
} stats_line_t;

bool stats_regression(double const *x, double const *y, int n, stats_line_t *out);

/* Value of the least squares line at `x`. */
double stats_predict(stats_line_t const *line, double x);

/* Number of values further than 1.5 IQR outside the quartiles. */
int stats_count_outliers(double const *values, int n, stats_quart_t quart);

/* The mode of a list: the most frequent value, and how often it occurs.  Ties
 * keep the smallest value and set `unique` to false. */
void stats_mode(double const *values, int n, double *mode, int *count,
                bool *unique);

#endif /* BENCH_STATS_H */
