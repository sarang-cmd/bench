/* test_help.c - line counting, line copying and scroll clamping for the help
 * screen.  Spec: src/core/help.h and F in the V2 plan. */

#include <string.h>

#include "help.h"
#include "test_util.h"

static char const one[] = "Only one line";
static char const three[] = "First\n\nThird";
static char const tools[] =
    "Pacer\n"
    "Times a practice paper against a plan.\n"
    "\n"
    "1. Enter minutes, marks.\n"
    "2. EXE starts the clock.";

static void test_count(void)
{
    T_SECTION("counting lines");

    T_INT(help_line_count(NULL), 0);
    T_INT(help_line_count(""), 1);
    T_INT(help_line_count(one), 1);
    T_INT(help_line_count("a\nb"), 2);
    T_INT(help_line_count(three), 3);
    /* A blank line still ends in '\n' and counts as an empty line. */
    T_INT(help_line_count("a\n\nb\n"), 4);
    T_INT(help_line_count(tools), 5);
}

static void test_line(void)
{
    char buf[16];

    T_SECTION("copying a line");

    T_INT(help_line(three, 0, buf, (int)sizeof buf), 5);
    T_CHECK(strcmp(buf, "First") == 0, "line 0 is '%s'", buf);

    /* The empty middle line is empty, not skipped. */
    T_INT(help_line(three, 1, buf, (int)sizeof buf), 0);
    T_CHECK(buf[0] == 0, "line 1 is empty");

    T_INT(help_line(three, 2, buf, (int)sizeof buf), 5);
    T_CHECK(strcmp(buf, "Third") == 0, "line 2 is '%s'", buf);

    /* Past the end, and garbage arguments, are empty rather than wild. */
    T_INT(help_line(three, 3, buf, (int)sizeof buf), 0);
    T_INT(help_line(three, -1, buf, (int)sizeof buf), 0);
    T_INT(help_line(NULL, 0, buf, (int)sizeof buf), 0);

    /* A long line is cut to the buffer and stays NUL terminated. */
    T_CHECK(help_line(tools, 1, buf, 5) == 4, "cut to 4 bytes");
    T_CHECK(strcmp(buf, "Time") == 0, "cut line is '%s'", buf);
}

static void test_scroll(void)
{
    T_SECTION("scroll clamping");

    /* Five lines, three fit: two positions, 0 and 2. */
    T_CHECK(help_needs_scroll(tools, 3), "5 lines need scroll at 3");
    T_CHECK(!help_needs_scroll(tools, 5), "5 lines fit in 5");
    T_CHECK(help_needs_scroll(tools, 4), "5 lines need scroll at 4");

    T_INT(help_scroll_pos(tools, 0, 3, 1), 1);
    T_INT(help_scroll_pos(tools, 1, 3, 1), 2);
    T_INT(help_scroll_pos(tools, 2, 3, 1), 2);   /* stops at the last page */
    T_INT(help_scroll_pos(tools, 0, 3, -1), 0);  /* never negative */
    T_INT(help_scroll_pos(tools, 0, 3, 99), 2);  /* a held key cannot run off */
    T_INT(help_scroll_pos(tools, 2, 3, -99), 0);

    /* Scrolling is a no-op when everything fits. */
    T_INT(help_scroll_pos(tools, 0, 10, 1), 0);
    T_INT(help_scroll_pos(tools, 0, 0, 1), 0);

    /* The last line is always on screen at the last position. */
    T_INT(help_scroll_pos(one, 0, 3, 1), 0);
}

static void test_skip_title(void)
{
    T_SECTION("title line");

    T_INT(help_skip_title(tools, "Pacer"), 1);
    T_INT(help_skip_title(tools, "Cards"), 0);
    T_INT(help_skip_title(tools, "Pace"), 0);
    T_INT(help_skip_title(tools, ""), 0);
    T_INT(help_skip_title(tools, NULL), 0);
    /* A first line that is only a prefix of the title is not a match. */
    T_INT(help_skip_title("Pacerx\nmore", "Pacer"), 0);
    /* Nor is a title that is longer than the line. */
    T_INT(help_skip_title("Pac\nmore", "Pacer"), 0);
}

static void run_tests(void)
{
    test_count();
    test_line();
    test_scroll();
    test_skip_title();
}

T_MAIN("test_help")
