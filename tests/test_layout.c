/* test_layout.c - the row centring the Tools category needs.
 * Spec: src/core/layout.h. */

#include "layout.h"
#include "test_util.h"

static void test_full_row(void)
{
    T_SECTION("a full row keeps the margin");

    /* 384 wide, three tiles of 121 with 4 between: 371 used, 6 left over. */
    T_INT(layout_centre_row(3, 121, 4, 384), 6);
    /* 3 x 100 + 2 x 4 = 308, so 38 each side. */
    T_INT(layout_centre_row(3, 100, 4, 384), 38);
    T_INT(layout_centre_row(1, 384, 4, 384), 0);
}

static void test_short_row(void)
{
    T_SECTION("a short row moves to the middle");

    /* Two tiles of 121 and a gap: 246 used in 384, so 69 each side. */
    T_INT(layout_centre_row(2, 121, 4, 384), 69);
    /* One tile: (384 - 121) / 2. */
    T_INT(layout_centre_row(1, 121, 4, 384), 131);
    /* The same two tiles in a narrow window still clear the edge. */
    T_INT(layout_centre_row(2, 100, 4, 320), 58);
}

static void test_edges(void)
{
    T_SECTION("edges that must not go negative");

    T_INT(layout_centre_row(0, 121, 4, 384), 0);
    T_INT(layout_centre_row(4, 121, 4, 384), 0);   /* wider than the span */
    T_INT(layout_centre_row(3, 200, 4, 384), 0);
    T_INT(layout_centre_row(2, 0, 4, 384), 0);
    T_INT(layout_centre_row(-1, 121, 4, 384), 0);
}

static void test_row_count(void)
{
    T_SECTION("how many tiles a row holds");

    T_INT(layout_row_count(0, 9, 3), 3);
    T_INT(layout_row_count(3, 9, 3), 3);
    T_INT(layout_row_count(6, 9, 3), 3);
    T_INT(layout_row_count(0, 2, 3), 2);
    T_INT(layout_row_count(0, 1, 3), 1);
    T_INT(layout_row_count(1, 2, 3), 1);
    T_INT(layout_row_count(0, 4, 3), 3);
    T_INT(layout_row_count(3, 4, 3), 1);
    T_INT(layout_row_count(0, 0, 3), 0);
    T_INT(layout_row_count(0, 5, 0), 0);
}

static void run_tests(void)
{
    test_full_row();
    test_short_row();
    test_edges();
    test_row_count();
}

T_MAIN("test_layout")
