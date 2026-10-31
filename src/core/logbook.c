/* logbook.c - see logbook.h. */

#include "logbook.h"

#include <stdio.h>
#include <string.h>

static bool leap(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

static int month_len(int y, int m)
{
    static int const LEN[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (m < 1 || m > 12) return 0;
    return m == 2 && leap(y) ? 29 : LEN[m - 1];
}

int log_day_number(int year, int month, int day)
{
    /* Days from 2000-01-01, by the usual civil-from-days formula, clamped to a
     * range a school career fits in. */
    int y = year - (month <= 2);
    int const era = (y >= 0 ? y : y - 399) / 400;
    int const yoe = y - era * 400;
    int const doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    int const doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

    if (year < 2000 || year > 2099) return -1;
    if (month < 1 || month > 12) return -1;
    if (day < 1 || day > month_len(year, month)) return -1;
    return era * 146097 + doe - 730425;   /* 730425 = days to 2000-01-01 */
}

bool log_parse(char const *line, log_entry_t *out)
{
    int y, m, d, minutes;
    char tool[32];
    int used = 0;

    (void)used;

    if (!line || !out) return false;
    if (sscanf(line, "%4d-%2d-%2d %4d %31s%n", &y, &m, &d, &minutes, tool, &used)
        != 5)
        return false;
    if (log_day_number(y, m, d) < 0) return false;
    if (minutes < 0 || minutes > 1440) return false;
    if (!tool[0] || strlen(tool) > LOG_TOOL_MAX) return false;

    out->year = y;
    out->month = m;
    out->day = d;
    out->minutes = minutes;
    strcpy(out->tool, tool);
    return true;
}

bool log_format(log_entry_t const *e, char *buf, int size)
{
    int const n = snprintf(buf, (size_t)(size > 0 ? size : 0), "%04d-%02d-%02d %d %s",
                           e->year, e->month, e->day, e->minutes, e->tool);

    return n > 0 && n < size;
}

int log_parse_all(char const *text, log_entry_t *out, int max, int *skipped)
{
    int count = 0;
    int bad = 0;
    /* Roomier than a real line (LOG_LINE_MAX is 32), so a long comment is
     * still one line: the old buffer split a 70-character comment into a
     * comment plus a broken line, and the totals gained a phantom skip. */
    char line[128];
    bool too_long;

    if (!text) {
        if (skipped) *skipped = 0;
        return 0;
    }

    while (*text) {
        int n = 0;

        too_long = false;
        while (*text && *text != '\n')
        {
            if (n < (int)sizeof line - 1)
                line[n++] = *text;
            else
                too_long = true;               /* swallow the rest of the line */
            text++;
        }
        line[n] = 0;
        if (*text == '\n') text++;

        if (!n && !too_long) continue;         /* blank line */
        if (!too_long && line[0] == '#') continue;  /* a note to self */
        if (too_long)
        {
            bad++;
            continue;
        }
        {
            log_entry_t e;

            if (log_parse(line, &e)) {
                if (count < max) out[count++] = e;
            } else
                bad++;
        }
    }
    if (skipped) *skipped = bad;
    return count;
}

int log_total(log_entry_t const *e, int n)
{
    int sum = 0;

    for (int i = 0; i < n; i++)
        if (e[i].minutes > 0) sum += e[i].minutes;
    return sum;
}

int log_day_total(log_entry_t const *e, int n, int day_num)
{
    int sum = 0;

    for (int i = 0; i < n; i++)
        if (log_day_number(e[i].year, e[i].month, e[i].day) == day_num)
            sum += e[i].minutes;
    return sum;
}

int log_tool_total(log_entry_t const *e, int n, char const *tool)
{
    int sum = 0;

    for (int i = 0; i < n; i++)
        if (strcmp(e[i].tool, tool) == 0) sum += e[i].minutes;
    return sum;
}

int log_days_logged(log_entry_t const *e, int n)
{
    int days = 0;

    for (int i = 0; i < n; i++) {
        bool seen = false;

        for (int j = 0; j < i && !seen; j++)
            if (log_day_number(e[i].year, e[i].month, e[i].day)
                == log_day_number(e[j].year, e[j].month, e[j].day))
                seen = true;
        if (!seen) days++;
    }
    return days;
}

int log_longest_streak(log_entry_t const *e, int n)
{
    /* Insertion sort of the distinct day numbers (a log is small), then the
     * longest run of +1 steps. */
    int days[LOG_MAX];
    int count = 0;
    int best = 0;
    int run = 1;

    for (int i = 0; i < n && count < LOG_MAX; i++) {
        int const d = log_day_number(e[i].year, e[i].month, e[i].day);
        int at = count;
        bool dup = false;

        if (d < 0) continue;
        while (at > 0 && days[at - 1] > d) at--;
        for (int k = 0; k < count; k++)
            if (days[k] == d) dup = true;
        if (dup) continue;
        memmove(&days[at + 1], &days[at], (size_t)(count - at) * sizeof days[0]);
        days[at] = d;
        count++;
    }

    for (int i = 0; i < count; i++) {
        if (i > 0 && days[i] == days[i - 1] + 1)
            run++;
        else
            run = 1;
        if (run > best) best = run;
    }
    return best;
}

bool log_latest(log_entry_t const *e, int n, int *day_num)
{
    int best = -1;

    for (int i = 0; i < n; i++) {
        int const d = log_day_number(e[i].year, e[i].month, e[i].day);

        if (d > best) best = d;
    }
    if (best < 0) return false;
    if (day_num) *day_num = best;
    return true;
}
