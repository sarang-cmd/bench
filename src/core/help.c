/* help.c - see help.h.  No gint, no display: every rule here is testable. */

#include "help.h"

#include <stddef.h>
#include <string.h>

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

/* ---- word wrapping ------------------------------------------------------ */

/* Length of logical line `index` without its newline, or -1 past the end. */
static int logical_len(char const *text, int index)
{
    char const *s = line_start(text, index);
    int n = 0;

    if (!s)
        return -1;
    while (s[n] && s[n] != '\n')
        n++;
    return n;
}

/* Start of logical line `index`. */
static char const *logical_at(char const *text, int index)
{
    return line_start(text, index);
}

/* Where the `want`-th segment of one logical line starts, and how long it is.
 * Shared by the counter and the copier so the two cannot disagree. */
static void segment(char const *s, int len, int cols, int want, int *start,
                    int *take)
{
    int pos = 0;

    if (cols < 1)
        cols = 1;
    while (want > 0 && pos < len) {
        int n = len - pos;

        if (n > cols) {
            int cut = cols;

            /* Break at the last space that fits: a wrapped word beats a word
             * cut in half.  A line with no space at all is hard-broken. */
            while (cut > 0 && s[pos + cut] != ' ')
                cut--;
            if (cut <= 0)
                cut = cols;
            n = cut;
        }
        pos += n;
        while (pos < len && s[pos] == ' ')
            pos++;
        want--;
    }
    *start = pos;
    if (pos >= len)
        *take = 0;
    else {
        int n = len - pos;

        if (n > cols) {
            int cut = cols;

            while (cut > 0 && s[pos + cut] != ' ')
                cut--;
            if (cut <= 0)
                cut = cols;
            n = cut;
        }
        *take = n;
    }
}

/* The number of display lines logical line `index` takes at `cols`. */
static int segments_in(char const *s, int len, int cols)
{
    int n = 0, pos = 0;

    if (cols < 1)
        cols = 1;
    if (len <= 0)
        return 1;
    while (pos < len) {
        int take = len - pos;

        if (take > cols) {
            int cut = cols;

            while (cut > 0 && s[pos + cut] != ' ')
                cut--;
            if (cut <= 0)
                cut = cols;
            take = cut;
        }
        pos += take;
        while (pos < len && s[pos] == ' ')
            pos++;
        n++;
    }
    return n;
}

int help_wrap_count(char const *text, int cols)
{
    int lines = help_line_count(text);
    int total = 0;

    if (lines <= 0)
        return 0;
    for (int i = 0; i < lines; i++) {
        char const *s = logical_at(text, i);
        int len = logical_len(text, i);

        total += segments_in(s, len, cols);
    }
    return total;
}

int help_wrap_line(char const *text, int cols, int index, char *out, int size)
{
    int lines = help_line_count(text);
    int seen = 0;

    if (!out || size <= 0)
        return 0;
    out[0] = 0;
    if (cols < 1)
        cols = 1;
    for (int i = 0; i < lines; i++) {
        char const *s = logical_at(text, i);
        int len = logical_len(text, i);
        int segments = segments_in(s, len, cols);

        if (index < seen + segments) {
            int start = 0, take = 0, n;

            segment(s, len, cols, index - seen, &start, &take);
            n = take;
            if (n > size - 1)
                n = size - 1;
            memcpy(out, s + start, (size_t)n);
            out[n] = 0;
            return n;
        }
        seen += segments;
    }
    return 0;
}

int help_wrap_needs_scroll(char const *text, int cols, int fit)
{
    if (fit < 1)
        return help_wrap_count(text, cols) > 0;
    return help_wrap_count(text, cols) > fit;
}

int help_wrap_scroll(char const *text, int cols, int first, int fit, int delta)
{
    int const total = help_wrap_count(text, cols);
    int last;

    if (fit < 1)
        return 0;
    last = total - fit;
    if (last < 0)
        last = 0;
    first += delta;
    if (first > last)
        first = last;
    if (first < 0)
        first = 0;
    return first;
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
