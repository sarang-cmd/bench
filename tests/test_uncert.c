/* test_uncert.c - every acceptance row and edge case of docs/plan.md 6.2. */

#include "uncert.h"
#include "test_util.h"

static uncert_qty_t qty(double value, double unc)
{
    uncert_qty_t q;
    q.value = value;
    q.unc = unc;
    return q;
}

static void acceptance_rows(void)
{
    T_SECTION("acceptance table");

    /* (10.0 +- 0.1) + (5.0 +- 0.2) = 15.0 +- 0.3 */
    uncert_result_t r = uncert_add(qty(10.0, 0.1), qty(5.0, 0.2));
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, 15.0, 1e-12);
    T_NEAR(r.q.unc, 0.30000000000000004, 1e-15);

    uncert_display_t d;
    T_INT(uncert_display(r.q, UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_STR(d.value, "15.0");
    T_STR(d.unc, "0.3");
    T_INT(d.decimals, 1);

    /* (2.0 +- 0.1) * (3.0 +- 0.1) = 6.0 +- 0.5 */
    r = uncert_mul(qty(2.0, 0.1), qty(3.0, 0.1));
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, 6.0, 1e-12);
    T_NEAR(r.q.unc, 0.5, 1e-12);
    T_INT(uncert_display(r.q, UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_STR(d.value, "6.0");
    T_STR(d.unc, "0.5");

    /* (4.0 +- 0.2) ^ 2 = 16 +- 2 */
    r = uncert_pow(qty(4.0, 0.2), 2.0);
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, 16.0, 1e-12);
    T_NEAR(r.q.unc, 1.6, 1e-12);
    T_INT(uncert_display(r.q, UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_STR(d.value, "16");
    T_STR(d.unc, "2");
    T_INT(d.decimals, 0);

    /* (10.0 +- 0.1) / (4.0 +- 0.2) = 2.5 +- 0.15, shown as 2.5 +- 0.2 */
    r = uncert_div(qty(10.0, 0.1), qty(4.0, 0.2));
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, 2.5, 1e-12);
    T_NEAR(r.q.unc, 0.15, 1e-12);
    T_INT(uncert_display(r.q, UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_STR(d.value, "2.5");
    T_STR(d.unc, "0.2");
    T_INT(d.decimals, 1);

    /* A value of 0 in a product: error line, no crash. */
    r = uncert_mul(qty(0.0, 0.1), qty(3.0, 0.1));
    T_INT(r.status, UNCERT_ERR_ZERO_A);
    T_NEAR(r.q.value, 0.0, 0.0);
    T_STR(uncert_error_text(r.status), "first value is 0");

    r = uncert_mul(qty(3.0, 0.1), qty(0.0, 0.1));
    T_INT(r.status, UNCERT_ERR_ZERO_B);

    r = uncert_div(qty(0.0, 0.1), qty(3.0, 0.1));
    T_INT(r.status, UNCERT_ERR_ZERO_A);
    r = uncert_div(qty(3.0, 0.1), qty(0.0, 0.1));
    T_INT(r.status, UNCERT_ERR_ZERO_B);
    r = uncert_pow(qty(0.0, 0.1), 2.0);
    T_INT(r.status, UNCERT_ERR_ZERO_A);
}

static void operations(void)
{
    T_SECTION("every operation");

    /* Subtraction: uncertainties add, the value can cross zero. */
    uncert_result_t r = uncert_sub(qty(5.0, 0.1), qty(10.0, 0.2));
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, -5.0, 1e-12);
    T_NEAR(r.q.unc, 0.30000000000000004, 1e-15);

    /* k * A scales the absolute uncertainty by |k|. */
    r = uncert_scale(qty(10.0, 0.1), 3.0);
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, 30.0, 1e-12);
    T_NEAR(r.q.unc, 0.30000000000000004, 1e-15);
    r = uncert_scale(qty(10.0, 0.1), -2.0);
    T_NEAR(r.q.value, -20.0, 1e-12);
    T_NEAR(r.q.unc, 0.2, 1e-15);

    /* Negative values use absolute values in the relative terms. */
    r = uncert_mul(qty(-2.0, 0.1), qty(3.0, 0.1));
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, -6.0, 1e-12);
    T_NEAR(r.q.unc, 0.5, 1e-12);

    r = uncert_div(qty(-8.0, 0.2), qty(2.0, 0.1));
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, -4.0, 1e-12);
    T_NEAR(r.q.unc, 0.30000000000000004, 1e-15);

    /* Powers: cube, zero, negative exponent. */
    r = uncert_pow(qty(2.0, 0.1), 3.0);
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, 8.0, 1e-12);
    T_NEAR(r.q.unc, 8.0 * 3.0 * (0.1 / 2.0), 1e-12);

    r = uncert_pow(qty(5.0, 0.5), 0.0);
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, 1.0, 1e-12);
    T_NEAR(r.q.unc, 0.0, 0.0);

    r = uncert_pow(qty(2.0, 0.1), -2.0);
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, 0.25, 1e-12);
    T_NEAR(r.q.unc, 0.25 * 2.0 * (0.1 / 2.0), 1e-12);

    /* Square root, the one fractional exponent this build supports. */
    r = uncert_pow(qty(4.0, 0.2), 0.5);
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, 2.0, 1e-12);
    T_NEAR(r.q.unc, 2.0 * 0.5 * (0.2 / 4.0), 1e-12);

    r = uncert_pow(qty(4.0, 0.2), -0.5);
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, 0.5, 1e-12);
    T_NEAR(r.q.unc, 0.5 * 0.5 * (0.2 / 4.0), 1e-12);

    /* A negative base with a fractional exponent is rejected. */
    r = uncert_pow(qty(-4.0, 0.2), 0.5);
    T_INT(r.status, UNCERT_ERR_NEG_BASE);
    /* A whole exponent on a negative base is fine. */
    r = uncert_pow(qty(-2.0, 0.1), 2.0);
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, 4.0, 1e-12);

    /* Other fractional exponents would need a libm exp/log: clear error. */
    r = uncert_pow(qty(4.0, 0.2), 0.25);
    T_INT(r.status, UNCERT_ERR_ARG);

    /* Dispatch used by the app's operation picker. */
    r = uncert_apply(UNCERT_OP_ADD, qty(1.0, 0.1), qty(2.0, 0.1));
    T_NEAR(r.q.value, 3.0, 1e-12);
    r = uncert_apply(UNCERT_OP_SUB, qty(1.0, 0.1), qty(2.0, 0.1));
    T_NEAR(r.q.value, -1.0, 1e-12);
    r = uncert_apply(UNCERT_OP_MUL, qty(2.0, 0.1), qty(5.0, 0.1));
    T_NEAR(r.q.value, 10.0, 1e-12);
    r = uncert_apply(UNCERT_OP_DIV, qty(10.0, 0.1), qty(5.0, 0.1));
    T_NEAR(r.q.value, 2.0, 1e-12);
    r = uncert_apply(UNCERT_OP_POW, qty(3.0, 0.1), qty(2.0, 0.0));
    T_NEAR(r.q.value, 9.0, 1e-12);
    r = uncert_apply(UNCERT_OP_SCALE, qty(3.0, 0.1), qty(4.0, 0.0));
    T_NEAR(r.q.value, 12.0, 1e-12);
    T_NEAR(r.q.unc, 0.4, 1e-12);
    r = uncert_apply((uncert_op_t)99, qty(1.0, 0.1), qty(1.0, 0.1));
    T_INT(r.status, UNCERT_ERR_ARG);
}

static void chain_mode(void)
{
    T_SECTION("chain mode: the result becomes A");

    uncert_qty_t a = qty(10.0, 0.1);
    uncert_result_t r = uncert_add(a, qty(5.0, 0.2));
    T_INT(r.status, UNCERT_OK);

    /* Feed the result back as A. */
    a = r.q;
    r = uncert_mul(a, qty(2.0, 0.05));
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, 30.0, 1e-12);
    /* 30 * (0.3/15 + 0.05/2) = 30 * 0.045 = 1.35 */
    T_NEAR(r.q.unc, 1.35, 1e-12);

    uncert_display_t d;
    T_INT(uncert_display(r.q, UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_STR(d.value, "30");
    T_STR(d.unc, "1");
    /* The percentage uses the rounded uncertainty shown above: 1/30 = 3.3 %. */
    T_STR(d.pct, "3.3");
    T_TRUE(d.pct_valid);

    /* A chain that hits an error must not poison the next step silently. */
    a = qty(0.0, 0.1);
    r = uncert_mul(a, qty(2.0, 0.1));
    T_INT(r.status, UNCERT_ERR_ZERO_A);
}

static void display_rules(void)
{
    T_SECTION("display: significant figures, place, percentage");

    uncert_display_t d;

    /* One significant figure by default. */
    T_INT(uncert_display(qty(15.0, 0.3), UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_STR(d.unc, "0.3");
    T_INT(d.unc_sig, 1);
    T_STR(d.pct, "2.0");

    /* With the two-figure setting, an uncertainty starting with 1 keeps 2. */
    T_INT(uncert_display(qty(2.5, 0.15), UNCERT_SIG_ONE_OR_TWO, &d),
          UNCERT_OK);
    T_STR(d.unc, "0.15");
    T_INT(d.unc_sig, 2);
    T_STR(d.value, "2.50");

    /* ... and any other leading digit still gets one figure. */
    T_INT(uncert_display(qty(2.5, 0.35), UNCERT_SIG_ONE_OR_TWO, &d),
          UNCERT_OK);
    T_STR(d.unc, "0.4");
    T_INT(d.unc_sig, 1);

    /* The value follows the uncertainty's decimal place. */
    T_INT(uncert_display(qty(1234.5678, 20.0), UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_STR(d.unc, "20");
    /* Same decimal place as the uncertainty (the units), so 1235. */
    T_STR(d.value, "1235");
    T_INT(d.decimals, 0);

    /* An uncertainty above the value still displays. */
    T_INT(uncert_display(qty(2.0, 5.0), UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_STR(d.value, "2");
    T_STR(d.unc, "5");
    T_STR(d.pct, "250");

    /* Zero value: no percentage, but no crash. */
    T_INT(uncert_display(qty(0.0, 0.1), UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_FALSE(d.pct_valid);
    T_STR(d.pct, "n/a");

    /* Exact value (uncertainty 0) keeps its own decimals. */
    T_INT(uncert_display(qty(2.5, 0.0), UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_STR(d.value, "2.5");
    T_STR(d.unc, "0.0");
    T_STR(d.pct, "0");

    /* Negative value. */
    T_INT(uncert_display(qty(-2.5, 0.15), UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_STR(d.value, "-2.5");
    T_STR(d.unc, "0.2");
    /* 0.2 / 2.5 = 8 % using the rounded uncertainty. */
    T_STR(d.pct, "8.0");

    /* Negative uncertainty is an input error. */
    T_INT(uncert_display(qty(1.0, -0.1), UNCERT_SIG_ONE, &d),
          UNCERT_ERR_NEG_UNC);
    T_INT(uncert_display(qty((double)NAN, 0.1), UNCERT_SIG_ONE, &d),
          UNCERT_ERR_ARG);
    T_INT(uncert_display(qty(1.0, 0.1), (uncert_sig_mode_t)7, &d),
          UNCERT_ERR_ARG);
    T_INT(uncert_display(qty(1.0, 0.1), UNCERT_SIG_ONE, NULL), UNCERT_ERR_ARG);
}

static void extremes(void)
{
    T_SECTION("tiny and huge values, overflow");

    uncert_result_t r = uncert_mul(qty(1e300, 1e290), qty(1e300, 1e290));
    T_INT(r.status, UNCERT_ERR_OVERFLOW);

    r = uncert_pow(qty(1e200, 1e190), 2.0);
    T_INT(r.status, UNCERT_ERR_OVERFLOW);

    /* Tiny values keep enough significant digits to render. */
    r = uncert_add(qty(1e-300, 1e-310), qty(1e-300, 1e-310));
    T_INT(r.status, UNCERT_OK);
    uncert_display_t d;
    T_INT(uncert_display(r.q, UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_TRUE(strchr(d.value, 'e') != NULL); /* scientific form */

    /* Very large uncertainty, very small value. */
    T_INT(uncert_display(qty(1e-6, 1e12), UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_STR(d.unc, "1000000000000");
    T_STR(d.value, "0");

    /* A negative uncertainty is rejected by every operation. */
    T_INT(uncert_add(qty(1.0, -0.1), qty(1.0, 0.1)).status, UNCERT_ERR_NEG_UNC);
    T_INT(uncert_sub(qty(1.0, -0.1), qty(1.0, 0.1)).status, UNCERT_ERR_NEG_UNC);
    T_INT(uncert_mul(qty(1.0, -0.1), qty(1.0, 0.1)).status, UNCERT_ERR_NEG_UNC);
    T_INT(uncert_div(qty(1.0, 0.1), qty(1.0, -0.1)).status, UNCERT_ERR_NEG_UNC);
    T_INT(uncert_pow(qty(1.0, -0.1), 2.0).status, UNCERT_ERR_NEG_UNC);
    T_INT(uncert_scale(qty(1.0, -0.1), 2.0).status, UNCERT_ERR_NEG_UNC);

    /* Non-finite inputs. */
    T_INT(uncert_pow(qty(2.0, 0.1), (double)NAN).status, UNCERT_ERR_ARG);
    T_INT(uncert_scale(qty(2.0, 0.1), (double)INFINITY).status,
          UNCERT_ERR_ARG);
    T_INT(uncert_add(qty((double)INFINITY, 0.1), qty(1.0, 0.1)).status,
          UNCERT_ERR_ARG);
}

static void large_exponent(void)
{
    T_SECTION("large whole exponents");

    /* 2^60 by repeated squaring, with the relative uncertainty rule. */
    uncert_result_t r = uncert_pow(qty(2.0, 0.001), 60.0);
    T_INT(r.status, UNCERT_OK);
    T_NEAR(r.q.value, 1152921504606846976.0, 1e3);
    T_TRUE(r.q.unc > 0.0);

    /* 1.0001^1000 stays near e. */
    r = uncert_pow(qty(1.0001, 0.00001), 1000.0);
    T_INT(r.status, UNCERT_OK);
    T_TRUE(r.q.value > 1.09 && r.q.value < 1.12); /* e ~ 1.105 */
    uncert_display_t d;
    T_INT(uncert_display(r.q, UNCERT_SIG_ONE, &d), UNCERT_OK);
    T_TRUE(d.pct_valid);

    /* 10^300 overflows; the error is reported instead of inf. */
    r = uncert_pow(qty(10.0, 0.1), 400.0);
    T_INT(r.status, UNCERT_ERR_OVERFLOW);
}

static void run_tests(void)
{
    acceptance_rows();
    operations();
    chain_mode();
    display_rules();
    extremes();
    large_exponent();
}

T_MAIN("test_uncert")
