/* test_exprs.c - the six named expressions and their file format (V2 F1). */

#include <string.h>

#include "exprs.h"
#include "test_util.h"

static void test_names(void)
{
    T_SECTION("slot names");

    T_STR(exprs_name(0), "f");
    T_STR(exprs_name(1), "g");
    T_STR(exprs_name(2), "h");
    T_STR(exprs_name(3), "p");
    T_STR(exprs_name(4), "q");
    T_STR(exprs_name(5), "r");
    T_CHECK(exprs_name(6) == NULL, "slot 6 has no name");
    T_CHECK(exprs_name(-1) == NULL, "slot -1 has no name");
}

static void test_round_trip(void)
{
    T_SECTION("format and parse");

    expr_slot_t slots[EXPR_SLOTS];
    expr_slot_t back[EXPR_SLOTS];
    char buf[512];
    int len;

    for (int i = 0; i < EXPR_SLOTS; i++)
        slots[i].text[0] = 0;
    exprs_set(slots, 0, "x^2+1");
    exprs_set(slots, 3, "sin(x)");

    len = exprs_format(slots, buf, (int)sizeof buf);
    T_CHECK(len > 0, "format returned %d", len);
    T_STR(buf, "f=x^2+1\n"
               "g=\n"
               "h=\n"
               "p=sin(x)\n"
               "q=\n"
               "r=\n");

    T_INT(exprs_parse(buf, back), 6);
    T_STR(back[0].text, "x^2+1");
    T_STR(back[1].text, "");
    T_STR(back[3].text, "sin(x)");
    T_TRUE(exprs_same(slots, back));
}

static void test_parse_is_forgiving(void)
{
    T_SECTION("bad lines are skipped");

    expr_slot_t slots[EXPR_SLOTS];
    char const *text = "# a comment\r\n"
                       "f=2x\r\n"
                       "zz=3\r\n"
                       "g\n"
                       "=nothing\n"
                       "q=a=b\n"
                       "\n"
                       "r=1+1\n";

    T_INT(exprs_parse(text, slots), 3);
    T_STR(slots[0].text, "2x");
    T_STR(slots[1].text, "");
    T_STR(slots[4].text, "a=b");
    T_STR(slots[5].text, "1+1");

    /* A short file leaves empty slots rather than stale text. */
    exprs_set(slots, 0, "stale");
    T_INT(exprs_parse("g=1\n", slots), 1);
    T_STR(slots[0].text, "");
    T_STR(slots[1].text, "1");
    T_INT(exprs_parse(NULL, slots), 0);
    T_STR(slots[1].text, "");
}

static void test_limits(void)
{
    T_SECTION("limits");

    expr_slot_t slots[EXPR_SLOTS];
    char buf[EXPR_TEXT_MAX + 32];
    char out[256];
    char longtext[EXPR_TEXT_MAX + 20];

    for (int i = 0; i < EXPR_SLOTS; i++)
        slots[i].text[0] = 0;

    /* A stored expression is cut to the slot, not past it. */
    for (int i = 0; i < (int)sizeof longtext - 1; i++)
        longtext[i] = '7';
    longtext[sizeof longtext - 1] = 0;
    T_INT(exprs_set(slots, 0, longtext), EXPR_TEXT_MAX - 1);

    /* A line longer than the slot is cut on the way in too. */
    snprintf(buf, sizeof buf, "f=%s\n", longtext);
    exprs_parse(buf, slots);
    T_INT((int)strlen(slots[0].text), EXPR_TEXT_MAX - 1);

    /* Formatting refuses a buffer that cannot hold all six slots. */
    T_INT(exprs_format(slots, out, 8), -1);
    T_TRUE(exprs_format(slots, out, (int)sizeof out) > 0);

    /* Out-of-range slots are ignored. */
    T_INT(exprs_set(slots, 9, "x"), 0);
    T_INT(exprs_set(slots, -1, "x"), 0);
}

static void test_same(void)
{
    T_SECTION("comparison");

    expr_slot_t a[EXPR_SLOTS];
    expr_slot_t b[EXPR_SLOTS];

    for (int i = 0; i < EXPR_SLOTS; i++)
    {
        a[i].text[0] = 0;
        b[i].text[0] = 0;
    }
    T_TRUE(exprs_same(a, b));
    exprs_set(b, 5, "x");
    T_TRUE(!exprs_same(a, b));
    exprs_set(a, 5, "x");
    T_TRUE(exprs_same(a, b));
}

static void run_tests(void)
{
    test_names();
    test_round_trip();
    test_parse_is_forgiving();
    test_limits();
    test_same();
}

T_MAIN("test_exprs")
