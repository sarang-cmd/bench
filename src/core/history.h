/* history.h - the last few results, so a number does not vanish on EXIT.
 *
 * V2 workstream A5.  A tool computes something, the student looks away, and the
 * answer is gone.  This keeps the last few answers as formatted lines - strings,
 * not numbers, because a result like "x = 1.414214 (4 steps)" is what was on the
 * screen and what should come back.
 *
 * Newest first: index 0 is the most recent entry.  Full means the oldest is
 * dropped.
 *
 * Plain C11, no gint, no allocation. */

#ifndef BENCH_HISTORY_H
#define BENCH_HISTORY_H

#include <stdbool.h>

#define HISTORY_MAX 8
#define HISTORY_LINE_MAX 40

typedef struct {
    char line[HISTORY_MAX][HISTORY_LINE_MAX + 1];
    int count;
} history_t;

void history_clear(history_t *h);

/* Push a line as the newest entry.  False when `line` is NULL, empty, longer
 * than HISTORY_LINE_MAX, or `h` is NULL; nothing is changed in that case. */
bool history_push(history_t *h, char const *line);

/* Number of entries held, 0..HISTORY_MAX. */
int history_count(history_t const *h);

/* The entry at `i`, 0 being the newest, or NULL when out of range. */
char const *history_at(history_t const *h, int i);

#endif /* BENCH_HISTORY_H */
