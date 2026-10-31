/* test_mathfn.c - the pure-C maths the expression engine stands on.
 *
 * The vectors are the textbook values, compared against libm (the test build
 * links it; core does not) to a tight tolerance, so a change in the series or
 * the range reduction has to stay accurate. */

#include <math.h>

#include "mathfn.h"
#include "test_util.h"

#define REL 1e-14

static void test_sqrt(void)
{
    T_SECTION("square root");

    T_EXACT(bm_sqrt(0.0), 0.0);
    T_EXACT(bm_sqrt(1.0), 1.0);
    T_EXACT(bm_sqrt(4.0), 2.0);
    T_EXACT(bm_sqrt(1000000.0), 1000.0);
    T_NEAR(bm_sqrt(2.0), 1.4142135623730951, REL);
    T_NEAR(bm_sqrt(3.0), 1.7320508075688772, REL);
    T_NEAR(bm_sqrt(0.5), 0.7071067811865476, REL);
    T_NEAR(bm_sqrt(1e-300), sqrt(1e-300), 1e-300);
    T_NEAR(bm_sqrt(1e300), sqrt(1e300), 1e-286);

    /* Subnormal input still lands on the root. */
    T_NEAR(bm_sqrt(4.9e-324), sqrt(4.9e-324), 1e-170);

    T_TRUE(bm_isnan(bm_sqrt(-1.0)));
    T_CHECK(isinf(bm_sqrt(bm_inf())), "sqrt(inf) is inf");
}

static void test_exp_ln(void)
{
    T_SECTION("exp and ln");

    T_EXACT(bm_exp(0.0), 1.0);
    T_NEAR(bm_exp(1.0), BM_E, REL);
    T_NEAR(bm_exp(-1.0), 1.0 / BM_E, REL);
    T_NEAR(bm_exp(10.0), 22026.465794806718, 1e-9);
    T_NEAR(bm_exp(-10.0), 4.5399929762484854e-05, 1e-19);
    T_NEAR(bm_exp(0.5), 1.6487212707001281, REL);
    T_NEAR(bm_exp(700.0), exp(700.0), 1e293);
    T_TRUE(!bm_isfinite(bm_exp(710.0)));
    T_EXACT(bm_exp(-800.0), 0.0);

    T_EXACT(bm_ln(1.0), 0.0);
    T_NEAR(bm_ln(BM_E), 1.0, REL);
    T_NEAR(bm_ln(10.0), 2.302585092994046, REL);
    T_NEAR(bm_ln(2.0), 0.6931471805599453, REL);
    T_NEAR(bm_ln(0.5), -0.6931471805599453, REL);
    T_NEAR(bm_ln(1e-300), log(1e-300), 1e-12);
    T_NEAR(bm_ln(1e300), log(1e300), 1e-12);
    T_TRUE(bm_isnan(bm_ln(-2.0)));
    T_TRUE(!bm_isfinite(bm_ln(0.0)));
    T_TRUE(bm_ln(0.0) < 0.0);

    T_NEAR(bm_log10(1000.0), 3.0, REL);
    T_NEAR(bm_log10(0.01), -2.0, REL);

    /* Round trip: exp(ln x) is x, to the accuracy of both. */
    T_NEAR(bm_exp(bm_ln(42.0)), 42.0, 1e-13);
}

static void test_pow(void)
{
    T_SECTION("powers");

    T_EXACT(bm_pow(2.0, 10.0), 1024.0);
    T_EXACT(bm_pow(2.0, 0.0), 1.0);
    T_EXACT(bm_pow(7.0, 0.0), 1.0);
    T_EXACT(bm_pow(-2.0, 3.0), -8.0);
    T_EXACT(bm_pow(-2.0, 2.0), 4.0);
    T_EXACT(bm_pow(2.0, -2.0), 0.25);
    T_NEAR(bm_pow(2.0, 0.5), 1.4142135623730951, REL);
    T_NEAR(bm_pow(9.0, 0.5), 3.0, REL);
    T_NEAR(bm_pow(27.0, 1.0 / 3.0), 3.0, 1e-12);
    T_NEAR(bm_pow(1.5, 2.5), pow(1.5, 2.5), REL);

    /* A negative base with a fractional exponent is not real. */
    T_TRUE(bm_isnan(bm_pow(-8.0, 1.0 / 3.0)));
    T_EXACT(bm_pow(0.0, 2.0), 0.0);
    T_TRUE(!bm_isfinite(bm_pow(0.0, -1.0)));
    T_EXACT(bm_pow(5.0, 3.0), 125.0);
}

static void test_trig(void)
{
    T_SECTION("sine, cosine, tangent");

    T_EXACT(bm_sin(0.0), 0.0);
    T_EXACT(bm_cos(0.0), 1.0);
    T_NEAR(bm_sin(BM_PI / 6.0), 0.5, REL);
    T_NEAR(bm_cos(BM_PI / 3.0), 0.5, REL);
    T_NEAR(bm_sin(BM_HALF_PI), 1.0, REL);
    T_NEAR(bm_cos(BM_HALF_PI), 0.0, 1e-15);
    T_NEAR(bm_sin(BM_PI), 0.0, 1e-15);
    T_NEAR(bm_cos(BM_PI), -1.0, REL);
    T_NEAR(bm_sin(BM_PI / 4.0), 0.7071067811865476, REL);
    T_NEAR(bm_cos(BM_PI / 4.0), 0.7071067811865476, REL);
    T_NEAR(bm_tan(BM_PI / 4.0), 1.0, REL);
    T_NEAR(bm_sin(-BM_PI / 2.0), -1.0, REL);
    T_NEAR(bm_cos(2.0 * BM_PI), 1.0, 1e-15);

    /* Degrees: the home screen and the tools all use bm_deg_to_rad. */
    T_NEAR(bm_sin(bm_deg_to_rad(30.0)), 0.5, REL);
    T_NEAR(bm_cos(bm_deg_to_rad(60.0)), 0.5, REL);
    T_NEAR(bm_tan(bm_deg_to_rad(45.0)), 1.0, REL);
    T_NEAR(bm_sin(bm_deg_to_rad(180.0)), 0.0, 1e-15);
    T_NEAR(bm_rad_to_deg(BM_PI), 180.0, REL);

    /* A pole is a domain error, not a huge number. */
    T_TRUE(bm_isnan(bm_tan(BM_HALF_PI)));
    T_TRUE(bm_isnan(bm_tan(3.0 * BM_HALF_PI)));

    /* Far from the origin the reduction is still usable at screen precision. */
    T_NEAR(bm_sin(1000.0), sin(1000.0), 1e-13);
    T_NEAR(bm_cos(1000.0), cos(1000.0), 1e-13);

    T_TRUE(bm_isnan(bm_sin(bm_inf())));
}

static void test_pow2(void)
{
    T_SECTION("powers of two");

    T_EXACT(bm_pow2(0), 1.0);
    T_EXACT(bm_pow2(10), 1024.0);
    T_EXACT(bm_pow2(-10), 1.0 / 1024.0);
    T_EXACT(bm_pow2(1023), 8.98846567431158e307);
    T_TRUE(!bm_isfinite(bm_pow2(1024)));
    T_EXACT(bm_pow2(-1074), 4.9e-324);
    T_EXACT(bm_pow2(-2000), 0.0);
}

static void test_predicates(void)
{
    T_SECTION("predicates");

    T_TRUE(bm_isnan(bm_nan()));
    T_FALSE(bm_isfinite(bm_nan()));
    T_FALSE(bm_isfinite(bm_inf()));
    T_TRUE(bm_isfinite(1.0));
    T_TRUE(bm_isfinite(0.0));
    T_TRUE(bm_inf() > 1e308);
    T_TRUE(bm_nan() != bm_nan());
}

static void run_tests(void)
{
    test_sqrt();
    test_exp_ln();
    test_pow();
    test_trig();
    test_pow2();
    test_predicates();
}

T_MAIN("test_mathfn")
