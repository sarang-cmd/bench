/* textsearch.c - see textsearch.h. */

#include "textsearch.h"

#include <stddef.h>

char textsearch_lower(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (char)(c - 'A' + 'a');
    return c;
}

static bool fold_equal(char a, char b)
{
    return textsearch_lower(a) == textsearch_lower(b);
}

/* Matches needle at hay[pos..].  Caller guarantees pos + nlen <= hlen. */
static bool match_at(char const *hay, char const *needle, int nlen)
{
    int i;
    for (i = 0; i < nlen; i++)
        if (!fold_equal(hay[i], needle[i]))
            return false;
    return true;
}

int textsearch_find(char const *hay, int hlen, int from, char const *needle,
                    int nlen)
{
    int i;

    if (!hay || !needle || hlen <= 0 || nlen <= 0)
        return -1;
    if (from < 0)
        from = 0;
    if (from > hlen - nlen)
        return -1;

    for (i = from; i + nlen <= hlen; i++)
        if (match_at(hay + i, needle, nlen))
            return i;
    return -1;
}

int textsearch_find_back(char const *hay, int hlen, int before,
                         char const *needle, int nlen)
{
    int i;

    if (!hay || !needle || hlen <= 0 || nlen <= 0)
        return -1;
    if (before > hlen)
        before = hlen;

    for (i = before - nlen; i >= 0; i--)
        if (match_at(hay + i, needle, nlen))
            return i;
    return -1;
}

int textsearch_count(char const *hay, int hlen, char const *needle, int nlen)
{
    int count = 0, pos = 0, found;

    if (!hay || !needle || hlen <= 0 || nlen <= 0)
        return 0;

    while ((found = textsearch_find(hay, hlen, pos, needle, nlen)) >= 0) {
        count++;
        pos = found + nlen;
        if (pos >= hlen)
            break;
    }
    return count;
}

int textsearch_find_wrap(char const *hay, int hlen, int from,
                         char const *needle, int nlen)
{
    int found;

    if (!hay || !needle || hlen <= 0 || nlen <= 0)
        return -1;
    if (from < 0)
        from = 0;
    if (from > hlen)
        from = hlen;

    found = textsearch_find(hay, hlen, from, needle, nlen);
    if (found >= 0)
        return found;
    return textsearch_find(hay, hlen, 0, needle, nlen);
}

/* ---- line helpers ------------------------------------------------------- */

int textsearch_line_count(char const *text, int len)
{
    int count = 0, pos = 0;

    if (!text || len <= 0)
        return 0;

    while (pos < len) {
        count++;
        while (pos < len && text[pos] != '\n')
            pos++;
        if (pos < len)
            pos++; /* step over the newline */
    }
    return count;
}

int textsearch_line_start(char const *text, int len, int index)
{
    int line = 0, pos = 0;

    if (!text || len < 0 || index < 0)
        return len < 0 ? 0 : len;
    if (index == 0)
        return 0;

    while (pos < len) {
        if (text[pos] == '\n') {
            line++;
            if (line == index)
                return pos + 1;
        }
        pos++;
    }
    return len;
}

int textsearch_line_end(char const *text, int len, int index)
{
    int start, end;

    if (!text || len < 0 || index < 0)
        return -1;
    start = textsearch_line_start(text, len, index);
    if (start >= len)
        return -1;

    end = start;
    while (end < len && text[end] != '\n')
        end++;
    if (end > start && text[end - 1] == '\r')
        end--;
    return end;
}

int textsearch_line_of(char const *text, int len, int pos)
{
    int line = 0, i, count;

    if (!text || len <= 0 || pos <= 0)
        return 0;
    if (pos > len)
        pos = len;

    for (i = 0; i < pos && i < len; i++)
        if (text[i] == '\n')
            line++;

    /* Past the final newline there is no line: report the last real one. */
    count = textsearch_line_count(text, len);
    if (count > 0 && line > count - 1)
        line = count - 1;
    return line;
}

bool textsearch_at_line_end(char const *text, int len, int pos)
{
    if (!text || pos < 0 || pos >= len)
        return true;
    return text[pos] == '\n';
}

int textsearch_words(char const *text, int len)
{
    int words = 0;
    bool in_word = false;

    for (int i = 0; i < len; i++) {
        char const c = text[i];
        bool const space = (c == ' ' || c == '\t' || c == '\n' || c == '\r');

        if (space)
            in_word = false;
        else if (!in_word) {
            in_word = true;
            words++;
        }
    }
    return words;
}
