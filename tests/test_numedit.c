/* test_numedit.c - the shared number field editor. */

#include <string.h>

#include "test_util.h"

#include "numedit.h"

static void typing(void)
{
    char buf[NUMEDIT_MAX] = "";

    T_SECTION("digits append in order");

    T_TRUE(numedit_put(buf, sizeof buf, '1'));
    T_TRUE(numedit_put(buf, sizeof buf, '2'));
    T_TRUE(numedit_put(buf, sizeof buf, '.'));
    T_TRUE(numedit_put(buf, sizeof buf, '5'));
    T_STR(buf, "12.5");
    T_TRUE(numedit_valid(buf));

    T_SECTION("a second dot is refused and changes nothing");
    T_FALSE(numedit_put(buf, sizeof buf, '.'));
    T_STR(buf, "12.5");

    T_SECTION("a lone zero is replaced, not stacked");
    numedit_set(buf, sizeof buf, "");
    T_TRUE(numedit_put(buf, sizeof buf, '0'));
    T_STR(buf, "0");
    T_TRUE(numedit_put(buf, sizeof buf, '5'));
    T_STR(buf, "5");
    /* ...but 0.5 keeps its zero, because the dot made it a number. */
    numedit_set(buf, sizeof buf, "");
    T_TRUE(numedit_put(buf, sizeof buf, '0'));
    T_TRUE(numedit_put(buf, sizeof buf, '.'));
    T_STR(buf, "0.");
    T_TRUE(numedit_put(buf, sizeof buf, '5'));
    T_STR(buf, "0.5");

    T_SECTION("a leading dot becomes 0.");
    numedit_set(buf, sizeof buf, "");
    T_TRUE(numedit_put(buf, sizeof buf, '.'));
    T_STR(buf, "0.");

    T_SECTION("anything that is not a digit or a dot is refused");
    numedit_set(buf, sizeof buf, "7");
    T_FALSE(numedit_put(buf, sizeof buf, 'x'));
    T_FALSE(numedit_put(buf, sizeof buf, ' '));
    T_FALSE(numedit_put(buf, sizeof buf, '-'));
    T_STR(buf, "7");
}

static void limits(void)
{
    char buf[4] = "";

    T_SECTION("the field stops at size - 1 characters");
    T_TRUE(numedit_put(buf, sizeof buf, '1'));
    T_TRUE(numedit_put(buf, sizeof buf, '2'));
    T_TRUE(numedit_put(buf, sizeof buf, '3'));
    T_FALSE(numedit_put(buf, sizeof buf, '4'));
    T_STR(buf, "123");

    T_SECTION("a full buffer refuses even a dot");
    T_FALSE(numedit_put(buf, sizeof buf, '.'));
    T_STR(buf, "123");
}

static void backspace(void)
{
    char buf[NUMEDIT_MAX];

    T_SECTION("DEL removes one character at a time");
    numedit_set(buf, sizeof buf, "12.5");
    T_TRUE(numedit_back(buf));
    T_STR(buf, "12.");
    T_TRUE(numedit_back(buf));
    T_STR(buf, "12");
    T_TRUE(numedit_back(buf));
    T_TRUE(numedit_back(buf));
    T_STR(buf, "");
    T_FALSE(numedit_back(buf));
    T_STR(buf, "");
}

static void setting_and_validity(void)
{
    char buf[8];

    T_SECTION("set truncates rather than overflowing");
    numedit_set(buf, sizeof buf, "123456789");
    T_STR(buf, "1234567");
    numedit_set(buf, sizeof buf, NULL);
    T_STR(buf, "");

    T_SECTION("validity matches what numparse accepts");
    T_TRUE(numedit_valid("0"));
    T_TRUE(numedit_valid("12"));
    T_TRUE(numedit_valid("0.5"));
    T_TRUE(numedit_valid("-3"));
    T_TRUE(numedit_valid("-0.25"));
    T_FALSE(numedit_valid(""));
    T_FALSE(numedit_valid("."));
    T_FALSE(numedit_valid("-"));
    T_FALSE(numedit_valid("1.2.3"));
    T_FALSE(numedit_valid("1e5"));
    T_FALSE(numedit_valid(" 7"));
    T_FALSE(numedit_valid("--1"));
    T_FALSE(numedit_valid(NULL));
}

static void run_tests(void)
{
    typing();
    limits();
    backspace();
    setting_and_validity();
}

T_MAIN("test_numedit")
