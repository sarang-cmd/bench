/* test_numfmt.c - decimal formatter: exact digits, half-up rounding, rendering.
 * Covers the plan's rounding rules (section 6.2) and the no-float-printf rule. */

#include "numfmt.h"
#include "test_util.h"

static char buf[NUMFMT_BUFSIZE];

static void from_double_exact(void)
{
    T_SECTION("double -> decimal digits");

    numfmt_dec_t d;

    T_INT(numfmt_dec_from_double(15.0, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_significant(&d), 2);
    T_INT(d.exp10, 0);
    T_FALSE(numfmt_dec_is_negative(&d));
    T_INT(numfmt_dec_place(&d), 0);

    T_INT(numfmt_dec_from_double(0.15, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_significant(&d), 2);
    T_INT(d.exp10, -2);
    T_INT(numfmt_dec_place(&d), 2);

    T_INT(numfmt_dec_from_double(-2.5, 15, &d), NUMFMT_OK);
    T_TRUE(numfmt_dec_is_negative(&d));
    T_INT(numfmt_dec_place(&d), 1);

    /* Negative zero is not a negative number in this model. */
    T_INT(numfmt_dec_from_double(-0.0, 15, &d), NUMFMT_OK);
    T_TRUE(numfmt_dec_is_zero(&d));
    T_FALSE(numfmt_dec_is_negative(&d));

    /* 1/3 is 0.333...: 15 digits, all threes. */
    T_INT(numfmt_dec_from_double(1.0 / 3.0, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_significant(&d), 15);
    T_INT(d.exp10, -15);
    T_INT(numfmt_dec_lead_digit(&d), 3);

    /* Trailing zeros of the exact value are folded into the exponent. */
    T_INT(numfmt_dec_from_double(100.0, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_significant(&d), 1);
    T_INT(d.exp10, 2);
    T_INT(numfmt_dec_place(&d), 0);

    /* Invalid input is rejected, not formatted. */
    T_INT(numfmt_dec_from_double((double)NAN, 15, &d), NUMFMT_ERR_ARG);
    T_INT(numfmt_dec_from_double((double)INFINITY, 15, &d), NUMFMT_ERR_ARG);
    T_INT(numfmt_dec_from_double(1.0, 0, &d), NUMFMT_ERR_ARG);
    T_INT(numfmt_dec_from_double(1.0, NUMFMT_DIGITS + 1, &d), NUMFMT_ERR_ARG);

    /* A huge and a tiny value still convert. */
    T_INT(numfmt_dec_from_double(1e300, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_place(&d), 0);
    T_INT(numfmt_dec_from_double(1e-300, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_place(&d), 300);
}

static void canonicalise(void)
{
    T_SECTION("canonicalise a double to 15 significant digits");

    numfmt_dec_t d;

    /* 0.1 + 0.2 is 0.30000000000000004 in binary: 15 digits reads 0.3. */
    T_INT(numfmt_dec_from_double(0.1 + 0.2, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_render(&d, 0, -1, buf, sizeof buf), 3);
    T_STR(buf, "0.3");

    /* 6 * (0.1/2 + 0.1/3) lands just under 0.5 in binary. */
    T_INT(numfmt_dec_from_double(6.0 * (0.1 / 2.0 + 0.1 / 3.0), 15, &d),
          NUMFMT_OK);
    T_INT(numfmt_dec_render(&d, 0, -1, buf, sizeof buf), 3);
    T_STR(buf, "0.5");

    /* Just under 0.15 in binary canonicalises to 0.15. */
    T_INT(numfmt_dec_from_double(2.5 * (0.1 / 10.0 + 0.2 / 4.0), 15, &d),
          NUMFMT_OK);
    T_INT(numfmt_dec_render(&d, 0, -1, buf, sizeof buf), 4);
    T_STR(buf, "0.15");

    /* Rounding up through 9.99 -> 10 with the carry handled. */
    T_INT(numfmt_dec_from_double(9.9999999999999999, 2, &d), NUMFMT_OK);
    T_INT(numfmt_dec_significant(&d), 1);
    T_INT(numfmt_dec_render(&d, 0, -1, buf, sizeof buf), 2);
    T_STR(buf, "10");
}

static void rounding(void)
{
    T_SECTION("half-up rounding on decimal digits");

    numfmt_dec_t d;

    /* The plan's row: (10.0 +- 0.1) / (4.0 +- 0.2) has a raw uncertainty of
     * 0.15, and half-up on the decimal digits gives 0.2. */
    T_INT(numfmt_dec_from_double(0.15, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_round_sig(&d, 1), NUMFMT_OK);
    T_INT(numfmt_dec_render(&d, 0, -1, buf, sizeof buf), 3);
    T_STR(buf, "0.2");

    /* 1.6 -> 2 at one significant figure. */
    T_INT(numfmt_dec_from_double(1.6, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_round_sig(&d, 1), NUMFMT_OK);
    T_INT(numfmt_dec_render(&d, 0, -1, buf, sizeof buf), 1);
    T_STR(buf, "2");

    /* Exactly 5 rounds away from zero, both signs. */
    T_INT(numfmt_round_place(2.5, 0, NULL), NUMFMT_OK);
    T_INT(numfmt_format_fixed(2.5, 0, buf, sizeof buf), 1);
    T_STR(buf, "3");
    T_INT(numfmt_format_fixed(-2.5, 0, buf, sizeof buf), 2);
    T_STR(buf, "-3");

    /* 0.45 is stored just below 0.45: canonical 15 digits says 0.45, then the
     * decimal half-up rule gives 0.5. */
    T_INT(numfmt_format_fixed(0.45, 1, buf, sizeof buf), 3);
    T_STR(buf, "0.5");

    /* No rounding when the place is already coarser than the value. */
    T_INT(numfmt_dec_from_double(300.0, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_round_place(&d, 0), NUMFMT_OK);
    T_INT(numfmt_dec_render(&d, 0, -1, buf, sizeof buf), 3);
    T_STR(buf, "300");
    T_INT(numfmt_dec_round_place(&d, -2), NUMFMT_OK); /* round to hundreds */
    T_INT(numfmt_dec_render(&d, 0, -1, buf, sizeof buf), 3);
    T_STR(buf, "300");

    /* Rounding a value smaller than half a unit gives zero, not a crash. */
    T_INT(numfmt_dec_from_double(0.04, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_round_place(&d, 1), NUMFMT_OK);
    T_TRUE(numfmt_dec_is_zero(&d));
    T_INT(numfmt_dec_render(&d, 0, -1, buf, sizeof buf), 1);
    T_STR(buf, "0");

    /* 0.05 is exactly half of 0.1: it rounds up to 0.1. */
    T_INT(numfmt_dec_from_double(0.05, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_round_place(&d, 1), NUMFMT_OK);
    T_INT(numfmt_dec_render(&d, 0, -1, buf, sizeof buf), 3);
    T_STR(buf, "0.1");

    /* Carry across the integer part: 9.96 -> 10.0 at one decimal. */
    T_INT(numfmt_dec_from_double(9.96, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_round_place(&d, 1), NUMFMT_OK);
    T_INT(numfmt_dec_render(&d, 1, 1, buf, sizeof buf), 4);
    T_STR(buf, "10.0");

    /* Carry that renormalises: 0.999 at one significant figure -> 1. */
    T_INT(numfmt_dec_from_double(0.999, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_round_sig(&d, 1), NUMFMT_OK);
    T_INT(numfmt_dec_significant(&d), 1);
    T_INT(numfmt_dec_render(&d, 0, -1, buf, sizeof buf), 1);
    T_STR(buf, "1");

    /* Bad arguments are reported. */
    T_INT(numfmt_dec_round_sig(&d, 0), NUMFMT_ERR_ARG);
    T_INT(numfmt_dec_round_place(NULL, 1), NUMFMT_ERR_ARG);
}

static void rendering(void)
{
    T_SECTION("plain rendering, padding and buffer limits");

    numfmt_dec_t d;

    T_INT(numfmt_dec_from_double(15.0, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_render(&d, 1, 1, buf, sizeof buf), 4);
    T_STR(buf, "15.0");
    T_INT(numfmt_dec_render(&d, 3, 3, buf, sizeof buf), 6);
    T_STR(buf, "15.000");
    /* max_dec rounds; 15 has nothing to round. */
    T_INT(numfmt_dec_render(&d, 0, 0, buf, sizeof buf), 2);
    T_STR(buf, "15");

    /* Padding a small value. */
    T_INT(numfmt_dec_from_double(0.15, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_render(&d, 3, 3, buf, sizeof buf), 5);
    T_STR(buf, "0.150");
    T_INT(numfmt_dec_render(&d, 0, 1, buf, sizeof buf), 3);
    T_STR(buf, "0.2");

    /* Zero, with and without decimals. */
    numfmt_dec_zero(&d);
    T_INT(numfmt_dec_render(&d, 0, 0, buf, sizeof buf), 1);
    T_STR(buf, "0");
    T_INT(numfmt_dec_render(&d, 2, 2, buf, sizeof buf), 4);
    T_STR(buf, "0.00");

    /* Negative values keep the sign. */
    T_INT(numfmt_dec_from_double(-0.5, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_render(&d, 1, 1, buf, sizeof buf), 4);
    T_STR(buf, "-0.5");

    /* A buffer that is too small fails cleanly instead of overflowing.
     * 123456789012 needs 13 characters with min_dec 2. */
    T_INT(numfmt_dec_from_double(123456789012.0, 15, &d), NUMFMT_OK);
    char tiny[4];
    T_INT(numfmt_dec_render(&d, 0, 0, tiny, sizeof tiny), NUMFMT_ERR_SIZE);
    T_INT(numfmt_dec_render(&d, 0, 0, buf, sizeof buf), 12);
    T_STR(buf, "123456789012");

    /* Argument checks. */
    T_INT(numfmt_dec_render(NULL, 0, 0, buf, sizeof buf), NUMFMT_ERR_ARG);
    T_INT(numfmt_dec_render(&d, 2, 1, buf, sizeof buf), NUMFMT_ERR_ARG);
    T_INT(numfmt_dec_render(&d, 0, 0, NULL, 10), NUMFMT_ERR_ARG);
    T_INT(numfmt_dec_render(&d, 0, 0, buf, 0), NUMFMT_ERR_ARG);
}

static void scientific(void)
{
    T_SECTION("scientific fallback");

    numfmt_dec_t d;

    T_INT(numfmt_dec_from_double(1e300, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_render_sci(&d, 2, buf, sizeof buf), 7);
    T_STR(buf, "1.0e300");

    T_INT(numfmt_dec_from_double(1.5e-7, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_render_sci(&d, 2, buf, sizeof buf), 6);
    T_STR(buf, "1.5e-7");

    /* A value between 1 and 999 needs no exponent. */
    T_INT(numfmt_dec_from_double(250.0, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_render_sci(&d, 2, buf, sizeof buf), 5);
    T_STR(buf, "2.5e2");

    T_INT(numfmt_dec_from_double(5.0, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_render_sci(&d, 1, buf, sizeof buf), 1);
    T_STR(buf, "5");

    /* fit: plain when it fits, scientific when it does not. */
    T_INT(numfmt_dec_from_double(15.0, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_render_fit(&d, 1, 1, 16, buf, sizeof buf), 4);
    T_STR(buf, "15.0");

    T_INT(numfmt_dec_from_double(1e-30, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_render_fit(&d, 0, -1, 16, buf, sizeof buf), 5);
    T_STR(buf, "1e-30");

    /* A long plain form falls back even when the buffer would have held it. */
    T_INT(numfmt_dec_from_double(123456.0, 15, &d), NUMFMT_OK);
    T_INT(numfmt_dec_render_fit(&d, 0, 0, 4, buf, sizeof buf), 3);
    T_STR(buf, "1e5");
}

static void double_helpers(void)
{
    T_SECTION("double helpers");

    double out = 0.0;

    T_INT(numfmt_round_sig(1.6, 1, &out), NUMFMT_OK);
    T_EXACT(out, 2.0);
    T_INT(numfmt_round_sig(-1.6, 1, &out), NUMFMT_OK);
    T_EXACT(out, -2.0);
    T_INT(numfmt_round_sig(0.0, 3, &out), NUMFMT_OK);
    T_EXACT(out, 0.0);

    T_INT(numfmt_round_place(2.345, 2, &out), NUMFMT_OK);
    T_NEAR(out, 2.35, 1e-12);
    T_INT(numfmt_round_place(-2.345, 2, &out), NUMFMT_OK);
    T_NEAR(out, -2.35, 1e-12);

    T_INT(numfmt_format_fixed(0.15, 1, buf, sizeof buf), 3);
    T_STR(buf, "0.2");
    T_INT(numfmt_format_fixed(-0.15, 1, buf, sizeof buf), 4);
    T_STR(buf, "-0.2");

    /* Two significant figures, plain when short. */
    /* Two figures are shown even when the second one is a padding zero. */
    T_INT(numfmt_format_sig(2.0, 2, buf, sizeof buf), 3);
    T_STR(buf, "2.0");
    T_INT(numfmt_format_sig(15.0, 3, buf, sizeof buf), 4);
    T_STR(buf, "15.0");
    T_INT(numfmt_format_sig(8.333333, 2, buf, sizeof buf), 3);
    T_STR(buf, "8.3");
    T_INT(numfmt_format_sig(1234.0, 2, buf, sizeof buf), 4);
    T_STR(buf, "1200");
    T_INT(numfmt_format_sig(1e300, 2, buf, sizeof buf), 7);
    T_STR(buf, "1.0e300");

    T_INT(numfmt_round_sig((double)NAN, 2, &out), NUMFMT_ERR_ARG);
    T_INT(numfmt_format_fixed((double)INFINITY, 2, buf, sizeof buf),
          NUMFMT_ERR_ARG);
}

static void round_trip(void)
{
    T_SECTION("decimal -> double round trip");

    static double const values[] = {
        0.0, 1.0, -1.0, 0.5, 1.0 / 3.0, 123.456, -0.000125,
        1e-300, 1e300, 9007199254740992.0,
    };

    for (unsigned i = 0; i < sizeof values / sizeof *values; i++) {
        numfmt_dec_t d;
        T_INT(numfmt_dec_from_double(values[i], 15, &d), NUMFMT_OK);
        double back = numfmt_dec_to_double(&d);
        /* 15 significant digits, so the relative error stays under 1e-14. */
        T_NEAR(back, values[i], 1e-14 * (values[i] < 0 ? -values[i] : values[i])
                                   + 1e-300);
    }

    /* Equality compares digits, so 0.15 and 0.150 are the same number. */
    numfmt_dec_t a, b;
    (void)numfmt_dec_from_double(0.15, 15, &a);
    (void)numfmt_dec_from_double(0.150, 15, &b);
    T_TRUE(numfmt_dec_equal(&a, &b));
    (void)numfmt_dec_from_double(0.1501, 15, &b);
    T_FALSE(numfmt_dec_equal(&a, &b));
}

static void run_tests(void)
{
    from_double_exact();
    canonicalise();
    rounding();
    rendering();
    scientific();
    double_helpers();
    round_trip();
}

T_MAIN("test_numfmt")
