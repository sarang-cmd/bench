/* test_graph.c - measured points with errors (V2 feature F8).
 *
 * The points are a Hooke's law run: x in cm, y in N, with a half-unit error
 * bar.  The gradient is 0.5 N/cm and the fit should say so.
 */

#include "test_util.h"

#include "graph.h"

static graph_point_t const HOOKE[] = {
    { 1, 0.5, 0.1 }, { 2, 1.0, 0.1 }, { 3, 1.5, 0.1 },
    { 4, 2.0, 0.1 }, { 5, 2.5, 0.1 },
};

/* A deliberately imperfect line, so the residuals are not all zero. */
static graph_point_t const WOBBLY[] = {
    { 1, 2.1, 0.2 }, { 2, 4.0, 0.2 }, { 3, 5.2, 0.2 }, { 4, 8.1, 0.2 },
};

static void test_perfect(void)
{
    graph_fit_t f;

    T_TRUE(graph_fit(HOOKE, 5, &f));
    T_NEAR(f.slope, 0.5, 1e-12);
    T_NEAR(f.intercept, 0.0, 1e-12);
    T_NEAR(f.r2, 1.0, 1e-12);
    T_INT(f.n, 5);
    T_TRUE(f.errors_known);
    /* No residual, so no uncertainty in the fit. */
    T_NEAR(f.slope_err, 0.0, 1e-12);
    T_NEAR(f.intercept_err, 0.0, 1e-12);
}

static void test_wobbly(void)
{
    graph_fit_t f;
    /* Least squares on the four points: Sxy = 9.6 over Sxx = 5, so the
     * slope is 1.92; with means 2.5 and 4.85 the intercept is 0.05. */
    double const slope = 1.92;
    double const intercept = 0.05;

    T_TRUE(graph_fit(WOBBLY, 4, &f));
    T_NEAR(f.slope, slope, 1e-12);
    T_NEAR(f.intercept, intercept, 1e-12);
    T_TRUE(f.r2 > 0.9 && f.r2 < 1.0);
    T_TRUE(f.slope_err > 0);
    T_TRUE(f.intercept_err > 0);

    /* Residuals are under 0.7, so the standard error of the slope lands around
     * 0.23: small enough to quote, large enough that the bar matters. */
    T_TRUE(f.slope_err < 0.5);
}

static void test_two_points(void)
{
    graph_fit_t f;

    T_TRUE(graph_fit(HOOKE, 2, &f));
    T_NEAR(f.slope, 0.5, 1e-12);
    T_FALSE(f.errors_known);
    T_NEAR(f.slope_err, 0.0, 1e-12);
}

static void test_refusals(void)
{
    graph_fit_t f;
    graph_point_t const vertical[] = { { 2, 1, 0 }, { 2, 5, 0 }, { 2, 9, 0 } };

    T_FALSE(graph_fit(HOOKE, 1, &f));
    T_FALSE(graph_fit(vertical, 3, &f));
    T_FALSE(graph_fit(NULL, 3, &f));
}

static void test_bars(void)
{
    T_NEAR(graph_bar_low(&HOOKE[0]), 0.4, 1e-12);
    T_NEAR(graph_bar_high(&HOOKE[0]), 0.6, 1e-12);

    {
        graph_point_t const negative = { 1, 2, -0.5 };

        T_NEAR(graph_bar_low(&negative), 1.5, 1e-12);
        T_NEAR(graph_bar_high(&negative), 2.5, 1e-12);
    }
}

static void test_range(void)
{
    double lo, hi, step;

    /* 0 .. 5 in steps of one. */
    graph_range(0, 5, 6, &lo, &hi, &step);
    T_NEAR(lo, 0, 1e-12);
    T_NEAR(hi, 5, 1e-12);
    T_NEAR(step, 1, 1e-12);

    /* 0 .. 4.3 needs a step of 1 and tops out at 5. */
    graph_range(0, 4.3, 6, &lo, &hi, &step);
    T_NEAR(lo, 0, 1e-12);
    T_NEAR(hi, 5, 1e-12);
    T_NEAR(step, 1, 1e-12);

    /* A range that starts below zero keeps zero on a tick. */
    graph_range(-3, 3, 6, &lo, &hi, &step);
    T_NEAR(lo, -3, 1e-12);
    T_NEAR(hi, 3, 1e-12);

    /* A flat axis is widened rather than divided by nothing. */
    graph_range(2, 2, 6, &lo, &hi, &step);
    T_TRUE(lo < 2 && hi > 2);
    T_TRUE(step > 0);

    /* 0.0 .. 0.06 in steps of 0.01. */
    graph_range(0, 0.06, 7, &lo, &hi, &step);
    T_NEAR(step, 0.01, 1e-12);
    T_NEAR(hi, 0.06, 1e-12);
}

static void test_project(void)
{
    T_INT(graph_project(0, 0, 10, 100), 0);
    T_INT(graph_project(10, 0, 10, 100), 100);
    T_INT(graph_project(5, 0, 10, 100), 50);
    T_INT(graph_project(0, 0, 0, 100), 0);
}

static void run_tests(void)
{
    T_SECTION("a perfect line");
    test_perfect();
    T_SECTION("a wobbly one, with error bars");
    test_wobbly();
    T_SECTION("two points");
    test_two_points();
    T_SECTION("refusals");
    test_refusals();
    T_SECTION("bar ends");
    test_bars();
    T_SECTION("axis ranges");
    test_range();
    T_SECTION("projection");
    test_project();
}

T_MAIN("test_graph")
