/* test_formula.c - line and heading arithmetic for the formula booklets.
 * Spec: src/core/formula.h. */

#include <string.h>

#include "formula.h"
#include "test_util.h"

/* Written as parts so the offsets can be computed rather than counted by hand:
 * a test whose numbers are guesses tests the guesses. */
static char const *const parts[] = {
    "# Analysis and approaches HL\n",
    "1.2 arithmetic: u_n = u_1 + (n-1)d\n",
    "\n",
    "## Topic 3: geometry\n",
    "area of a triangle = (1/2)ab sin C\n",
    "### small heading\n",
    "not a heading # in the middle\n",
    "#hashtag without a space\n",
};
#define PARTS ((int)(sizeof parts / sizeof parts[0]))

static char book[512];

static void build(void)
{
    book[0] = 0;
    for (int i = 0; i < PARTS; i++)
        strcat(book, parts[i]);
}

static int part_start(int i)
{
    int off = 0;

    for (int k = 0; k < i; k++)
        off += (int)strlen(parts[k]);
    return off;
}

static void test_lines(void)
{
    int const len = (int)strlen(book);

    T_SECTION("counting and finding lines");

    T_INT(formula_line_count(book, len), PARTS);
    T_INT(formula_line_count("", 0), 0);
    T_INT(formula_line_count(NULL, 10), 0);
    /* A file with no trailing newline still has its last line. */
    T_INT(formula_line_count("a\nb", 3), 2);
    T_INT(formula_line_count("a\nb\n", 4), 2);

    T_INT(formula_line_start(book, len, 0), 0);
    T_INT(formula_line_start(book, len, 1), part_start(1));
    T_INT(formula_line_start(book, len, 2), part_start(2));
    T_INT(formula_line_start(book, len, 99), len);

    /* The end is the newline, so one before the next line's start. */
    T_INT(formula_line_end(book, len, 0), part_start(1) - 1);
    T_INT(formula_line_end(book, len, 2), part_start(3) - 1);
    T_INT(formula_line_end(book, len, PARTS - 1), len - 1);
}

static void test_offsets(void)
{
    int const len = (int)strlen(book);

    T_SECTION("which line a byte offset is on");

    T_INT(formula_line_of_offset(book, len, 0), 0);
    T_INT(formula_line_of_offset(book, len, part_start(1) - 1), 0);  /* newline */
    T_INT(formula_line_of_offset(book, len, part_start(1)), 1);
    T_INT(formula_line_of_offset(book, len, len - 1), PARTS - 1);
    T_INT(formula_line_of_offset(book, len, -1), -1);
    T_INT(formula_line_of_offset(book, len, len), -1);
}

static void test_headings(void)
{
    int const len = (int)strlen(book);

    T_SECTION("headings, and what is not one");

    T_INT(formula_heading_level(book, len, 0), 1);
    T_INT(formula_heading_level(book, len, 1), 0);
    T_INT(formula_heading_level(book, len, 3), 2);
    T_INT(formula_heading_level(book, len, 5), 3);
    T_INT(formula_heading_level(book, len, 6), 0);   /* '#' mid-line */
    T_INT(formula_heading_level(book, len, 7), 0);   /* no space after '#' */

    /* Depth 3 stops at every heading; depth 1 only at the top level. */
    T_INT(formula_next_heading(book, len, 1, 3), 3);
    T_INT(formula_next_heading(book, len, 3, 3), 3);   /* at a heading: stays */
    T_INT(formula_next_heading(book, len, 6, 3), 6);   /* none after: stays */
    /* There is only one '#' heading, at line 0, so depth 1 never moves. */
    T_INT(formula_next_heading(book, len, 1, 1), 1);
    T_INT(formula_next_heading(book, len, 0, 1), 0);

    /* Depth 2 is what the viewer uses: a topic, not every sub-heading. */
    T_INT(formula_next_heading(book, len, 1, 2), 3);
    /* After the '##' there is only a '###', which depth 2 skips. */
    T_INT(formula_next_heading(book, len, 4, 2), 4);
    T_INT(formula_next_heading(book, len, 6, 3), 6);

    T_INT(formula_prev_heading(book, len, 5, 3), 3);
    T_INT(formula_prev_heading(book, len, 5, 2), 3);
    T_INT(formula_prev_heading(book, len, 4, 2), 3);
    T_INT(formula_prev_heading(book, len, 3, 2), 0);
    T_INT(formula_prev_heading(book, len, 6, 2), 3);
    T_INT(formula_prev_heading(book, len, 0, 3), 0);   /* none before: stays */

    /* A file with no headings never moves. */
    T_INT(formula_next_heading("a\nb\n", 4, 0, 2), 0);
    T_INT(formula_prev_heading("a\nb\n", 4, 1, 2), 1);
}

static void run_tests(void)
{
    build();
    test_lines();
    test_offsets();
    test_headings();
}

T_MAIN("test_formula")
