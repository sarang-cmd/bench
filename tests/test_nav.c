/* test_nav.c - the back stack behind EXIT.  Spec: src/core/nav.h. */

#include "nav.h"
#include "test_util.h"

enum { MENU = 0, SETTINGS = 6, ABOUT = 7, HELP = 8, CPLEX = 9, ROOTS = 13 };

static void test_empty(void)
{
    T_SECTION("nothing to go back to");

    nav_reset();
    T_INT(nav_depth(), 0);
    T_INT(nav_pop(), -1);
    T_INT(nav_peek(), -1);
}

static void test_one_step(void)
{
    T_SECTION("one screen deep");

    nav_reset();
    nav_push(MENU);
    T_INT(nav_depth(), 1);
    T_INT(nav_peek(), MENU);
    T_INT(nav_depth(), 1);        /* peeking changes nothing */
    T_INT(nav_pop(), MENU);
    T_INT(nav_depth(), 0);
    T_INT(nav_pop(), -1);
}

static void test_walks_back_in_order(void)
{
    T_SECTION("menu, then settings, then about: EXIT twice");

    /* A push records the screen you are leaving, so opening Settings from the
     * menu records the menu, and opening About from Settings records Settings. */
    nav_reset();
    nav_push(MENU);       /* opening Settings */
    nav_push(SETTINGS);   /* opening About */
    T_INT(nav_depth(), 2);
    T_INT(nav_pop(), SETTINGS);
    T_INT(nav_pop(), MENU);
    T_INT(nav_depth(), 0);
    T_INT(nav_pop(), -1);   /* nothing left: main.c shows the home screen */
}

static void test_same_screen_twice(void)
{
    T_SECTION("pushing the screen already on top");

    nav_reset();
    nav_push(CPLEX);
    nav_push(CPLEX);
    T_INT(nav_depth(), 1);
    T_INT(nav_pop(), CPLEX);

    /* But a screen that differs from the top is a real step. */
    nav_reset();
    nav_push(CPLEX);
    nav_push(ROOTS);
    nav_push(CPLEX);
    T_INT(nav_depth(), 3);
    T_INT(nav_pop(), CPLEX);
    T_INT(nav_pop(), ROOTS);
    T_INT(nav_pop(), CPLEX);
}

static void test_bounded(void)
{
    T_SECTION("more screens than the stack holds");

    nav_reset();
    for (int i = 0; i < NAV_DEPTH + 4; i++)
        nav_push(MENU + i);
    T_INT(nav_depth(), NAV_DEPTH);
    /* The oldest four fell off; the newest NAV_DEPTH remain, newest last. */
    T_INT(nav_pop(), MENU + NAV_DEPTH + 3);
    T_INT(nav_pop(), MENU + NAV_DEPTH + 2);
    for (int i = 0; i < NAV_DEPTH - 2; i++)
        nav_pop();
    T_INT(nav_depth(), 0);
    T_INT(nav_pop(), -1);
}

static void test_never_corrupts(void)
{
    T_SECTION("push and pop in a long loop");

    nav_reset();
    for (int i = 0; i < 500; i++)
    {
        nav_push(MENU + (i % 5));
        if (i % 3 == 0)
            nav_pop();
        T_CHECK(nav_depth() >= 0 && nav_depth() <= NAV_DEPTH,
                "depth %d out of range at %d", nav_depth(), i);
    }
}

static void run_tests(void)
{
    test_empty();
    test_one_step();
    test_walks_back_in_order();
    test_same_screen_twice();
    test_bounded();
    test_never_corrupts();
}

T_MAIN("test_nav")
