/* test_numparse.c - native tests for src/core/numparse.c. */

#include "../src/core/numparse.h"
#include "test_util.h"

static void parse_ok(void)
{
    double v = 0.0;

    T_SECTION("plain integers and decimals");
    T_INT(numparse_cstr("0", &v), NUMPARSE_OK);
    T_EXACT(v, 0.0);
    T_INT(numparse_cstr("42", &v), NUMPARSE_OK);
    T_EXACT(v, 42.0);
    T_INT(numparse_cstr("1.125", &v), NUMPARSE_OK);
    T_EXACT(v, 1.125);
    T_INT(numparse_cstr("0.15", &v), NUMPARSE_OK);
    T_EXACT(v, 0.15);
    T_INT(numparse_cstr("-3.5", &v), NUMPARSE_OK);
    T_EXACT(v, -3.5);
    T_INT(numparse_cstr("+7", &v), NUMPARSE_OK);
    T_EXACT(v, 7.0);
    T_INT(numparse_cstr(".5", &v), NUMPARSE_OK);
    T_EXACT(v, 0.5);
    T_INT(numparse_cstr("5.", &v), NUMPARSE_OK);
    T_EXACT(v, 5.0);
    T_INT(numparse_cstr("  12.25  ", &v), NUMPARSE_OK);
    T_EXACT(v, 12.25);

    T_SECTION("exponents");
    T_INT(numparse_cstr("1e3", &v), NUMPARSE_OK);
    T_EXACT(v, 1000.0);
    T_INT(numparse_cstr("2.5e-3", &v), NUMPARSE_OK);
    T_EXACT(v, 0.0025);
    T_INT(numparse_cstr("-1.5E2", &v), NUMPARSE_OK);
    T_EXACT(v, -150.0);
    T_INT(numparse_cstr("1e0", &v), NUMPARSE_OK);
    T_EXACT(v, 1.0);
    T_INT(numparse_cstr("0.001", &v), NUMPARSE_OK);
    T_EXACT(v, 0.001);
    T_INT(numparse_cstr("123456789", &v), NUMPARSE_OK);
    T_EXACT(v, 123456789.0);
}

static void parse_errors(void)
{
    double v = 123.0;

    T_SECTION("errors leave the value alone");
    T_INT(numparse_cstr("", &v), NUMPARSE_ERR_EMPTY);
    T_INT(numparse_cstr("   ", &v), NUMPARSE_ERR_EMPTY);
    T_INT(numparse_cstr("-", &v), NUMPARSE_ERR_EMPTY);
    T_INT(numparse_cstr(".", &v), NUMPARSE_ERR_EMPTY);
    T_EXACT(v, 123.0);

    T_INT(numparse_cstr("1.2.3", &v), NUMPARSE_ERR_SYNTAX);
    T_INT(numparse_cstr("12a", &v), NUMPARSE_ERR_SYNTAX);
    T_INT(numparse_cstr("1e", &v), NUMPARSE_ERR_SYNTAX);
    T_INT(numparse_cstr("--1", &v), NUMPARSE_ERR_SYNTAX);
    T_INT(numparse_cstr("1 2", &v), NUMPARSE_ERR_SYNTAX);
    T_EXACT(v, 123.0);

    T_INT(numparse_cstr("1234567890123456789", &v), NUMPARSE_ERR_RANGE);
    T_INT(numparse_cstr("1e400", &v), NUMPARSE_ERR_RANGE);

    T_SECTION("bad arguments");
    T_INT(numparse_double(NULL, 3, &v), NUMPARSE_ERR_EMPTY);
    T_INT(numparse_double("1", 1, NULL), NUMPARSE_ERR_EMPTY);
    T_INT(numparse_double("1", 0, &v), NUMPARSE_ERR_EMPTY);
    T_FALSE(numparse_ok("x", 1));
    T_TRUE(numparse_ok("1.5", 3));
}

static void range_exact(void)
{
    double v = 0.0;

    T_SECTION("values the tools use");
    T_INT(numparse_cstr("90", &v), NUMPARSE_OK);
    T_EXACT(v, 90.0);
    T_INT(numparse_cstr("120", &v), NUMPARSE_OK);
    T_INT(numparse_cstr("110", &v), NUMPARSE_OK);
    T_INT(numparse_cstr("10.0", &v), NUMPARSE_OK);
    T_EXACT(v, 10.0);
    T_INT(numparse_cstr("0.1", &v), NUMPARSE_OK);
    T_EXACT(v, 0.1);
    T_INT(numparse_cstr("1000000007", &v), NUMPARSE_OK);
    T_EXACT(v, 1000000007.0);
    T_INT(numparse_cstr("1e308", &v), NUMPARSE_OK);
    T_TRUE(v > 1e307);
    T_INT(numparse_cstr("1e-320", &v), NUMPARSE_OK);
    T_TRUE(v > 0.0 && v < 1e-300);
}

static void run_tests(void)
{
    parse_ok();
    parse_errors();
    range_exact();
}

T_MAIN("test_numparse")
