/* test_pi.c - the pi table, checked against the algorithm rather than itself.
 * Spec: src/core/pi.h. */

#include <string.h>

#include "pi.h"
#include "test_util.h"

/* The digits, computed here by the classic spigot: keep a row of remainders,
 * multiply by ten, take the carry out of the front.  Written from the algorithm
 * so a typo in the table cannot hide by being copied into the test. */
static void spigot(int want, char *out)
{
    /* Rabinowitz and Wagon's spigot, from their paper: a row of remainders,
     * multiply by ten, carry out of the front.  Plain integers, no big-number
     * library - which is the point, since it shares no code with the table. */
    static unsigned long a[400];
    int const len = 10 * want / 3;
    int n = 0;
    int nines = 0;
    int predigit = 0;

    for (int i = 0; i < len; i++)
        a[i] = 2;

    for (int j = 1; j <= want; j++)
    {
        unsigned long q = 0;

        for (int i = len; i > 0; i--)
        {
            unsigned long x = 10ul * a[i - 1] + q * (unsigned long)i;
            unsigned long d = (unsigned long)(2 * i - 1);

            a[i - 1] = x % d;
            q = x / d;
        }
        a[0] = q % 10ul;
        q /= 10ul;
        if (q == 9ul)
            nines++;
        else if (q == 10ul)
        {
            out[n++] = (char)('0' + predigit + 1);
            for (int k = 0; k < nines; k++)
                out[n++] = '0';
            predigit = 0;
            nines = 0;
        }
        else
        {
            if (j > 1)
            {
                out[n++] = (char)('0' + predigit);
                for (int k = 0; k < nines; k++)
                    out[n++] = '9';
            }
            predigit = (int)q;
            nines = 0;
        }
    }
    out[n++] = (char)('0' + predigit);
    out[n] = 0;
}

static void test_table_against_spigot(void)
{
    char ref[128];
    int bad = 0;

    T_SECTION("the table matches an independently computed 100 digits");

    spigot(101, ref);          /* the spigot yields one fewer than asked */
    T_INT((int)strlen(ref), 100);
    for (int i = 0; i < (int)strlen(ref); i++)
    {
        if (pi_digit(i) != ref[i] - '0')
        {
            bad++;
            if (bad == 1)
                T_INT(pi_digit(i), ref[i] - '0');   /* reports the first */
        }
    }
    T_INT(bad, 0);

    /* The digits everyone knows, as a second, human check. */
    T_INT(pi_digit(0), 3);
    T_INT(pi_digit(1), 1);
    T_INT(pi_digit(2), 4);
    T_INT(pi_digit(3), 1);
    T_INT(pi_digit(4), 5);
    T_INT(pi_digit(5), 9);
    T_INT(pi_digit(100), 9);   /* 3.14159...: the 100th decimal is 9 */
}

static void test_table_bounds(void)
{
    T_SECTION("the table has an end, and says so");

    T_INT(pi_digit_count(), 200);
    T_INT(pi_digit(-1), -1);
    T_INT(pi_digit(200), -1);
    T_INT(pi_digit(1000), -1);
    T_INT(pi_digit(199) >= 0, 1);
    T_INT(pi_digit(199) <= 9, 1);
}

static void test_match(void)
{
    T_SECTION("how far a guess got");

    T_INT(pi_match(1, "14159", 5), 5);
    T_INT(pi_match(1, "1416", 4), 3);        /* 5 is wrong */
    T_INT(pi_match(1, "2", 1), 0);
    T_INT(pi_match(0, "3", 1), 1);
    T_INT(pi_match(0, "", 0), 0);
    T_INT(pi_match(0, NULL, 3), 0);
    T_INT(pi_match(-1, "3", 1), 0);
    /* A guess that runs off the end stops at the end. */
    T_INT(pi_match(198, "79", 2) <= 2, 1);
    T_INT(pi_match(200, "1", 1), 0);
}

static void test_text(void)
{
    char buf[64];

    T_SECTION("the digits as a reader expects them");

    T_INT(pi_text(1, buf, (int)sizeof buf), 1);
    T_STR(buf, "3");

    T_INT(pi_text(2, buf, (int)sizeof buf), 3);
    T_STR(buf, "3.1");

    T_INT(pi_text(6, buf, (int)sizeof buf), 7);
    T_STR(buf, "3.14159");

    /* A space every five decimals after the point. */
    T_INT(pi_text(7, buf, (int)sizeof buf), 9);
    T_STR(buf, "3.14159 2");

    T_INT(pi_text(11, buf, (int)sizeof buf), 13);
    T_STR(buf, "3.14159 26535");

    T_INT(pi_text(12, buf, (int)sizeof buf), 15);
    T_STR(buf, "3.14159 26535 8");


    /* Errors, never a truncated string. */
    T_INT(pi_text(0, buf, (int)sizeof buf), -1);
    T_INT(pi_text(201, buf, (int)sizeof buf), -1);
    T_INT(pi_text(6, buf, 3), -1);
    T_INT(pi_text(6, NULL, 8), -1);
}

static void run_tests(void)
{
    test_table_against_spigot();
    test_table_bounds();
    test_match();
    test_text();
}

T_MAIN("test_pi")
