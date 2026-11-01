/* test_hwreport.c - the eight assumptions and the report they turn into. */

#include <stdio.h>
#include <string.h>

#include "test_util.h"

#include "hwreport.h"

static void labels_and_states(void)
{
    T_SECTION("every row has a label and a line of detail");

    for (int i = 0; i < HWREPORT_ITEMS; i++)
    {
        T_TRUE(hwreport_label(i)[0] != 0);
        T_TRUE(hwreport_detail(i)[0] != 0);
        /* The detail starts with the row number, so the screen and the report
         * cannot drift apart. */
        T_INT(hwreport_detail(i)[0], '1' + i);
    }
    T_STR(hwreport_label(-1), "");
    T_STR(hwreport_label(HWREPORT_ITEMS), "");
    T_STR(hwreport_detail(HWREPORT_ITEMS), "");

    T_SECTION("the state cycle is unknown -> works -> fails -> unknown");
    T_STR(hwreport_state_text(HW_UNKNOWN), "not tried");
    T_STR(hwreport_state_text(HW_WORKS), "works");
    T_STR(hwreport_state_text(HW_FAILS), "fails");
    T_INT(hwreport_next(HW_UNKNOWN), HW_WORKS);
    T_INT(hwreport_next(HW_WORKS), HW_FAILS);
    T_INT(hwreport_next(HW_FAILS), HW_UNKNOWN);
    T_INT(hwreport_next(hwreport_next(hwreport_next(HW_UNKNOWN))), HW_UNKNOWN);
}

static void reset_and_count(void)
{
    hw_state_t s[HWREPORT_ITEMS];

    T_SECTION("reset marks nothing");
    for (int i = 0; i < HWREPORT_ITEMS; i++)
        s[i] = HW_WORKS;
    hwreport_reset(s);
    for (int i = 0; i < HWREPORT_ITEMS; i++)
        T_INT(s[i], HW_UNKNOWN);
    T_INT(hwreport_marked(s), 0);
    hwreport_reset(NULL); /* must not crash */

    T_SECTION("marked counts the rows that are no longer unknown");
    s[0] = HW_WORKS;
    s[3] = HW_FAILS;
    s[7] = HW_WORKS;
    T_INT(hwreport_marked(s), 3);
    T_INT(hwreport_marked(NULL), 0);
}

static void report_text(void)
{
    hw_state_t s[HWREPORT_ITEMS];
    char buf[1024];

    T_SECTION("the report names the version and every row");
    hwreport_reset(s);
    s[0] = HW_WORKS;
    s[1] = HW_FAILS;
    int n = hwreport_format("1.1.0", s, buf, (int)sizeof buf);
    T_TRUE(n > 0);
    T_INT((int)strlen(buf), n);
    T_TRUE(strstr(buf, "Bench hardware report\n") == buf);
    T_TRUE(strstr(buf, "version 1.1.0\n") != NULL);
    T_TRUE(strstr(buf, "1 BFile files") != NULL);
    T_TRUE(strstr(buf, "8 A frame costs") != NULL);
    T_TRUE(strstr(buf, "\n   works\n") != NULL);
    T_TRUE(strstr(buf, "\n   fails\n") != NULL);
    T_TRUE(strstr(buf, "\n   not tried\n") != NULL);
    /* Two marked out of eight. */
    T_TRUE(strstr(buf, "Some rows are still not tried.\n") != NULL);

    T_SECTION("a NULL version is written as unknown, not skipped");
    hwreport_format(NULL, s, buf, (int)sizeof buf);
    T_TRUE(strstr(buf, "version unknown\n") != NULL);
    hwreport_format("", s, buf, (int)sizeof buf);
    T_TRUE(strstr(buf, "version unknown\n") != NULL);

    T_SECTION("all eight marked says so");
    for (int i = 0; i < HWREPORT_ITEMS; i++)
        s[i] = HW_WORKS;
    hwreport_format("1.1.0", s, buf, (int)sizeof buf);
    T_TRUE(strstr(buf, "All eight marked.\n") != NULL);

    T_SECTION("a buffer that is too small returns -1 and writes nothing");
    char small[40];
    memset(small, 'x', sizeof small);
    T_INT(hwreport_format("1.1.0", s, small, (int)sizeof small), -1);
    T_INT(small[0], 'x');
    T_INT(hwreport_format("1.1.0", s, small, 0), -1);
    T_INT(hwreport_format("1.1.0", NULL, buf, (int)sizeof buf), -1);
    T_INT(hwreport_format("1.1.0", s, NULL, (int)sizeof buf), -1);
}

static void run_tests(void)
{
    labels_and_states();
    reset_and_count();
    report_text();
}

T_MAIN("test_hwreport")
