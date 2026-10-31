/* stats.c - the arithmetic behind the statistics screen (V2 feature F7).
 *
 * Everything sorts a copy of the caller's values: the order the student typed
 * is theirs, and a summary must not silently reorder it.
 */

#include "stats.h"

#include <string.h>

#include "mathfn.h"

static void sort(double *v, int n)
{
    /* Insertion sort: n is at most 200 and this runs once per screen. */
    for (int i = 1; i < n; i++)
    {
        double const key = v[i];
        int j = i - 1;

        while (j >= 0 && v[j] > key)
        {
            v[j + 1] = v[j];
            j--;
        }
        v[j + 1] = key;
    }
}

/* The median of a sorted run; 0 for an empty one. */
static double median_sorted(double const *v, int n)
{
    if (n <= 0)
        return 0.0;
    if (n % 2 == 1)
        return v[n / 2];
    return (v[n / 2 - 1] + v[n / 2]) / 2.0;
}

/* Piecewise-linear centile, position (n-1)p + 1 for p in [0,1]. */
static double centile_linear(double const *v, int n, double p)
{
    double const pos = (double)(n - 1) * p;
    int const lo = (int)pos;
    double const frac = pos - (double)lo;

    if (lo + 1 >= n)
        return v[n - 1];
    return v[lo] + (v[lo + 1] - v[lo]) * frac;
}

void stats_mode(double const *values, int n, double *mode, int *count,
                bool *unique)
{
    double best = 0.0;
    int best_count = 0;
    bool tie = false;

    *mode = 0.0;
    *count = 0;
    *unique = false;
    if (n <= 0)
        return;

    /* One pass per distinct value is fine at 200; the list is not sorted here
     * because the caller's order may matter to them. */
    for (int i = 0; i < n; i++)
    {
        int seen = 0;

        for (int j = 0; j < n; j++)
            if (values[j] == values[i])
                seen++;
        if (seen > best_count || (seen == best_count && values[i] < best))
        {
            tie = seen == best_count;
            best = values[i];
            best_count = seen;
        }
        else if (seen == best_count && values[i] != best)
            tie = true;
    }

    *mode = best;
    *count = best_count;
    *unique = !tie;
}

int stats_count_outliers(double const *values, int n, stats_quart_t quart)
{
    double *sorted;
    double copy[STATS_MAX];
    double q1, q3, iqr;
    int count = 0;

    if (n <= 0 || n > STATS_MAX)
        return 0;
    memcpy(copy, values, (size_t)n * sizeof copy[0]);
    sort(copy, n);
    sorted = copy;

    if (quart == STATS_QUART_LINEAR)
    {
        q1 = centile_linear(sorted, n, 0.25);
        q3 = centile_linear(sorted, n, 0.75);
    }
    else
    {
        int const half = n / 2;

        q1 = median_sorted(sorted, half);
        q3 = median_sorted(sorted + (n - half), half);
    }
    iqr = q3 - q1;
    for (int i = 0; i < n; i++)
        if (sorted[i] < q1 - 1.5 * iqr || sorted[i] > q3 + 1.5 * iqr)
            count++;
    return count;
}

bool stats_summary(double const *values, int n, stats_quart_t quart,
                   stats_summary_t *out)
{
    double copy[STATS_MAX];
    double sumsq = 0.0;

    memset(out, 0, sizeof *out);
    if (!values || n < 1 || n > STATS_MAX)
        return false;
    memcpy(copy, values, (size_t)n * sizeof copy[0]);
    sort(copy, n);

    out->n = n;
    for (int i = 0; i < n; i++)
        out->sum += values[i];
    out->mean = out->sum / (double)n;
    out->median = median_sorted(copy, n);
    stats_mode(values, n, &out->mode, &out->mode_count, &out->mode_unique);

    out->min = copy[0];
    out->max = copy[n - 1];
    out->range = out->max - out->min;

    if (quart == STATS_QUART_LINEAR)
    {
        out->q1 = centile_linear(copy, n, 0.25);
        out->q3 = centile_linear(copy, n, 0.75);
    }
    else
    {
        int const half = n / 2;

        out->q1 = median_sorted(copy, half);
        out->q3 = median_sorted(copy + (n - half), half);
    }
    out->iqr = out->q3 - out->q1;
    out->fence_low = out->q1 - 1.5 * out->iqr;
    out->fence_high = out->q3 + 1.5 * out->iqr;
    out->outliers = stats_count_outliers(values, n, quart);

    for (int i = 0; i < n; i++)
    {
        double const d = values[i] - out->mean;

        sumsq += d * d;
    }
    out->var_pop = sumsq / (double)n;
    out->sd_pop = bm_sqrt(out->var_pop);
    if (n > 1)
    {
        out->var_sample = sumsq / (double)(n - 1);
        out->sd_sample = bm_sqrt(out->var_sample);
    }
    return true;
}

bool stats_regression(double const *x, double const *y, int n, stats_line_t *out)
{
    double sx = 0.0, sy = 0.0, sxx = 0.0, syy = 0.0, sxy = 0.0;

    memset(out, 0, sizeof *out);
    if (!x || !y || n < 2 || n > STATS_MAX)
        return false;

    for (int i = 0; i < n; i++)
    {
        sx += x[i];
        sy += y[i];
        sxx += x[i] * x[i];
        syy += y[i] * y[i];
        sxy += x[i] * y[i];
    }
    out->n = n;
    out->mean_x = sx / (double)n;
    out->mean_y = sy / (double)n;

    {
        double const denom = (double)n * sxx - sx * sx;

        if (denom == 0.0)
            return false;   /* a vertical set of points has no gradient */
        out->slope = ((double)n * sxy - sx * sy) / denom;
        out->intercept = out->mean_y - out->slope * out->mean_x;
    }

    {
        double const dx = (double)n * sxx - sx * sx;
        double const dy = (double)n * syy - sy * sy;

        if (dx > 0.0 && dy > 0.0)
        {
            double const cov = (double)n * sxy - sx * sy;

            out->r = cov / bm_sqrt(dx * dy);
            out->r2 = out->r * out->r;
        }
    }

    if (n > 1)
    {
        double vx = 0.0, vy = 0.0;

        for (int i = 0; i < n; i++)
        {
            double const dx = x[i] - out->mean_x;
            double const dy = y[i] - out->mean_y;

            vx += dx * dx;
            vy += dy * dy;
        }
        out->sx = bm_sqrt(vx / (double)(n - 1));
        out->sy = bm_sqrt(vy / (double)(n - 1));
    }
    return true;
}

double stats_predict(stats_line_t const *line, double x)
{
    return line->intercept + line->slope * x;
}
