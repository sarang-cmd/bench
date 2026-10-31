/* md_parse.c - see md_parse.h. */

#include "md_parse.h"

#include <string.h>

/* ---- line helpers ------------------------------------------------------- */

static bool is_space(char c)
{
    return c == ' ' || c == '\t';
}

bool md_line_is_blank(char const *line, int len)
{
    int i;
    for (i = 0; i < len; i++)
        if (!is_space(line[i]))
            return false;
    return true;
}

/* Leading indentation in spaces (a tab counts as one indent step). */
static int indent_width(char const *line, int len)
{
    int i = 0, spaces = 0;

    while (i < len && is_space(line[i])) {
        spaces += line[i] == '\t' ? MD_INDENT_SPACES : 1;
        i++;
    }
    return spaces;
}

static int list_level(int spaces)
{
    int level = spaces / MD_INDENT_SPACES;
    if (level < 0)
        level = 0;
    if (level > MD_MAX_LIST_LEVEL)
        level = MD_MAX_LIST_LEVEL;
    return level;
}

/* ---- block classification ----------------------------------------------- */

typedef struct {
    md_block_type_t type;
    uint8_t level;
    uint8_t check;
    uint8_t marker;
    int content_start;  /* offset within the line */
    int content_end;
} line_kind_t;

static bool is_rule(char const *line, int len)
{
    int dashes = 0, i;
    for (i = 0; i < len; i++) {
        if (line[i] == '-')
            dashes++;
        else if (!is_space(line[i]))
            return false;
    }
    return dashes >= 3;
}

static line_kind_t classify(char const *line, int len)
{
    line_kind_t k;
    int i = 0;

    memset(&k, 0, sizeof k);
    k.type = MD_BLOCK_PARAGRAPH;
    k.content_start = 0;
    k.content_end = len;

    if (md_line_is_blank(line, len) || len == 0) {
        k.type = MD_BLOCK_PARAGRAPH;
        k.content_end = 0;
        return k;
    }

    /* Fenced code: ``` (any language tag after it). */
    if (len >= 3 && line[0] == '`' && line[1] == '`' && line[2] == '`') {
        k.type = MD_BLOCK_CODE;
        return k;
    }

    if (is_rule(line, len)) {
        k.type = MD_BLOCK_RULE;
        k.content_start = k.content_end = 0;
        return k;
    }

    /* Headings: 1 to 3 hashes then a space.  A 4th hash is clamped to 3 so it
     * still renders as a heading rather than raw text. */
    if (line[0] == '#') {
        int hashes = 0;
        while (i < len && line[i] == '#')
            i++, hashes++;
        if (i >= len || is_space(line[i])) {
            while (i < len && is_space(line[i]))
                i++;
            k.type = MD_BLOCK_HEADING;
            k.level = (uint8_t)(hashes > MD_MAX_HEADING_LEVEL
                                    ? MD_MAX_HEADING_LEVEL
                                    : hashes);
            if (k.level == 0)
                k.level = 1;
            k.content_start = i;
            return k;
        }
    }

    /* Quotes. */
    if (line[0] == '>') {
        i = 1;
        while (i < len && is_space(line[i]))
            i++;
        k.type = MD_BLOCK_QUOTE;
        k.content_start = i;
        return k;
    }

    /* Lists, after indentation. */
    {
        int spaces = indent_width(line, len);
        int start = 0;

        while (start < len && is_space(line[start]))
            start++;

        if ((line[start] == '-' || line[start] == '*') &&
            (start + 1 >= len || is_space(line[start + 1]))) {
            /* A bare "-" is an empty list item. */
            int p = start + 2;
            if (p > len)
                p = len;
            while (p < len && is_space(line[p]))
                p++;
            k.type = MD_BLOCK_BULLET;
            k.level = (uint8_t)list_level(spaces);
            k.content_start = p;

            /* Check boxes: - [ ] and - [x] */
            if (p + 2 < len && line[p] == '[' && line[p + 2] == ']') {
                char c = line[p + 1];
                if (c == ' ') {
                    k.check = MD_CHECK_TODO;
                    k.content_start = p + 3;
                } else if (c == 'x' || c == 'X') {
                    k.check = MD_CHECK_DONE;
                    k.content_start = p + 3;
                }
                if (k.check != MD_CHECK_NONE) {
                    while (k.content_start < len &&
                           is_space(line[k.content_start]))
                        k.content_start++;
                }
            }
            return k;
        }

        /* Ordered: 1. 2. 10. */
        {
            int digits = 0, value = 0;
            int p = start;
            while (p < len && line[p] >= '0' && line[p] <= '9') {
                if (value < 1000)
                    value = value * 10 + (line[p] - '0');
                digits++;
                p++;
            }
            if (digits > 0 && p + 1 <= len && p < len && line[p] == '.' &&
                (p + 1 >= len || is_space(line[p + 1]))) {
                int q = p + 1;
                while (q < len && is_space(line[q]))
                    q++;
                k.type = MD_BLOCK_ORDERED;
                k.level = (uint8_t)list_level(spaces);
                k.marker = (uint8_t)(value > 255 ? 255 : (value < 1 ? 1
                                                                    : value));
                k.content_start = q;
                return k;
            }
        }
    }

    return k;
}

/* Finds the end of the line starting at `pos` (exclusive, before the newline).
 * Sets *next to the offset after the newline. */
static int line_bounds(char const *src, int len, int pos, int *next)
{
    int end = pos;

    while (end < len && src[end] != '\n')
        end++;
    *next = end < len ? end + 1 : len;
    if (end > pos && src[end - 1] == '\r')
        end--;
    return end;
}

static void trim_range(char const *src, int *start, int *end)
{
    while (*start < *end && is_space(src[*start]))
        (*start)++;
    while (*end > *start && is_space(src[*end - 1]))
        (*end)--;
}

int md_parse_blocks_from(char const *src, int len, int from,
                         md_block_t *blocks, int max)
{
    int pos = from, line_no = 0, count = 0;

    if (!src || len < 0 || from < 0 || from > len)
        return 0;

    while (pos < len) {
        int next = pos, end = line_bounds(src, len, pos, &next);
        line_kind_t k = classify(src + pos, end - pos);

        if (k.type == MD_BLOCK_PARAGRAPH && k.content_end == 0) {
            /* Blank line: ends any paragraph. */
            pos = next;
            line_no++;
            continue;
        }

        if (k.type == MD_BLOCK_CODE) {
            int content_start = next;
            int content_end = next;
            int fence_line = line_no;
            int lines = 0;

            pos = next;
            line_no++;

            while (pos < len) {
                int n2, e2 = line_bounds(src, len, pos, &n2);
                if (e2 - pos >= 3 && src[pos] == '`' && src[pos + 1] == '`' &&
                    src[pos + 2] == '`') {
                    pos = n2;
                    line_no++;
                    break;
                }
                content_end = e2;
                lines++;
                pos = n2;
                line_no++;
            }
            /* Trailing newline before the closing fence is not content. */
            if (content_end < content_start) {
                content_end = content_start;
                lines = 0;
            }

            if (count < max) {
                md_block_t *b = &blocks[count];
                memset(b, 0, sizeof *b);
                b->type = MD_BLOCK_CODE;
                b->first_line = (uint16_t)fence_line;
                b->line_count = (uint16_t)lines;
                b->offset = (uint32_t)content_start;
                b->length = (uint32_t)(content_end - content_start);
                b->next_offset = (uint32_t)pos;
            }
            count++;
            continue;
        }

        {
            int start = pos + k.content_start;
            int stop = pos + k.content_end;
            int first_line = line_no;
            int lines = 1;

            if (k.type == MD_BLOCK_PARAGRAPH) {
                /* A paragraph runs until a blank line or a special line. */
                start = pos;
                stop = end;
                while (next < len) {
                    int n2, e2 = line_bounds(src, len, next, &n2);
                    line_kind_t k2 = classify(src + next, e2 - next);
                    if (k2.type != MD_BLOCK_PARAGRAPH || k2.content_end == 0)
                        break;
                    stop = e2;
                    lines++;
                    next = n2;
                }
                pos = next;
                line_no += lines;
            } else {
                pos = next;
                line_no++;
            }

            trim_range(src, &start, &stop);
            if (count < max) {
                md_block_t *b = &blocks[count];
                memset(b, 0, sizeof *b);
                b->type = (uint8_t)k.type;
                b->level = k.level;
                b->check = k.check;
                b->marker = k.marker;
                b->first_line = (uint16_t)first_line;
                b->line_count = (uint16_t)lines;
                b->offset = (uint32_t)start;
                b->length = (uint32_t)(stop > start ? stop - start : 0);
                b->next_offset = (uint32_t)pos;
            }
            count++;
        }
    }
    return count;
}

/* ---- inline spans ------------------------------------------------------- */

typedef struct {
    md_span_t *spans;
    int max;
    int count;    /* total found, may exceed max */
} span_sink_t;

static void emit(span_sink_t *s, int offset, int length, md_span_style_t style)
{
    if (length <= 0)
        return;
    if (s->count < s->max) {
        s->spans[s->count].offset = (uint32_t)offset;
        s->spans[s->count].length = (uint16_t)(length > 0xFFFF ? 0xFFFF
                                                               : length);
        s->spans[s->count].style = (uint8_t)style;
        s->spans[s->count].pad = 0;
    }
    s->count++;
}

/* Index of the first occurrence of `marker` in src[from..end), or -1. */
static int find_marker(char const *src, int from, int end, char marker)
{
    int i;
    for (i = from; i < end; i++)
        if (src[i] == marker)
            return i;
    return -1;
}

int md_parse_spans(char const *src, int start, int end, md_span_t *spans,
                   int max)
{
    span_sink_t sink;
    int i = start, text_start = start;

    if (!src || start < 0 || end < start)
        return 0;
    sink.spans = spans;
    sink.max = max > 0 ? max : 0;
    sink.count = 0;

    while (i < end) {
        char c = src[i];

        if (c == '\\' && i + 1 < end) {
            /* Escaped character: it is text, without the backslash. */
            emit(&sink, text_start, i - text_start, MD_SPAN_TEXT);
            emit(&sink, i + 1, 1, MD_SPAN_TEXT);
            i += 2;
            text_start = i;
            continue;
        }

        if (c == '`') {
            int close = find_marker(src, i + 1, end, '`');
            if (close > i + 1) {
                emit(&sink, text_start, i - text_start, MD_SPAN_TEXT);
                emit(&sink, i + 1, close - (i + 1), MD_SPAN_CODE);
                i = close + 1;
                text_start = i;
                continue;
            }
            i++;
            continue;
        }

        if (c == '*' && i + 1 < end && src[i + 1] == '*') {
            int close = -1, j;
            for (j = i + 2; j + 1 < end; j++)
                if (src[j] == '*' && src[j + 1] == '*') {
                    close = j;
                    break;
                }
            if (close > i + 2) {
                emit(&sink, text_start, i - text_start, MD_SPAN_TEXT);
                emit(&sink, i + 2, close - (i + 2), MD_SPAN_BOLD);
                i = close + 2;
                text_start = i;
                continue;
            }
            i += 2;
            continue;
        }

        if (c == '*' || c == '_') {
            int close = find_marker(src, i + 1, end, c);
            if (close > i + 1) {
                emit(&sink, text_start, i - text_start, MD_SPAN_TEXT);
                emit(&sink, i + 1, close - (i + 1), MD_SPAN_ITALIC);
                i = close + 1;
                text_start = i;
                continue;
            }
            i++;
            continue;
        }

        if (c == '[') {
            int close_text = find_marker(src, i + 1, end, ']');
            if (close_text > i + 1 && close_text + 1 < end &&
                src[close_text + 1] == '(') {
                int close_url = find_marker(src, close_text + 2, end, ')');
                if (close_url > close_text + 1) {
                    emit(&sink, text_start, i - text_start, MD_SPAN_TEXT);
                    emit(&sink, i + 1, close_text - (i + 1), MD_SPAN_TEXT);
                    i = close_url + 1;
                    text_start = i;
                    continue;
                }
            }
            i++;
            continue;
        }

        i++;
    }

    emit(&sink, text_start, i - text_start, MD_SPAN_TEXT);
    return sink.count;
}

int md_parse_blocks(char const *src, int len, md_block_t *blocks, int max)
{
    return md_parse_blocks_from(src, len, 0, blocks, max);
}
