/* history.c - see history.h. */

#include "history.h"

#include <string.h>

void history_clear(history_t *h)
{
    if (!h)
        return;
    for (int i = 0; i < HISTORY_MAX; i++)
        h->line[i][0] = 0;
    h->count = 0;
}

bool history_push(history_t *h, char const *line)
{
    int n;

    if (!h || !line)
        return false;
    n = (int)strlen(line);
    if (n == 0 || n > HISTORY_LINE_MAX)
        return false;

    /* Shift the older entries down, then write the new one at the top.  Eight
     * short strings, once per solve: not worth a ring buffer. */
    for (int i = HISTORY_MAX - 1; i > 0; i--)
        memcpy(h->line[i], h->line[i - 1], sizeof h->line[0]);
    memcpy(h->line[0], line, (size_t)n + 1);
    if (h->count < HISTORY_MAX)
        h->count++;
    return true;
}

int history_count(history_t const *h)
{
    return h ? h->count : 0;
}

char const *history_at(history_t const *h, int i)
{
    if (!h || i < 0 || i >= h->count)
        return NULL;
    return h->line[i];
}
