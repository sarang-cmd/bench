/* graph.c - see graph.h. */

#include "graph.h"

#include <stddef.h>

#include "mathfn.h"

double graph_bar_low(graph_point_t const *p)
{
    double const e = p->err < 0 ? -p->err : p->err;

    return p->y - e;
}

double graph_bar_high(graph_point_t const *p)
{
    double const e = p->err < 0 ? -p->err : p->err;

    return p->y + e;
}

bool graph_fit(graph_point_t const *pts, int n, graph_fit_t *out)
{
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    double mean_x, mean_y, sxx_dev, ss_res = 0, ss_tot = 0;

    if (!pts || !out || n < 2) return false;
    for (int i = 0; i < n; i++) {
        graph_point_t const *p = &pts[i];

        sx += p->x;
        sy += p->y;
        sxx += p->x * p->x;
        sxy += p->x * p->y;
    }
    mean_x = sx / n;
    mean_y = sy / n;
    sxx_dev = sxx - sx * sx / n;
    if (sxx_dev <= 0) return false;      /* all the x are the same */

    out->n = n;
    out->slope = (sxy - sx * sy / n) / sxx_dev;
    out->intercept = mean_y - out->slope * mean_x;

    for (int i = 0; i < n; i++) {
        double const fit = out->intercept + out->slope * pts[i].x;
        double const res = pts[i].y - fit;

        ss_res += res * res;
        ss_tot += (pts[i].y - mean_y) * (pts[i].y - mean_y);
    }
    out->r2 = ss_tot > 0 ? 1.0 - ss_res / ss_tot : 1.0;

    if (n >= 3) {
        double const s = bm_sqrt(ss_res / (n - 2));

        out->slope_err = s / bm_sqrt(sxx_dev);
        out->intercept_err = s * bm_sqrt(1.0 / n + mean_x * mean_x / sxx_dev);
        out->errors_known = true;
    } else {
        out->slope_err = 0;
        out->intercept_err = 0;
        out->errors_known = false;
    }
    return true;
}

void graph_range(double lo, double hi, int max_ticks, double *out_lo,
                 double *out_hi, double *out_step)
{
    static double const STEPS[] = { 1, 2, 2.5, 5, 10 };
    double span, mag, step = 1;

    if (max_ticks < 2) max_ticks = 2;
    if (hi < lo) {
        double const t = lo;

        lo = hi;
        hi = t;
    }
    if (hi - lo < 1e-12) {
        lo -= 1;
        hi += 1;
    }
    span = hi - lo;
    mag = 1;
    while (span / mag > 1000) mag *= 10;
    while (span / mag < 1) mag /= 10;

    for (int i = 0; i < 5; i++) {
        step = STEPS[i] * mag;
        if (span / step <= max_ticks) break;
    }

    {
        double a = (lo < 0 ? -(double)(int)(-lo / step + 0.999999) : (double)(int)(lo / step)) * step;
        double b = a;

        while (b < hi) b += step;
        while (b - step > hi) b -= step;
        *out_lo = a;
        *out_hi = b;
        *out_step = step;
    }
}

int graph_project(double value, double lo, double hi, int len)
{
    if (hi == lo) return 0;
    return (int)((value - lo) / (hi - lo) * len + 0.5);
}
