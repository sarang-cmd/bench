/* test_pageind.c - the page label and arrow states.
 * Spec: src/core/pageind.h, and the "too subtle" report about the dots. */

#include <string.h>

#include "pageind.h"
#include "test_util.h"

static void test_label(void)
{
    char buf[16];

    T_SECTION("the page label reads one-based");

    T_INT(pageind_label(0, 2, buf, (int)sizeof buf), 5);
    T_STR(buf, "1 / 2");

    T_INT(pageind_label(1, 2, buf, (int)sizeof buf), 5);
    T_STR(buf, "2 / 2");

    T_INT(pageind_label(0, 3, buf, (int)sizeof buf), 5);
    T_STR(buf, "1 / 3");

    T_INT(pageind_label(9, 12, buf, (int)sizeof buf), 7);
    T_STR(buf, "10 / 12");

    T_INT(pageind_label(11, 12, buf, (int)sizeof buf), 7);
    T_STR(buf, "12 / 12");
}

static void test_label_errors(void)
{
    char buf[8];

    T_SECTION("a label is never truncated or invented");

    T_INT(pageind_label(0, 0, buf, (int)sizeof buf), -1);
    T_INT(pageind_label(-1, 2, buf, (int)sizeof buf), -1);
    T_INT(pageind_label(2, 2, buf, (int)sizeof buf), -1);
    T_INT(pageind_label(0, 2, buf, 4), -1);       /* "1 / 2" needs 6 bytes */
    T_INT(pageind_label(9, 12, buf, 7), -1);      /* "10 / 12" plus NUL is 8 */
    T_INT(pageind_label(0, 2, NULL, 8), -1);
}

static void test_arrows(void)
{
    T_SECTION("which ends have a page");

    T_FALSE(pageind_left(0));
    T_TRUE(pageind_left(1));
    T_TRUE(pageind_left(2));

    T_TRUE(pageind_right(0, 2));
    T_FALSE(pageind_right(1, 2));
    T_FALSE(pageind_right(2, 3));     /* past the end: no arrow */
    T_FALSE(pageind_right(0, 1));
}

static void test_step(void)
{
    T_SECTION("a step stays inside the range");

    T_INT(pageind_step(0, 3, 1), 1);
    T_INT(pageind_step(1, 3, 1), 2);
    T_INT(pageind_step(2, 3, 1), 2);   /* already last */

    T_INT(pageind_step(2, 3, -1), 1);
    T_INT(pageind_step(0, 3, -1), 0);  /* already first */

    /* A stale page from a list that shrank lands on the last page. */
    T_INT(pageind_step(7, 3, 0), 2);
    T_INT(pageind_step(-2, 3, 0), 0);
    T_INT(pageind_step(0, 0, 1), 0);
}

static void run_tests(void)
{
    test_label();
    test_label_errors();
    test_arrows();
    test_step();
}

T_MAIN("test_pageind")
