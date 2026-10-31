/* test_cplx.c - complex arithmetic (V2 F6). */

#include <string.h>

#include "cplx.h"
#include "test_util.h"

static void test_arithmetic(void)
{
    cplx_t const a = cx(3.0, 4.0);
    cplx_t const b = cx(1.0, -2.0);
    cplx_t r;

    T_SECTION("add, subtract, multiply, divide");

    r = cplx_add(a, b);
    T_NEAR(r.re, 4.0, 0.0);
    T_NEAR(r.im, 2.0, 0.0);

    r = cplx_sub(a, b);
    T_NEAR(r.re, 2.0, 0.0);
    T_NEAR(r.im, 6.0, 0.0);

    /* (3 + 4i)(1 - 2i) = 3 - 6i + 4i - 8i^2 = 11 - 2i. */
    r = cplx_mul(a, b);
    T_NEAR(r.re, 11.0, 1e-12);
    T_NEAR(r.im, -2.0, 1e-12);

    /* (3 + 4i)/(1 - 2i) = (-5 + 10i)/5 = -1 + 2i. */
    r = cplx_div(a, b);
    T_NEAR(r.re, -1.0, 1e-12);
    T_NEAR(r.im, 2.0, 1e-12);

    /* Dividing by zero is not a number. */
    T_TRUE(cplx_is_nan(cplx_div(a, cx(0.0, 0.0))));

    r = cplx_conj(a);
    T_NEAR(r.re, 3.0, 0.0);
    T_NEAR(r.im, -4.0, 0.0);
}

static void test_powers(void)
{
    cplx_t z = cx(1.0, 1.0);
    cplx_t const r = cplx_powi(z, 4);

    T_SECTION("whole powers");

    /* (1 + i)^2 = 2i, so (1 + i)^4 = -4. */
    T_NEAR(r.re, -4.0, 1e-9);
    T_NEAR(r.im, 0.0, 1e-9);

    /* i^3 = -i, and anything^0 = 1. */
    z = cx(0.0, 1.0);
    T_NEAR(cplx_powi(z, 3).im, -1.0, 1e-12);
    T_NEAR(cplx_powi(z, 0).re, 1.0, 1e-12);

    /* A negative power is the reciprocal: (1 + i)^-1 = 0.5 - 0.5i. */
    z = cx(1.0, 1.0);
    T_NEAR(cplx_powi(z, -1).re, 0.5, 1e-12);
    T_NEAR(cplx_powi(z, -1).im, -0.5, 1e-12);
}

static void test_polar(void)
{
    cplx_t const a = cx(3.0, 4.0);
    cplx_t const b = cx(1.0, 1.0);
    cplx_t p;

    T_SECTION("modulus, argument and polar form");

    T_NEAR(cplx_abs(a), 5.0, 1e-12);
    T_NEAR(cplx_arg(b), 0.7853981634, 1e-9);       /* pi/4 */
    T_NEAR(cplx_arg(cx(-1.0, 0.0)), 3.1415926536, 1e-9);
    T_NEAR(cplx_arg(cx(0.0, -1.0)), -1.5707963268, 1e-9);

    p = cplx_polar(2.0, 1.5707963268);             /* 2 at 90 degrees */
    T_NEAR(p.re, 0.0, 1e-9);
    T_NEAR(p.im, 2.0, 1e-9);
}

static void test_format(void)
{
    char buf[48];

    T_SECTION("writing it out");

    T_INT(cplx_format(cx(3.0, 4.0), 3, buf, (int)sizeof buf) > 0, 1);
    T_STR(buf, "3 + 4i");
    T_INT(cplx_format(cx(3.0, -4.0), 3, buf, (int)sizeof buf) > 0, 1);
    T_STR(buf, "3 - 4i");
    T_INT(cplx_format(cx(3.0, 0.0), 3, buf, (int)sizeof buf) > 0, 1);
    T_STR(buf, "3");
    T_INT(cplx_format(cx(0.0, 2.0), 3, buf, (int)sizeof buf) > 0, 1);
    T_STR(buf, "2i");
    T_INT(cplx_format(cx(0.0, -1.0), 3, buf, (int)sizeof buf) > 0, 1);
    T_STR(buf, "-i");
    T_INT(cplx_format(cx(1.5, 0.5), 3, buf, (int)sizeof buf) > 0, 1);
    T_STR(buf, "1.5 + 0.5i");
    T_INT(cplx_format(cx(1.0, -1.0), 3, buf, (int)sizeof buf) > 0, 1);
    T_STR(buf, "1 - i");
}

static void run_tests(void)
{
    test_arithmetic();
    test_powers();
    test_polar();
    test_format();
}

T_MAIN("test_cplx")
