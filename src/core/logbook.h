/* logbook.h - the study log (V2 feature F13).
 *
 * Spec: the V2 master prompt, F13.  One line per study session, in a plain
 * text file the student can read and edit:
 *
 *      2026-10-31 25 pacer
 *
 * date, minutes, tool.  A broken line is skipped rather than counted wrong, so
 * hand-editing the file cannot corrupt the totals.
 *
 * The day arithmetic is civil-from-days, the same formula the rest of the
 * world uses, so a streak across a month boundary is not a special case.
 *
 * No gint, no libm.
 */

#ifndef BENCH_LOGBOOK_H
#define BENCH_LOGBOOK_H

#include <stdbool.h>

#define LOG_MAX 400
#define LOG_TOOL_MAX 12
#define LOG_LINE_MAX 32

typedef struct {
    int year, month, day;
    int minutes;
    char tool[LOG_TOOL_MAX + 1];
} log_entry_t;

/* Days since 2000-01-01, or -1 for a date that does not exist. */
int log_day_number(int year, int month, int day);

/* "2026-10-31 25 pacer" -> entry.  False on a line that is not that shape, a
 * date that does not exist, minutes outside 0..1440 or a tool name that is
 * empty or too long. */
bool log_parse(char const *line, log_entry_t *out);
bool log_format(log_entry_t const *e, char *buf, int size);

/* Whole file: parses what it can into `out` (at most `max`) and returns the
 * number of entries kept.  `skipped` may be NULL. */
int log_parse_all(char const *text, log_entry_t *out, int max, int *skipped);

int log_total(log_entry_t const *e, int n);
int log_day_total(log_entry_t const *e, int n, int day_num);
int log_tool_total(log_entry_t const *e, int n, char const *tool);
int log_days_logged(log_entry_t const *e, int n);

/* The longest run of consecutive days with at least one session. */
int log_longest_streak(log_entry_t const *e, int n);

/* The entry with the largest, or smallest, day number.  False when empty. */
bool log_latest(log_entry_t const *e, int n, int *day_num);

#endif /* BENCH_LOGBOOK_H */
