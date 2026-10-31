/* test_logbook.c - the study log (V2 feature F13).
 *
 * Day numbers are checked against the calendar: 2000-01-01 is day 0,
 * 2026-10-31 is day 9800, and 2024-02-29 exists because 2024 is a leap year.
 */

#include "test_util.h"

#include "logbook.h"

static char const FILE_TEXT[] =
    "# a note to self\n"
    "2026-10-26 30 pacer\n"
    "2026-10-27 20 cards\n"
    "2026-10-28 45 notes\n"
    "2026-10-29 15 cards\n"
    "\n"
    "2026-10-31 60 pacer\n"
    "oops, this line is broken\n"
    "2026-13-01 10 pacer\n"
    "2026-10-30 2000 pacer\n";

static void test_days(void)
{
    T_INT(log_day_number(2000, 1, 1), 0);
    T_INT(log_day_number(2000, 1, 2), 1);
    T_INT(log_day_number(2000, 3, 1), 60);
    T_INT(log_day_number(2001, 1, 1), 366);
    T_INT(log_day_number(2024, 2, 29), 8825);
    T_INT(log_day_number(2026, 10, 31), 9800);

    T_INT(log_day_number(2023, 2, 29), -1);   /* not a leap year */
    T_INT(log_day_number(2026, 13, 1), -1);
    T_INT(log_day_number(2026, 4, 31), -1);
    T_INT(log_day_number(1999, 12, 31), -1);
}

static void test_lines(void)
{
    log_entry_t e;
    char buf[LOG_LINE_MAX * 2];

    T_TRUE(log_parse("2026-10-31 25 pacer", &e));
    T_INT(e.year, 2026);
    T_INT(e.month, 10);
    T_INT(e.day, 31);
    T_INT(e.minutes, 25);
    T_STR(e.tool, "pacer");

    T_TRUE(log_format(&e, buf, (int)sizeof buf));
    T_STR(buf, "2026-10-31 25 pacer");

    T_FALSE(log_parse("", &e));
    T_FALSE(log_parse("2026-10-31", &e));
    T_FALSE(log_parse("2026-10-31 25", &e));
    T_FALSE(log_parse("2026-02-30 25 pacer", &e));
    T_FALSE(log_parse("2026-10-31 -5 pacer", &e));
    T_FALSE(log_parse("2026-10-31 2000 pacer", &e));
    T_FALSE(log_parse("2026-10-31 25 averylongtoolname", &e));

    /* Round trip through the line format keeps the fields. */
    e.minutes = 0;
    T_TRUE(log_format(&e, buf, (int)sizeof buf));
    T_TRUE(log_parse(buf, &e));
    T_INT(e.minutes, 0);
}

static void test_totals(void)
{
    log_entry_t all[LOG_MAX];
    int skipped = 0;
    int const n = log_parse_all(FILE_TEXT, all, LOG_MAX, &skipped);

    /* Five good lines; the broken one, the 13th month and the 2000 minutes
     * are all refused. */
    T_INT(n, 5);
    T_INT(skipped, 3);

    T_INT(log_total(all, n), 30 + 20 + 45 + 15 + 60);
    T_INT(log_tool_total(all, n, "pacer"), 90);
    T_INT(log_tool_total(all, n, "cards"), 35);
    T_INT(log_tool_total(all, n, "moles"), 0);

    T_INT(log_day_total(all, n, log_day_number(2026, 10, 27)), 20);
    T_INT(log_day_total(all, n, 9800), 60);
    T_INT(log_day_total(all, n, log_day_number(2026, 10, 30)), 0);

    T_INT(log_days_logged(all, n), 5);

    /* 26, 27, 28, 29 then a day off, then 31: the best run is four. */
    T_INT(log_longest_streak(all, n), 4);

    {
        int day = 0;

        T_TRUE(log_latest(all, n, &day));
        T_INT(day, 9800);
    }
}

static void test_streaks(void)
{
    log_entry_t e[4];

    /* Two sessions on one day count once. */
    T_TRUE(log_parse("2026-10-01 10 pacer", &e[0]));
    T_TRUE(log_parse("2026-10-01 15 cards", &e[1]));
    T_TRUE(log_parse("2026-10-02 20 notes", &e[2]));
    T_TRUE(log_parse("2026-10-06 5 notes", &e[3]));
    T_INT(log_days_logged(e, 4), 3);
    T_INT(log_longest_streak(e, 4), 2);
    T_INT(log_total(e, 4), 50);

    T_INT(log_longest_streak(e, 0), 0);
    T_FALSE(log_latest(e, 0, NULL));
}

static void test_empty(void)
{
    log_entry_t e[2];
    int skipped = 99;

    T_INT(log_parse_all("", e, 2, &skipped), 0);
    T_INT(skipped, 0);
    T_INT(log_parse_all(NULL, e, 2, &skipped), 0);
    T_INT(log_parse_all("# only a comment\n\n", e, 2, &skipped), 0);
    T_INT(skipped, 0);
    T_INT(log_total(e, 0), 0);
}

static void run_tests(void)
{
    T_SECTION("day numbers");
    test_days();
    T_SECTION("reading and writing a line");
    test_lines();
    T_SECTION("totals over a file");
    test_totals();
    T_SECTION("streaks");
    test_streaks();
    T_SECTION("an empty log");
    test_empty();
}

T_MAIN("test_logbook")
