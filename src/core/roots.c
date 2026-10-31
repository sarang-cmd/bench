/* roots.c - bisection, Newton and the secant method (V2 feature F4).
 *
 * Every iterate is kept, not just the answer: the table of n, x, f(x) and the
 * step size is the point of the tool.  The arrays are fixed, so the RAM use is
 * the same for every equation.
 */

#include "roots.h"

#include <stdbool.h>
#include <string.h>

#include "expr.h"

static double absd(double v)
{
    return v < 0.0 ? -v : v;
}

static int is_bad(double v)
{
    /* NaN is the only value that is not equal to itself. */
    return v != v;
}

/* Evaluate `f` at x.  Returns false and sets out->column on a parse error. */
static bool at(char const *f, double x, double *fx, root_result_t *out)
{
    expr_result_t r = expr_eval(f, x, EXPR_RAD);

    if (r.status != EXPR_OK)
    {
        if (out)
        {
            out->status = ROOT_ERR_EXPR;
            out->column = r.column;
            out->count = 0;
            out->root = 0.0;
        }
        return false;
    }
    *fx = r.value;
    return true;
}

static void record(root_result_t *out, int n, double x, double fx, double d)
{
    if (!out || out->count >= ROOT_MAX_STEPS)
        return;
    root_step_t *s = &out->step[out->count++];

    s->n = n;
    s->x = x;
    s->fx = fx;
    s->delta = d;
}

static root_status_t finish(root_result_t *out, root_status_t status,
                            double root)
{
    if (out)
    {
        out->status = status;
        out->root = root;
    }
    return status;
}

/* ---- bisection ---------------------------------------------------------- */

root_status_t root_bisect_max(char const *f, double a, double b, double tol,
                              int max_iter, root_result_t *out)
{
    double fa, fb, mid = a, fm;
    int n;

    if (out)
    {
        memset(out, 0, sizeof *out);
        out->status = ROOT_OK;
    }
    if (tol <= 0.0 || a == b)
        return finish(out, ROOT_ERR_BAD_INTERVAL, 0.0);
    if (!at(f, a, &fa, out) || !at(f, b, &fb, out))
        return ROOT_ERR_EXPR;

    if (absd(fa) < tol)
    {
        record(out, 1, a, fa, 0.0);
        return finish(out, ROOT_OK, a);
    }
    if (absd(fb) < tol)
    {
        record(out, 1, b, fb, 0.0);
        return finish(out, ROOT_OK, b);
    }
    if ((fa < 0.0) == (fb < 0.0))
        return finish(out, ROOT_ERR_BRACKET, 0.0);

    for (n = 1; n <= max_iter; n++)
    {
        mid = a + (b - a) / 2.0;
        if (!at(f, mid, &fm, out))
            return ROOT_ERR_EXPR;
        record(out, n, mid, fm, (b - a) / 2.0);
        if (absd(fm) < tol || (b - a) / 2.0 < tol)
            return finish(out, ROOT_OK, mid);
        if ((fm < 0.0) == (fa < 0.0))
        {
            a = mid;
            fa = fm;
        }
        else
            b = mid;
    }
    return finish(out, ROOT_ERR_NO_CONVERGE, mid);
}

root_status_t root_bisect(char const *f, double a, double b, double tol,
                          root_result_t *out)
{
    return root_bisect_max(f, a, b, tol, ROOT_MAX_ITER, out);
}

/* ---- Newton ------------------------------------------------------------- */

root_status_t root_newton_max(char const *f, double x0, double tol,
                              int max_iter, root_result_t *out)
{
    double x = x0, fx, hi, lo, d, next, fnext, delta;
    int n;

    if (out)
    {
        memset(out, 0, sizeof *out);
        out->status = ROOT_OK;
    }
    if (tol <= 0.0)
        return finish(out, ROOT_ERR_BAD_INTERVAL, 0.0);
    if (!at(f, x, &fx, out))
        return ROOT_ERR_EXPR;
    if (absd(fx) < tol)
        return finish(out, ROOT_OK, x);

    for (n = 1; n <= max_iter; n++)
    {
        /* Central difference: a slope from two nearby points, which is what a
         * calculator can always do, since the student may not have typed the
         * derivative. */
        if (!at(f, x + ROOT_H, &hi, out) || !at(f, x - ROOT_H, &lo, out))
            return ROOT_ERR_EXPR;
        d = (hi - lo) / (2.0 * ROOT_H);
        if (absd(d) < 1e-12)
            return finish(out, ROOT_ERR_FLAT, x);
        next = x - fx / d;
        if (is_bad(next))
            return finish(out, ROOT_ERR_NO_CONVERGE, x);
        if (!at(f, next, &fnext, out))
            return ROOT_ERR_EXPR;
        delta = absd(next - x);
        record(out, n, next, fnext, delta);
        x = next;
        fx = fnext;
        if (absd(fx) < tol || delta < tol)
            return finish(out, ROOT_OK, x);
    }
    return finish(out, ROOT_ERR_NO_CONVERGE, x);
}

root_status_t root_newton(char const *f, double x0, double tol,
                          root_result_t *out)
{
    return root_newton_max(f, x0, tol, ROOT_MAX_ITER, out);
}

/* ---- secant ------------------------------------------------------------- */

root_status_t root_secant(char const *f, double x0, double x1, double tol,
                          root_result_t *out)
{
    double fa, fb, next, fnext;
    int n;

    if (out)
    {
        memset(out, 0, sizeof *out);
        out->status = ROOT_OK;
    }
    if (tol <= 0.0 || x0 == x1)
        return finish(out, ROOT_ERR_BAD_INTERVAL, 0.0);
    if (!at(f, x0, &fa, out) || !at(f, x1, &fb, out))
        return ROOT_ERR_EXPR;

    for (n = 1; n <= ROOT_MAX_ITER; n++)
    {
        if (absd(fb - fa) < 1e-15)
            return finish(out, ROOT_ERR_FLAT, x1);
        next = x1 - fb * (x1 - x0) / (fb - fa);
        if (is_bad(next))
            return finish(out, ROOT_ERR_NO_CONVERGE, x1);
        if (!at(f, next, &fnext, out))
            return ROOT_ERR_EXPR;
        record(out, n, next, fnext, absd(next - x1));
        x0 = x1;
        fa = fb;
        x1 = next;
        fb = fnext;
        if (absd(fb) < tol || absd(next - x0) < tol)
            return finish(out, ROOT_OK, x1);
    }
    return finish(out, ROOT_ERR_NO_CONVERGE, x1);
}

/* ---- text --------------------------------------------------------------- */

char const *root_status_text(root_status_t status)
{
    switch (status)
    {
    case ROOT_OK: return "converged";
    case ROOT_ERR_EXPR: return "check the expression";
    case ROOT_ERR_BRACKET: return "no sign change in the bracket";
    case ROOT_ERR_FLAT: return "zero slope: no step possible";
    case ROOT_ERR_NO_CONVERGE: return "stopped after 50 steps";
    case ROOT_ERR_BAD_INTERVAL: return "the interval or tolerance is unusable";
    }
    return "unknown";
}
