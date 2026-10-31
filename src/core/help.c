/* help.c - see help.h.  No gint, no display: every rule here is testable. */

#include "help.h"

#include <stddef.h>

int help_line_count(char const *text)
{
    int n = 1;

    if (!text)
        return 0;
    for (; *text; text++)
        if (*text == '\n')
            n++;
    return n;
}

/* Start of line `index`, or NULL past the end. */
static char const *line_start(char const *text, int index)
{
    if (!text || index < 0)
        return NULL;
    while (index > 0)
    {
        while (*text && *text != '\n')
            text++;
        if (!*text)
            return NULL;
        text++;
        index--;
    }
    return text;
}

int help_line(char const *text, int index, char *out, int size)
{
    char const *s = line_start(text, index);
    int n = 0;

    if (!out || size <= 0)
        return 0;
    out[0] = 0;
    if (!s)
        return 0;
    while (s[n] && s[n] != '\n' && n < size - 1)
    {
        out[n] = s[n];
        n++;
    }
    out[n] = 0;
    return n;
}

int help_needs_scroll(char const *text, int fit)
{
    if (fit < 1)
        return help_line_count(text) > 0;
    return help_line_count(text) > fit;
}

int help_scroll_pos(char const *text, int first, int fit, int delta)
{
    int const lines = help_line_count(text);
    int last;
    int pos;

    /* No room for a line: there is nothing to scroll to. */
    if (fit < 1)
        return 0;
    last = lines > fit ? lines - fit : 0;
    pos = first + delta;

    if (pos < 0)
        pos = 0;
    if (pos > last)
        pos = last;
    return pos;
}

int help_skip_title(char const *text, char const *title)
{
    char const *s = text;
    size_t n = 0;

    if (!text || !title || !*title)
        return 0;
    while (title[n])
        n++;
    for (size_t i = 0; i < n; i++)
        if (s[i] != title[i])
            return 0;
    return s[n] == '\n' ? 1 : 0;
}
