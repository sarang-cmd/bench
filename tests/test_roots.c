/* test_roots.c - the root finder (V2 feature F4).
 *
 * Vectors from the master prompt: Newton on x^2 - 2 from 1 gives
 * 1.5, 1.416667, 1.414216; bisection on [1, 2] reaches 1.414214 within 1e-6
 * in twenty iterations or fewer.  Everything else here is the behaviour a
 * student will hit: no bracket, a flat slope, a bad expression.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "test_util.h"

#include "roots.h"

static double const SQRT2 = 1.4142135623730951;

static void test_newton(void)
{
    root_result_t r;

    T_INT(root_newton("x^2-2", 1.0, ROOT_TOL, &r), ROOT_OK);
    T_INT(r.count >= 3, 1);
    /* The prompt's table, to the six decimal places the screen shows.  The
     * slope is a central difference, so the first step is right to about
     * 1e-10, not to the last bit. */
    T_NEAR(r.step[0].x, 1.5, 1e-9);
    T_NEAR(r.step[1].x, 1.4166666666666667, 1e-9);
    T_NEAR(r.step[2].x, 1.4142156862745099, 1e-9);
    T_NEAR(r.root, SQRT2, 1e-9);
    T_INT(r.count <= 6, 1);
    /* f(x) at the root is what closed it. */
    T_NEAR(r.step[r.count - 1].fx, 0.0, 1e-5);
    T_STR(root_status_text(r.status), "converged");
}

static void test_bisection(void)
{
    root_result_t r;

    T_INT(root_bisect("x^2-2", 1.0, 2.0, ROOT_TOL, &r), ROOT_OK);
    T_NEAR(r.root, SQRT2, ROOT_TOL);
    /* (b - a)/2 halves each time: 20 iterations get under 1e-6. */
    T_INT(r.count <= 20, 1);
    T_NEAR(r.step[0].x, 1.5, 1e-12);
    T_NEAR(r.step[0].delta, 0.5, 1e-12);
    T_INT(r.step[1].n, 2);
}

static void test_secant(void)
{
    root_result_t r;

    T_INT(root_secant("x^2-2", 1.0, 2.0, ROOT_TOL, &r), ROOT_OK);
    T_NEAR(r.root, SQRT2, ROOT_TOL);
    T_INT(r.count >= 2, 1);

    /* sin(x) - 0.5 is solved in radians, whatever the screen says. */
    T_INT(root_secant("sin(x)-0.5", 0.0, 1.0, ROOT_TOL, &r), ROOT_OK);
    T_NEAR(r.root, 0.5235987755982988, 1e-6);
}

static void test_errors(void)
{
    root_result_t r;

    /* No sign change: x^2 + 1 is never zero. */
    T_INT(root_bisect("x^2+1", -1.0, 1.0, ROOT_TOL, &r), ROOT_ERR_BRACKET);
    T_STR(root_status_text(ROOT_ERR_BRACKET),
          "no sign change in the bracket");

    /* Zero slope at the guess: Newton cannot take a step. */
    T_INT(root_newton("x^2+1", 0.0, ROOT_TOL, &r), ROOT_ERR_FLAT);
    T_STR(root_status_text(r.status), "zero slope: no step possible");

    /* A parse error keeps the column so the screen can point at it. */
    T_INT(root_bisect("x^2+", 1.0, 2.0, ROOT_TOL, &r), ROOT_ERR_EXPR);
    T_INT(r.column > 0, 1);
    T_STR(root_status_text(ROOT_ERR_EXPR), "check the expression");

    /* An interval that is not an interval, and a tolerance of zero. */
    T_INT(root_bisect("x", 1.0, 1.0, ROOT_TOL, &r), ROOT_ERR_BAD_INTERVAL);
    T_INT(root_newton("x", 1.0, 0.0, &r), ROOT_ERR_BAD_INTERVAL);

    /* Fifty steps and no answer is reported, never guessed. */
    T_INT(root_newton_max("x^3-2x+2", 0.0, 1e-12, 3, &r),
          ROOT_ERR_NO_CONVERGE);
    T_INT(r.count, 3);
    T_STR(root_status_text(ROOT_ERR_NO_CONVERGE), "stopped after 50 steps");
}

static void test_endpoints(void)
{
    root_result_t r;

    /* A root sitting exactly on an end of the bracket. */
    T_INT(root_bisect("x", 0.0, 4.0, ROOT_TOL, &r), ROOT_OK);
    T_NEAR(r.root, 0.0, 1e-12);
    T_INT(r.count, 1);

    /* x - 3 is linear: the secant method finds it in one step. */
    T_INT(root_secant("x-3", 0.0, 5.0, ROOT_TOL, &r), ROOT_OK);
    T_NEAR(r.root, 3.0, 1e-12);
    T_INT(r.count, 1);
}

static void test_null_out(void)
{
    /* The verdict must be available without a table: the screen asks for both,
     * the tests for either. */
    T_INT(root_newton("x^2-2", 1.0, ROOT_TOL, NULL), ROOT_OK);
    T_INT(root_bisect("x^2+1", -1.0, 1.0, ROOT_TOL, NULL), ROOT_ERR_BRACKET);
}

static void run_tests(void)
{
    T_SECTION("Newton: the prompt's table for x^2 - 2");
    test_newton();
    T_SECTION("bisection: a bracket that halves");
    test_bisection();
    T_SECTION("secant: two guesses, no derivative");
    test_secant();
    T_SECTION("what goes wrong, and what it says");
    test_errors();
    T_SECTION("a root on an endpoint");
    test_endpoints();
    T_SECTION("no table wanted");
    test_null_out();
}

T_MAIN("test_roots")
