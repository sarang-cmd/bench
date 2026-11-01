/* test_percent.c - the percentage questions, with the vectors a paper uses.
 * Spec: src/core/percent.h. */

#include "percent.h"
#include "test_util.h"

static void test_change(void)
{
    double v;

    T_SECTION("percentage change");

    T_INT(pc_change(40, 46, &v), PC_OK);
    T_NEAR(v, 15.0, 1e-9);

    T_INT(pc_change(80, 60, &v), PC_OK);
    T_NEAR(v, -25.0, 1e-9);

    T_INT(pc_change(50, 50, &v), PC_OK);
    T_NEAR(v, 0.0, 1e-12);

    T_INT(pc_change(200, 100, &v), PC_OK);
    T_NEAR(v, -50.0, 1e-9);

    T_INT(pc_change(0, 10, &v), PC_ERR_FROM_ZERO);
    T_INT(pc_change(10, 10, NULL), PC_ERR_ARG);
}

static void test_apply_and_reverse(void)
{
    double v;

    T_SECTION("applying and undoing a percentage");

    T_INT(pc_apply(40, 15, &v), PC_OK);
    T_NEAR(v, 46.0, 1e-9);

    T_INT(pc_apply(40, -15, &v), PC_OK);
    T_NEAR(v, 34.0, 1e-9);

    /* After 15% off, 34 was 40.  This is the one people get wrong by adding
     * 15% of 34 back on. */
    T_INT(pc_reverse(34, -15, &v), PC_OK);
    T_NEAR(v, 40.0, 1e-9);

    T_INT(pc_reverse(120, 20, &v), PC_OK);
    T_NEAR(v, 100.0, 1e-9);

    T_INT(pc_reverse(0, -100, &v), PC_ERR_NO_ORIGINAL);
    T_INT(pc_reverse(5, 0, &v), PC_OK);
    T_NEAR(v, 5.0, 1e-12);
    T_INT(pc_reverse(5, 0, NULL), PC_ERR_ARG);
    T_INT(pc_apply(5, 0, NULL), PC_ERR_ARG);
}

static void test_compound(void)
{
    double v;

    T_SECTION("compound growth and depreciation");

    /* 500 at 4.5% for 7 years: 500 x 1.045^7 = 680.43 (2 dp). */
    T_INT(pc_compound(500, 4.5, 7, &v), PC_OK);
    T_NEAR(v, 680.4309, 1e-3);

    /* Zero years is the principal, untouched. */
    T_INT(pc_compound(500, 4.5, 0, &v), PC_OK);
    T_NEAR(v, 500.0, 1e-12);

    /* One year is the simple case. */
    T_INT(pc_compound(200, 10, 1, &v), PC_OK);
    T_NEAR(v, 220.0, 1e-9);

    /* Depreciation: a car at 12% a year for 5 years keeps 52.7%. */
    T_INT(pc_compound(10000, -12, 5, &v), PC_OK);
    T_NEAR(v, 5279.0, 2.0);

    /* The classic doubling time: 100 at 7% for 10 years. */
    T_INT(pc_compound(100, 7, 10, &v), PC_OK);
    T_NEAR(v, 196.7, 0.1);

    T_INT(pc_compound(100, 5, -1, &v), PC_ERR_PERIODS);
    T_INT(pc_compound(100, 5, 201, &v), PC_ERR_PERIODS);
    T_INT(pc_compound(100, 5, 10, NULL), PC_ERR_ARG);

    /* Growth so fast the answer cannot be shown is a status, not an inf. */
    T_INT(pc_compound(1.0, 1e300, 2, &v), PC_ERR_TOO_BIG);
}

static void test_text(void)
{
    T_SECTION("every status has a sentence");

    T_STR(pc_error_text(PC_ERR_FROM_ZERO),
          "no change from zero can be a percentage");
    T_TRUE(pc_error_text(PC_OK)[0] != 0);
    T_TRUE(pc_error_text(PC_ERR_PERIODS)[0] != 0);
    T_TRUE(pc_error_text((pc_status_t)99)[0] != 0);
}

static void run_tests(void)
{
    test_change();
    test_apply_and_reverse();
    test_compound();
    test_text();
}

T_MAIN("test_percent")
