/* test_stats.c - statistics with working (V2 feature F7).
 *
 * The vectors are the master prompt's: 2 4 4 4 5 5 7 9 has mean 5, median 4.5,
 * population SD 2 and sample SD 2.1381; the five points give slope 0.6,
 * intercept 2.2 and r = 0.7746.
 */

#include "test_util.h"

#include "stats.h"

static double const SAMPLE[] = { 2, 4, 4, 4, 5, 5, 7, 9 };
static double const PX[] = { 1, 2, 3, 4, 5 };
static double const PY[] = { 2, 4, 5, 4, 5 };

static void test_summary(void)
{
    stats_summary_t s;

    T_TRUE(stats_summary(SAMPLE, 8, STATS_QUART_MEDIAN, &s));
    T_INT(s.n, 8);
    T_NEAR(s.sum, 40, 1e-12);
    T_NEAR(s.mean, 5, 1e-12);
    T_NEAR(s.median, 4.5, 1e-12);
    T_NEAR(s.sd_pop, 2, 1e-12);
    T_NEAR(s.sd_sample, 2.1380899352993947, 1e-9);
    T_NEAR(s.var_pop, 4, 1e-12);
    T_NEAR(s.sd_sample * s.sd_sample, 32.0 / 7.0, 1e-9);

    T_NEAR(s.min, 2, 1e-12);
    T_NEAR(s.max, 9, 1e-12);
    T_NEAR(s.range, 7, 1e-12);

    /* 4 appears three times and nothing else comes close. */
    T_NEAR(s.mode, 4, 1e-12);
    T_INT(s.mode_count, 3);
    T_TRUE(s.mode_unique);

    /* The median method: halves of four, medians 4 and 6. */
    T_NEAR(s.q1, 4, 1e-12);
    T_NEAR(s.q3, 6, 1e-12);
    T_NEAR(s.iqr, 2, 1e-12);
    T_NEAR(s.fence_low, 1, 1e-12);
    T_NEAR(s.fence_high, 9, 1e-12);
    T_INT(s.outliers, 0);

    /* The spreadsheet method: (n-1)p + 1 interpolates instead. */
    T_TRUE(stats_summary(SAMPLE, 8, STATS_QUART_LINEAR, &s));
    T_NEAR(s.q1, 4, 1e-12);
    T_NEAR(s.q3, 5.5, 1e-12);
    T_NEAR(s.iqr, 1.5, 1e-12);

    /* Empty lists are refused, not summarised as zero. */
    T_FALSE(stats_summary(SAMPLE, 0, STATS_QUART_MEDIAN, &s));
}

static void test_mode(void)
{
    double mode;
    int count;
    bool unique;
    double const tie[] = { 1, 1, 2, 2, 3 };

    stats_mode(tie, 5, &mode, &count, &unique);
    T_NEAR(mode, 1, 0);
    T_INT(count, 2);
    T_FALSE(unique);

    /* A list where everything appears once has no unique mode. */
    stats_mode(PX, 5, &mode, &count, &unique);
    T_INT(count, 1);
    T_FALSE(unique);
}

static void test_quartile_edges(void)
{
    stats_summary_t s;
    double const three[] = { 1, 2, 3 };
    double const two[] = { 1, 2 };

    T_TRUE(stats_summary(three, 3, STATS_QUART_LINEAR, &s));
    T_NEAR(s.q1, 1.5, 1e-12);
    T_NEAR(s.q3, 2.5, 1e-12);
    T_NEAR(s.median, 2, 1e-12);

    T_TRUE(stats_summary(two, 2, STATS_QUART_LINEAR, &s));
    T_NEAR(s.median, 1.5, 1e-12);
    T_NEAR(s.q1, 1.25, 1e-12);
    T_NEAR(s.q3, 1.75, 1e-12);
}

static void test_regression(void)
{
    stats_line_t line;

    T_TRUE(stats_regression(PX, PY, 5, &line));
    T_NEAR(line.slope, 0.6, 1e-12);
    T_NEAR(line.intercept, 2.2, 1e-12);
    T_NEAR(line.r, 0.7745966692414834, 1e-12);
    T_NEAR(line.r2, 0.6, 1e-12);
    T_NEAR(stats_predict(&line, 0), 2.2, 1e-12);
    T_NEAR(stats_predict(&line, 6), 5.8, 1e-12);

    /* A perfect line has r = 1 and no residual. */
    {
        double const x[] = { 1, 2, 3, 4 };
        double const y[] = { 3, 5, 7, 9 };

        T_TRUE(stats_regression(x, y, 4, &line));
        T_NEAR(line.slope, 2, 1e-12);
        T_NEAR(line.intercept, 1, 1e-12);
        T_NEAR(line.r, 1, 1e-12);
    }

    /* Vertical points have no gradient to report. */
    {
        double const x[] = { 2, 2, 2 };
        double const y[] = { 1, 2, 3 };

        T_FALSE(stats_regression(x, y, 3, &line));
    }
}

static void test_outliers(void)
{
    double const with_one[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 40 };

    T_INT(stats_count_outliers(with_one, 10, STATS_QUART_LINEAR), 1);
    T_INT(stats_count_outliers(SAMPLE, 8, STATS_QUART_MEDIAN), 0);
}

static void run_tests(void)
{
    T_SECTION("the prompt's list");
    test_summary();
    T_SECTION("mode and ties");
    test_mode();
    T_SECTION("quartiles on tiny lists");
    test_quartile_edges();
    T_SECTION("least squares");
    test_regression();
    T_SECTION("outlier fences");
    test_outliers();
}

T_MAIN("test_stats")
