/* formula.c - see formula.h. */

#include "formula.h"

#include <stddef.h>

/* Offset just past the newline that ends `index`. */
static int line_limit(char const *text, int len, int index)
{
    int i = formula_line_start(text, len, index);

    while (i < len && text[i] != '\n')
        i++;
    return i < len ? i + 1 : len;
}

int formula_line_count(char const *text, int len)
{
    int n = 0;
    int i = 0;

    if (!text || len <= 0)
        return 0;
    while (i < len)
    {
        n++;
        while (i < len && text[i] != '\n')
            i++;
        if (i < len)
            i++;            /* the newline itself */
    }
    return n;
}

int formula_line_start(char const *text, int len, int index)
{
    int i = 0;

    if (!text || len <= 0 || index <= 0)
        return 0;
    for (int n = 0; n < index && i < len; n++)
    {
        while (i < len && text[i] != '\n')
            i++;
        if (i < len)
            i++;
    }
    return i > len ? len : i;
}

int formula_line_end(char const *text, int len, int index)
{
    int i = formula_line_start(text, len, index);

    if (!text || len <= 0 || i >= len)
        return len;
    while (i < len && text[i] != '\n')
        i++;
    return i;
}

int formula_line_of_offset(char const *text, int len, int off)
{
    int line = 0;
    int i = 0;

    if (!text || len <= 0 || off < 0 || off >= len)
        return -1;
    while (i < len)
    {
        int const limit = line_limit(text, len, line);

        if (off < limit)
            return line;
        i = limit;
        line++;
    }
    return line;
}

int formula_heading_level(char const *text, int len, int line)
{
    int i = formula_line_start(text, len, line);
    int level = 0;

    if (!text || len <= 0 || i >= len)
        return 0;
    while (i < len && text[i] == '#')
    {
        level++;
        i++;
    }
    if (level < 1 || level > 3)
        return 0;
    if (i >= len || text[i] != ' ')
        return 0;
    return level;
}

int formula_next_heading(char const *text, int len, int from, int depth)
{
    for (int i = from; i < formula_line_count(text, len); i++)
    {
        int const level = formula_heading_level(text, len, i);

        if (level > 0 && level <= depth)
            return i;
    }
    return from;
}

int formula_prev_heading(char const *text, int len, int from, int depth)
{
    for (int i = from - 1; i >= 0; i--)
    {
        int const level = formula_heading_level(text, len, i);

        if (level > 0 && level <= depth)
            return i;
    }
    return from;
}
