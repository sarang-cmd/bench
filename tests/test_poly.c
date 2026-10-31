/* test_poly.c - polynomial arithmetic and factoring (V2 F3).
 *
 * Every vector the master prompt gives for F3 is here, plus the parse and
 * format round trips the screens depend on. */

#include <stdbool.h>
#include <string.h>

#include "poly.h"
#include "test_util.h"

static void expect_format(char const *text, char const *want)
{
    poly_t p;
    char buf[80];

    T_INT(poly_parse(text, &p), POLY_OK);
    T_INT(poly_format(&p, buf, (int)sizeof buf) >= 0, 1);
    T_STR(buf, want);
}

static void test_parse_format(void)
{
    T_SECTION("parse and format");

    expect_format("x^2 - 5x + 6", "x^2 - 5x + 6");
    expect_format("2x^3 - 5x + 1", "2x^3 - 5x + 1");
    expect_format("x", "x");
    expect_format("-x + 2", "-x + 2");
    expect_format("5", "5");
    expect_format("0", "0");
    expect_format("x^2", "x^2");
    expect_format("-3x^2 + 0x + 1", "-3x^2 + 1");
    expect_format("  x  +  1  ", "x + 1");

    {
        poly_t p;

        T_INT(poly_parse("", &p), POLY_ERR_SYNTAX);
        T_INT(poly_parse("x^9", &p), POLY_ERR_DEGREE);
        T_INT(poly_parse("2 3", &p), POLY_ERR_SYNTAX);
        T_INT(poly_parse("x^^2", &p), POLY_ERR_SYNTAX);
    }
}

static void test_quadratic(void)
{
    T_SECTION("quadratic");

    {
        /* x^2 - 5x + 6: roots 2 and 3, discriminant 1, vertex (2.5, -0.25). */
        double disc, r1re, r1im, r2re, r2im;

        T_INT(poly_quadratic(1.0, -5.0, 6.0, &disc, &r1re, &r1im, &r2re, &r2im),
              POLY_OK);
        T_NEAR(disc, 1.0, 1e-12);
        T_NEAR(r1re, 3.0, 1e-12);
        T_NEAR(r2re, 2.0, 1e-12);
        T_NEAR(r1im, 0.0, 0.0);
        T_NEAR(r2im, 0.0, 0.0);

        /* The vertex follows from the coefficients. */
        T_NEAR(-(-5.0) / (2.0 * 1.0), 2.5, 1e-12);
        T_NEAR(1.0 * 2.5 * 2.5 - 5.0 * 2.5 + 6.0, -0.25, 1e-12);
    }

    {
        /* x^2 + 2x + 5: discriminant -16, roots -1 +/- 2i. */
        double disc, r1re, r1im, r2re, r2im;

        T_INT(poly_quadratic(1.0, 2.0, 5.0, &disc, &r1re, &r1im, &r2re, &r2im),
              POLY_OK);
        T_NEAR(disc, -16.0, 1e-12);
        T_NEAR(r1re, -1.0, 1e-12);
        T_NEAR(r2re, -1.0, 1e-12);
        T_NEAR(r1im, 2.0, 1e-12);
        T_NEAR(r2im, -2.0, 1e-12);
    }

    {
        /* A repeated root, and a line. */
        double disc, r1re, r1im, r2re, r2im;

        poly_quadratic(1.0, -4.0, 4.0, &disc, &r1re, &r1im, &r2re, &r2im);
        T_NEAR(disc, 0.0, 1e-12);
        T_NEAR(r1re, 2.0, 1e-12);
        T_NEAR(r2re, 2.0, 1e-12);
        T_INT(poly_quadratic(0.0, 2.0, 4.0, &disc, &r1re, &r1im, &r2re, &r2im),
              POLY_ERR_SYNTAX);
        T_NEAR(r1re, -2.0, 1e-12);
    }
}

static void test_arithmetic(void)
{
    T_SECTION("add, subtract, multiply, expand");

    poly_t a, b, out;
    char buf[80];

    poly_parse("x^2 + 1", &a);
    poly_parse("x - 1", &b);
    poly_mul(&a, &b, &out);
    poly_format(&out, buf, (int)sizeof buf);
    T_STR(buf, "x^3 - x^2 + x - 1");

    poly_add(&a, &b, &out);
    poly_format(&out, buf, (int)sizeof buf);
    T_STR(buf, "x^2 + x");

    poly_sub(&a, &b, &out);
    poly_format(&out, buf, (int)sizeof buf);
    T_STR(buf, "x^2 - x + 2");

    /* (x + 2)^3 = x^3 + 6x^2 + 12x + 8. */
    T_INT(poly_binomial(1.0, 2.0, 3, &out), POLY_OK);
    poly_format(&out, buf, (int)sizeof buf);
    T_STR(buf, "x^3 + 6x^2 + 12x + 8");

    /* (2x + 3)^4, for the sequences tool's coefficient: 216 at x^2. */
    poly_binomial(2.0, 3.0, 4, &out);
    T_NEAR(out.c[2], 216.0, 1e-9);

    T_INT(poly_binomial(1.0, 1.0, 9, &out), POLY_ERR_DEGREE);
}

static void test_div(void)
{
    T_SECTION("division");

    poly_t a, b, q, r;
    char buf[80];

    /* (x^3 - 2x^2 - 4) / (x - 3) = x^2 + x + 3 remainder 5. */
    poly_parse("x^3 - 2x^2 - 4", &a);
    poly_parse("x - 3", &b);
    T_INT(poly_div(&a, &b, &q, &r), POLY_OK);
    poly_format(&q, buf, (int)sizeof buf);
    T_STR(buf, "x^2 + x + 3");
    T_NEAR(r.c[0], 5.0, 1e-12);
    T_INT(r.deg, 0);

    /* An exact division has no remainder. */
    poly_parse("x^2 - 5x + 6", &a);
    poly_parse("x - 2", &b);
    poly_div(&a, &b, &q, &r);
    poly_format(&q, buf, (int)sizeof buf);
    T_STR(buf, "x - 3");
    T_NEAR(r.c[0], 0.0, 1e-12);

    /* A smaller numerator is all remainder, and dividing by zero is refused. */
    poly_parse("3", &a);
    poly_parse("x + 1", &b);
    poly_div(&a, &b, &q, &r);
    T_NEAR(r.c[0], 3.0, 1e-12);
    poly_parse("0", &b);
    T_INT(poly_div(&a, &b, &q, &r), POLY_ERR_DIV_ZERO);
}

static void test_factor(void)
{
    T_SECTION("factoring over the rationals");

    poly_t p, rest;
    double roots[POLY_MAX_ROOTS];
    int count = 0;

    /* x^3 - 6x^2 + 11x - 6 = (x-1)(x-2)(x-3). */
    poly_parse("x^3 - 6x^2 + 11x - 6", &p);
    T_INT(poly_factor(&p, roots, &count, &rest), POLY_OK);
    T_INT(count, 3);
    {
        double sum = roots[0] + roots[1] + roots[2];
        double prod = roots[0] * roots[1] * roots[2];

        T_NEAR(sum, 6.0, 1e-9);
        T_NEAR(prod, 6.0, 1e-9);
        for (int i = 0; i < 3; i++)
            T_NEAR(poly_eval(&p, roots[i]), 0.0, 1e-9);
    }
    T_INT(rest.deg, 0);
    T_NEAR(rest.c[0], 1.0, 1e-9);

    /* x^2 - 5x + 6 has the rational roots 2 and 3. */
    poly_parse("x^2 - 5x + 6", &p);
    poly_factor(&p, roots, &count, &rest);
    T_INT(count, 2);

    /* 2x^2 - 3x + 1 = (2x - 1)(x - 1): the root 1/2 is not an integer. */
    poly_parse("2x^2 - 3x + 1", &p);
    poly_factor(&p, roots, &count, &rest);
    T_INT(count, 2);
    {
        bool half = false, one = false;

        for (int i = 0; i < count; i++)
        {
            if (roots[i] > 0.49 && roots[i] < 0.51)
                half = true;
            if (roots[i] > 0.99 && roots[i] < 1.01)
                one = true;
        }
        T_TRUE(half);
        T_TRUE(one);
    }

    /* x^2 + 1 has none, and is left as it is. */
    poly_parse("x^2 + 1", &p);
    poly_factor(&p, roots, &count, &rest);
    T_INT(count, 0);
    T_INT(rest.deg, 2);
}

static void test_eval(void)
{
    T_SECTION("evaluating");

    poly_t p;

    poly_parse("2x^3 - 5x + 1", &p);
    T_NEAR(poly_eval(&p, 2.0), 16.0 - 10.0 + 1.0, 1e-12);
    T_NEAR(poly_eval(&p, 0.0), 1.0, 1e-12);
    T_NEAR(poly_eval(&p, -1.0), -2.0 + 5.0 + 1.0, 1e-12);
}

static void run_tests(void)
{
    test_parse_format();
    test_quadratic();
    test_arithmetic();
    test_div();
    test_factor();
    test_eval();
}

T_MAIN("test_poly")
