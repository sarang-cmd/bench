/* test_history.c - the last few results. */

#include <stdio.h>
#include <string.h>

#include "test_util.h"

#include "history.h"

static void empty_and_push(void)
{
    history_t h;

    T_SECTION("a fresh history holds nothing");

    history_clear(&h);
    T_INT(history_count(&h), 0);
    T_TRUE(history_at(&h, 0) == NULL);
    T_TRUE(history_at(&h, -1) == NULL);

    T_SECTION("the newest entry is at 0");
    T_TRUE(history_push(&h, "root 1.414214"));
    T_INT(history_count(&h), 1);
    T_STR(history_at(&h, 0), "root 1.414214");
    T_TRUE(history_at(&h, 1) == NULL);

    T_TRUE(history_push(&h, "root 2.000000"));
    T_INT(history_count(&h), 2);
    T_STR(history_at(&h, 0), "root 2.000000");
    T_STR(history_at(&h, 1), "root 1.414214");
}

static void refuses_bad_lines(void)
{
    history_t h;

    T_SECTION("empty, NULL and over-long lines are refused, and change nothing");
    history_clear(&h);
    T_FALSE(history_push(&h, ""));
    T_FALSE(history_push(&h, NULL));
    T_FALSE(history_push(NULL, "x"));
    T_INT(history_count(&h), 0);

    /* 40 characters fit, 41 do not. */
    T_TRUE(history_push(&h, "1234567890123456789012345678901234567890"));
    T_FALSE(history_push(&h, "12345678901234567890123456789012345678901"));
    T_INT(history_count(&h), 1);
    T_INT((int)strlen(history_at(&h, 0)), 40);
}

static void drops_the_oldest(void)
{
    history_t h;
    char line[HISTORY_LINE_MAX + 1];

    T_SECTION("a full history drops the oldest entry");
    history_clear(&h);
    for (int i = 0; i < HISTORY_MAX + 2; i++)
    {
        snprintf(line, sizeof line, "result %d", i);
        T_TRUE(history_push(&h, line));
    }
    T_INT(history_count(&h), HISTORY_MAX);
    T_STR(history_at(&h, 0), "result 9");
    T_STR(history_at(&h, HISTORY_MAX - 1), "result 2");
    T_TRUE(history_at(&h, HISTORY_MAX) == NULL);
}

static void run_tests(void)
{
    empty_and_push();
    refuses_bad_lines();
    drops_the_oldest();
}

T_MAIN("test_history")
