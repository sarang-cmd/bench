/* test_expr.c - the expression engine (V2 F1).
 *
 * The vectors at the top of each section are the ones the master prompt names
 * for F1; the rest are the cases that broke during development or that a
 * student would hit in the first five minutes. */

#include <string.h>

#define BM_PI_D 3.14159265358979323846
#define BM_E_D 2.71828182845904523536

#include "expr.h"
#include "test_util.h"

static double eval(char const *text, double x)
{
    return expr_eval(text, x, EXPR_RAD).value;
}

static void expect(char const *text, double want, double eps)
{
    expr_result_t const r = expr_eval(text, 0.0, EXPR_RAD);

    T_CHECK(r.status == EXPR_OK, "%s: status %s at column %d", text,
            expr_status_text(r.status), r.column);
    T_NEAR(r.value, want, eps);
}

static void expect_error(char const *text, expr_status_t status, int column)
{
    expr_result_t const r = expr_eval(text, 0.0, EXPR_RAD);

    T_CHECK(r.status == status, "%s: got %s, want %s", text,
            expr_status_text(r.status), expr_status_text(status));
    T_INT(r.column, column);
}

static void test_basic_arithmetic(void)
{
    T_SECTION("arithmetic");

    /* The prompt's vector: 2^10 = 1024. */
    expect("2^10", 1024.0, 0.0);
    expect("1+2*3", 7.0, 0.0);
    expect("(1+2)*3", 9.0, 0.0);
    expect("10/4", 2.5, 0.0);
    expect("7-10", -3.0, 0.0);
    expect("-3", -3.0, 0.0);
    expect("2^-1", 0.5, 0.0);
    /* The unary minus binds looser than the power: -2^2 is -(2^2). */
    expect("-2^2", -4.0, 0.0);
    expect("(-2)^2", 4.0, 0.0);
    /* The power is right associative: 2^3^2 is 2^(3^2). */
    expect("2^3^2", 512.0, 0.0);
    expect("+5", 5.0, 0.0);
    expect("1.5*2", 3.0, 0.0);
    expect(".5+.25", 0.75, 0.0);
    expect("1e3", 1000.0, 0.0);
    expect("2.5e-2", 0.025, 1e-15);
    expect("0.1+0.2", 0.30000000000000004, 1e-16);
}

static void test_variable_and_constants(void)
{
    T_SECTION("x, pi and e");

    /* 3x at x = 4 gives 12. */
    T_NEAR(eval("3x", 4.0), 12.0, 0.0);
    T_NEAR(eval("x^2", 5.0), 25.0, 0.0);
    T_NEAR(eval("2(x+1)", 3.0), 8.0, 0.0);
    T_NEAR(eval("(x+1)(x-1)", 4.0), 15.0, 1e-12);
    T_NEAR(eval("x", 7.5), 7.5, 0.0);
    T_NEAR(eval("2x+3x", 2.0), 10.0, 0.0);
    T_NEAR(eval("pi", 0.0), BM_PI_D, 1e-15);
    T_NEAR(eval("2pi", 0.0), 2.0 * BM_PI_D, 1e-15);
    T_NEAR(eval("e", 0.0), BM_E_D, 1e-15);
    T_NEAR(eval("2e", 0.0), 2.0 * BM_E_D, 1e-15);
}

static void test_functions(void)
{
    T_SECTION("functions");

    /* sin(pi/6) = 0.5 within 1e-12.  sqrt(2)^2 = 2 within 1e-12. */
    T_NEAR(eval("sin(pi/6)", 0.0), 0.5, 1e-12);
    T_NEAR(eval("sqrt(2)^2", 0.0), 2.0, 1e-12);
    T_NEAR(eval("cos(0)", 0.0), 1.0, 1e-15);
    T_NEAR(eval("tan(pi/4)", 0.0), 1.0, 1e-12);
    T_NEAR(eval("asin(0.5)", 0.0), BM_PI_D / 6.0, 1e-12);
    T_NEAR(eval("acos(1)", 0.0), 0.0, 1e-15);
    T_NEAR(eval("atan(1)", 0.0), BM_PI_D / 4.0, 1e-12);
    T_NEAR(eval("ln(e)", 0.0), 1.0, 1e-12);
    T_NEAR(eval("log(1000)", 0.0), 3.0, 1e-12);
    T_NEAR(eval("exp(1)", 0.0), BM_E_D, 1e-12);
    T_NEAR(eval("abs(-4)", 0.0), 4.0, 0.0);
    T_NEAR(eval("floor(2.7)", 0.0), 2.0, 0.0);
    T_NEAR(eval("ceil(2.1)", 0.0), 3.0, 0.0);
    T_NEAR(eval("sinh(0)", 0.0), 0.0, 0.0);
    T_NEAR(eval("cosh(0)", 0.0), 1.0, 0.0);
    T_NEAR(eval("tanh(0)", 0.0), 0.0, 0.0);

    /* Nested calls, and a function applied to a whole expression. */
    T_NEAR(eval("sqrt(sqrt(16))", 0.0), 2.0, 0.0);
    T_NEAR(eval("abs(3-5)+2", 0.0), 4.0, 0.0);
    T_NEAR(eval("sin(pi/6)+cos(pi/3)", 0.0), 1.0, 1e-12);

    /* Functions are case insensitive, as is x and pi. */
    T_NEAR(eval("SIN(PI/6)", 0.0), 0.5, 1e-12);
    T_NEAR(eval("3X", 2.0), 6.0, 0.0);
}

static void test_angle_mode(void)
{
    T_SECTION("degrees and radians");

    T_NEAR(expr_eval("sin(30)", 0.0, EXPR_DEG).value, 0.5, 1e-12);
    T_NEAR(expr_eval("sin(30)", 0.0, EXPR_RAD).value, -0.9880316240928618,
           1e-13);
    T_NEAR(expr_eval("asin(0.5)", 0.0, EXPR_DEG).value, 30.0, 1e-12);
    T_NEAR(expr_eval("atan(1)", 0.0, EXPR_DEG).value, 45.0, 1e-12);
    T_NEAR(expr_eval("cos(180)", 0.0, EXPR_DEG).value, -1.0, 1e-12);
    /* A plain expression does not care about the mode. */
    T_NEAR(expr_eval("2+2", 0.0, EXPR_DEG).value,
           expr_eval("2+2", 0.0, EXPR_RAD).value, 0.0);
}

static void test_errors(void)
{
    T_SECTION("errors and their columns");

    /* The prompt's vector: 1/0 is "divide by zero" at column 2. */
    expect_error("1/0", EXPR_ERR_DIV_ZERO, 2);
    expect_error("2/(1-1)", EXPR_ERR_DIV_ZERO, 2);

    expect_error("", EXPR_ERR_EMPTY, 1);
    expect_error("   ", EXPR_ERR_EMPTY, 1);

    expect_error("1+", EXPR_ERR_SYNTAX, 3);
    expect_error("(1+2", EXPR_ERR_SYNTAX, 1);
    expect_error("1+2)", EXPR_ERR_SYNTAX, 4);
    expect_error("*3", EXPR_ERR_SYNTAX, 1);
    expect_error("2#3", EXPR_ERR_SYNTAX, 2);
    expect_error("sin 0", EXPR_ERR_SYNTAX, 1);

    /* An unknown name is reported by name and column, not as a syntax error. */
    {
        expr_result_t const r = expr_eval("2+frobnicate", 0.0, EXPR_RAD);

        T_CHECK(r.status == EXPR_ERR_NAME, "unknown name: got %s",
                expr_status_text(r.status));
        T_INT(r.column, 3);
        T_STR(r.token, "frobnicate");
    }

    /* Domains: sqrt of a negative, ln of zero. */
    expect_error("sqrt(-1)", EXPR_ERR_DOMAIN, 1);
    expect_error("ln(0)", EXPR_ERR_DOMAIN, 1);
    expect_error("log(-5)", EXPR_ERR_DOMAIN, 1);
    expect_error("asin(2)", EXPR_ERR_DOMAIN, 1);

    /* Deep nesting is refused rather than overflowing the stack. */
    {
        char deep[128];
        int i = 0;

        for (int k = 0; k < 40; k++)
            deep[i++] = '(';
        deep[i++] = '1';
        for (int k = 0; k < 40; k++)
            deep[i++] = ')';
        deep[i] = 0;
        T_CHECK(expr_eval(deep, 0.0, EXPR_RAD).status == EXPR_ERR_DEPTH,
                "40 levels are refused");
    }
}

static void test_status_text(void)
{
    T_SECTION("status text");

    T_STR(expr_status_text(EXPR_OK), "ok");
    T_STR(expr_status_text(EXPR_ERR_DIV_ZERO), "divide by zero");
    T_STR(expr_status_text(EXPR_ERR_DOMAIN), "not a real number");
    T_CHECK(strlen(expr_status_text(EXPR_ERR_SYNTAX)) > 0, "syntax text");
    for (int s = 0; s <= EXPR_ERR_DEPTH; s++)
        T_CHECK(expr_status_text((expr_status_t)s) != NULL, "status %d", s);
}

static void test_table_vector(void)
{
    T_SECTION("the table vector");

    /* "A table of x^2 for x = 1 to 5 gives 1, 4, 9, 16, 25." */
    double want = 1.0;

    for (double x = 1.0; x <= 5.0; x += 1.0)
    {
        expr_result_t const r = expr_eval("x^2", x, EXPR_RAD);

        T_CHECK(r.status == EXPR_OK, "x=%g: %s", x, expr_status_text(r.status));
        T_NEAR(r.value, want, 0.0);
        want += 1.0 + 2.0 * x;
    }
}

static void run_tests(void)
{
    test_basic_arithmetic();
    test_variable_and_constants();
    test_functions();
    test_angle_mode();
    test_errors();
    test_status_text();
    test_table_vector();
}

T_MAIN("test_expr")
