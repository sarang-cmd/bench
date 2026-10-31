/* test_sigfig.c - significant figures, standard form and SI prefixes (V2 F9).
 *
 * The vectors are the master prompt's, plus the cases that decide the rules:
 * trailing zeros before and after a point, an exponent, and a zero. */

#include <string.h>

#include "sigfig.h"
#include "test_util.h"

static void test_count(void)
{
    T_SECTION("counting significant figures");

    /* The prompt's vector: 0.004560 has four. */
    T_INT(sf_count_text("0.004560"), 4);
    T_INT(sf_count_text("4.560"), 4);
    T_INT(sf_count_text("4.56"), 3);
    T_INT(sf_count_text("1234.5"), 5);
    T_INT(sf_count_text("1200"), 2);      /* no point: the zeros are placeholders */
    T_INT(sf_count_text("1200."), 4);     /* the point makes them significant */
    T_INT(sf_count_text("1200.0"), 5);
    T_INT(sf_count_text("0.5"), 1);
    T_INT(sf_count_text(".5"), 1);
    T_INT(sf_count_text("10"), 1);
    T_INT(sf_count_text("10.0"), 3);
    T_INT(sf_count_text("-3.14"), 3);
    T_INT(sf_count_text("+2.0e10"), 2);
    T_INT(sf_count_text("1e-7"), 1);
    T_INT(sf_count_text("0"), 1);
    T_INT(sf_count_text("0.00"), 1);

    /* Not a number. */
    T_INT(sf_count_text(""), -1);
    T_INT(sf_count_text("1.2.3"), -1);
    T_INT(sf_count_text("12x"), -1);
    T_INT(sf_count_text("1e"), -1);
    T_INT(sf_count_text("  "), -1);
}

static void test_standard(void)
{
    char buf[40];

    T_SECTION("standard form");

    T_CHECK(sf_standard(0.00456, 3, buf, (int)sizeof buf) > 0, "0.00456");
    T_STR(buf, "4.56 x 10^-3");

    /* The prompt: 1234.5 to 3 significant figures is 1230, written 1.23 x 10^3. */
    sf_standard(1234.5, 3, buf, (int)sizeof buf);
    T_STR(buf, "1.23 x 10^3");

    sf_standard(1234.5, 4, buf, (int)sizeof buf);
    T_STR(buf, "1.235 x 10^3");   /* half-up: 1234.5 -> 1235 */

    sf_standard(5.0, 1, buf, (int)sizeof buf);
    T_STR(buf, "5 x 10^0");

    sf_standard(-0.00042, 2, buf, (int)sizeof buf);
    T_STR(buf, "-4.2 x 10^-4");

    sf_standard(0.0, 3, buf, (int)sizeof buf);
    T_STR(buf, "0");

    sf_standard(9.99, 2, buf, (int)sizeof buf);
    T_STR(buf, "1.0 x 10^1");
}

static void test_si(void)
{
    char buf[32];

    T_SECTION("SI prefixes and engineering form");

    T_STR(sf_si_prefix(-3), "m");
    T_STR(sf_si_prefix(6), "M");
    T_STR(sf_si_prefix(0), "");
    T_STR(sf_si_prefix(1), "");

    T_INT(sf_si_power(0.00456), -3);
    T_INT(sf_si_power(4.56), 0);
    T_INT(sf_si_power(4560.0), 3);
    T_INT(sf_si_power(0.0), 0);
    T_INT(sf_si_power(999.0), 0);

    sf_si(0.00456, 3, buf, (int)sizeof buf);
    T_STR(buf, "4.56 m");    /* the app appends the unit */
    sf_si(2500.0, 2, buf, (int)sizeof buf);
    T_STR(buf, "2.5 k");
    sf_si(3.3, 2, buf, (int)sizeof buf);
    T_STR(buf, "3.3 ");
}

static void run_tests(void)
{
    test_count();
    test_standard();
    test_si();
}

T_MAIN("test_sigfig")
