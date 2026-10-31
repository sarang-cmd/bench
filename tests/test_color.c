/* test_color.c - the palette maths the contrast check stands on.
 *
 * Spec: docs/design.md section 4.  The numbers here are the ones the design
 * doc quotes, so the doc and the code cannot drift apart silently. */

#include "color.h"
#include "test_util.h"

/* The palette, as 8-bit hex from docs/design.md.  Kept here as literals so a
 * wrong 5-6-5 constant in theme.h fails this test instead of looking fine. */
#define PAPER 0xf4, 0xf3, 0xee
#define INK 0x2f, 0x2d, 0x2e
#define RED 0xd9, 0x2d, 0x27
#define YELLOW 0xf2, 0xc2, 0x30
#define BLUE 0x2f, 0x5f, 0xd0
#define TEAL 0x1f, 0x9c, 0x8a
#define VIOLET 0x8b, 0x5c, 0xf6
#define SHADE 0xd9, 0xd7, 0xcd

static void test_pack(void)
{
    T_SECTION("5-6-5 packing");

    T_INT(col_rgb565(0, 0, 0), 0x0000);
    T_INT(col_rgb565(255, 255, 255), 0xFFFF);
    T_INT(col_rgb565(255, 0, 0), 0xF800);
    T_INT(col_rgb565(0, 255, 0), 0x07E0);
    T_INT(col_rgb565(0, 0, 255), 0x001F);

    /* Rounding, not truncation: 0x80 red is nearer 16/31 than 15/31. */
    T_INT(col_rgb565(128, 0, 0), 0x8000);

    /* Out of range values clamp instead of bleeding into the next channel. */
    T_INT(col_rgb565(-4, -4, -4), 0x0000);
    T_INT(col_rgb565(300, 300, 300), 0xFFFF);
}

static void test_unpack(void)
{
    int r = -1;
    int g = -1;
    int b = -1;

    T_SECTION("5-6-5 unpacking");

    col_rgb888(0xFFFF, &r, &g, &b);
    T_INT(r, 255);
    T_INT(g, 255);
    T_INT(b, 255);

    col_rgb888(0x0000, &r, &g, &b);
    T_INT(r, 0);
    T_INT(g, 0);
    T_INT(b, 0);

    /* NULL pointers are allowed: the parser calls it for one channel only. */
    col_rgb888(0xFFFF, NULL, NULL, NULL);

    /* A round trip through 5-6-5 stays within one 8-bit step of the pack. */
    for (int v = 0; v <= 255; v += 17)
    {
        int got = 0;

        col_rgb888(col_rgb565(v, v, v), &got, NULL, NULL);
        T_CHECK(got - v <= 8 && v - got <= 8, "grey %d came back as %d", v, got);
    }
}

static void test_luminance(void)
{
    T_SECTION("relative luminance");

    T_NEAR(col_luminance(0x0000), 0.0, 0.0005);
    T_NEAR(col_luminance(0xFFFF), 1.0, 0.0005);

    /* Paper is light, ink is dark, and the order holds across the palette. */
    T_CHECK(col_luminance(col_rgb565(PAPER)) > 0.8, "paper should be light");
    T_CHECK(col_luminance(col_rgb565(INK)) < 0.05, "ink should be dark");
    T_CHECK(col_luminance(col_rgb565(YELLOW)) > col_luminance(col_rgb565(TEAL)),
            "yellow should be lighter than teal");
    T_CHECK(col_luminance(col_rgb565(TEAL)) > col_luminance(col_rgb565(BLUE)),
            "teal should be lighter than blue");
    /* Violet is lighter than blue and darker than teal; measured, not assumed
     * (the first version of this test guessed the other way round). */
    T_CHECK(col_luminance(col_rgb565(VIOLET)) > col_luminance(col_rgb565(BLUE)),
            "violet should be lighter than blue");
    T_CHECK(col_luminance(col_rgb565(TEAL)) > col_luminance(col_rgb565(VIOLET)),
            "teal should be lighter than violet");
}

static void test_contrast(void)
{
    T_SECTION("contrast ratios");

    /* WCAG anchors: identical colours are 1:1, black on white is 21:1. */
    T_NEAR(col_contrast(col_rgb565(PAPER), col_rgb565(PAPER)), 1.0, 0.001);
    T_NEAR(col_contrast(0x0000, 0xFFFF), 21.0, 0.05);

    /* Symmetry. */
    T_NEAR(col_contrast(0x0000, 0xFFFF), col_contrast(0xFFFF, 0x0000), 1e-9);

    /* Ink on paper is the page's body text and must clear AA easily. */
    T_CHECK(col_contrast(col_rgb565(INK), col_rgb565(PAPER)) >= COL_TEXT_MIN_CONTRAST,
            "ink on paper is the body text and must clear 4.5:1");

    /* Every accent is a tile fill.  The palette only has ink and paper to put
     * on top, and on red, teal and violet neither reaches 4.5:1, so the rule
     * from docs/design.md is: the title takes the better of the two and must
     * clear the large-text floor of 3:1; the subtitle takes the better of the
     * two and must clear 4.5:1, or it goes on a paper plate.
     *
     * This test is the reason the policy exists rather than "always ink":
     * these are the measured numbers. */
    int const accents[] = { col_rgb565(RED), col_rgb565(YELLOW),
                            col_rgb565(BLUE), col_rgb565(TEAL),
                            col_rgb565(VIOLET), col_rgb565(SHADE) };
    char const *const names[] = { "red", "yellow", "blue", "teal", "violet",
                                  "shade" };

    for (int i = 0; i < 6; i++)
    {
        int const ink = col_rgb565(INK);
        int const paper = col_rgb565(PAPER);
        int const title = col_better_on(accents[i], ink, paper);
        double const title_ratio = col_contrast(title, accents[i]);

        T_CHECK(title_ratio >= COL_TITLE_MIN_CONTRAST,
                "title on %s is %.2f:1, under the 3:1 floor", names[i],
                title_ratio);
        T_CHECK(title != accents[i],
                "title colour on %s equals the fill", names[i]);

        /* Whenever the better foreground is under 4.5:1 the subtitle is drawn
         * on paper, and ink on paper has to be readable by a wide margin. */
        if (title_ratio < COL_TEXT_MIN_CONTRAST)
            T_CHECK(col_contrast(col_rgb565(INK), col_rgb565(PAPER)) >=
                        COL_TEXT_MIN_CONTRAST,
                    "the plate fallback on %s must be readable", names[i]);
    }

    /* Blue and violet are the two accents the plan named for paper text; the
     * numbers agree for blue, and violet is the case the plan did not foresee
     * (paper on violet is 3.79:1, so violet gets a plate). */
    T_CHECK(col_contrast(col_rgb565(PAPER), col_rgb565(BLUE)) >=
                COL_TEXT_MIN_CONTRAST,
            "paper on blue must clear 4.5:1");
    T_CHECK(col_contrast(col_rgb565(PAPER), col_rgb565(VIOLET)) <
                COL_TEXT_MIN_CONTRAST,
            "paper on violet is under 4.5:1, so violet must use a plate");
}

static void test_icon_floor(void)
{
    T_SECTION("the icon floor");

    /* The bug this whole workstream exists for: an icon drawn in the same
     * colour as its tile has a ratio of exactly 1 and is invisible. */
    T_NEAR(col_contrast(col_rgb565(RED), col_rgb565(RED)), 1.0, 0.001);
    T_CHECK(col_contrast(col_rgb565(RED), col_rgb565(RED)) < COL_ICON_MIN_CONTRAST,
            "an icon on its own colour must fail the floor");

    /* Paper icons on each accent clear the 1.5:1 floor... */
    int const accents[] = { col_rgb565(RED), col_rgb565(YELLOW),
                            col_rgb565(BLUE), col_rgb565(TEAL),
                            col_rgb565(VIOLET), col_rgb565(SHADE) };
    char const *const names[] = { "red", "yellow", "blue", "teal", "violet",
                                  "shade" };

    /* A selected tile draws its icon in the better of ink and paper, so every
     * accent has to give one of them at least 1.5:1.  Shade is the case that
     * forces the rule: a paper icon on the shade fill is 1.31:1 and invisible,
     * which is exactly the Settings tile the old code got away with only
     * because it drew a black ring. */
    T_CHECK(col_contrast(col_rgb565(PAPER), col_rgb565(SHADE)) <
                COL_ICON_MIN_CONTRAST,
            "paper on shade is under the floor, so shade needs an ink icon");

    for (int i = 0; i < 6; i++)
    {
        int const icon = col_better_on(accents[i], col_rgb565(INK),
                                       col_rgb565(PAPER));

        T_CHECK(col_contrast(icon, accents[i]) >= COL_ICON_MIN_CONTRAST,
                "best icon on %s is %.2f:1, under the 1.5:1 floor", names[i],
                col_contrast(icon, accents[i]));
        T_CHECK(icon != accents[i], "icon on %s equals the fill", names[i]);
    }

    /* Unselected tiles are paper: the accent icon has to read there too, or
     * the tile falls back to ink. */
    for (int i = 0; i < 6; i++)
    {
        int const icon = col_contrast(accents[i], col_rgb565(PAPER)) >=
                                 COL_ICON_MIN_CONTRAST
                             ? accents[i]
                             : col_rgb565(INK);

        T_CHECK(col_contrast(icon, col_rgb565(PAPER)) >= COL_ICON_MIN_CONTRAST,
                "%s icon on paper is %.2f:1", names[i],
                col_contrast(icon, col_rgb565(PAPER)));
    }
}

static void run_tests(void)
{
    test_pack();
    test_unpack();
    test_luminance();
    test_contrast();
    test_icon_floor();
}

T_MAIN("test_color")
