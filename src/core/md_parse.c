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

    /* Display math: a line that is exactly "$$" (spaces allowed).  Blocks are
     * organised by md_parse_blocks_from(), which pairs the two lines. */
    if (len >= 2 && line[0] == '$' && line[1] == '$') {
        int p = 2;
        while (p < len && is_space(line[p]))
            p++;
        if (p == len) {
            k.type = MD_BLOCK_MATH;
            k.content_start = k.content_end = 0;
            return k;
        }
    }

    /* Fenced code: ``` (any language tag after it). */
    if (len >= 3 && line[0] == '`' && line[1] == '`' && line[2] == '`') {
        k.type = MD_BLOCK_CODE;
        return k;
    }

    /* A table row: already padded for the screen by md_prepare(), so it is its
     * own block and is never wrapped or merged into a paragraph. */
    if (line[0] == '|') {
        k.type = MD_BLOCK_TABLE;
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

        if (k.type == MD_BLOCK_MATH) {
            /* "$" opens: the content is everything up to the closing line.  A
             * missing closer ends the block at the end of the note, so a broken
             * formula costs one block rather than the rest of the page. */
            int content_start = next, content_end = next;
            int open_line = line_no, lines = 0;

            pos = next;
            line_no++;
            while (pos < len) {
                int n2, e2 = line_bounds(src, len, pos, &n2);
                int p = pos;
                while (p < e2 && is_space(src[p]))
                    p++;
                if (e2 - p >= 2 && src[p] == '$' && src[p + 1] == '$') {
                    int q = p + 2;
                    while (q < e2 && is_space(src[q]))
                        q++;
                    pos = n2;
                    line_no++;
                    if (q == e2)
                        break;
                    continue;
                }
                if (lines == 0)
                    content_start = pos;
                content_end = e2;
                lines++;
                pos = n2;
                line_no++;
            }
            if (content_end < content_start) {
                content_end = content_start;
                lines = 0;
            }
            if (count < max) {
                md_block_t *b = &blocks[count];
                memset(b, 0, sizeof *b);
                b->type = MD_BLOCK_MATH;
                b->first_line = (uint16_t)open_line;
                b->line_count = (uint16_t)lines;
                b->offset = (uint32_t)content_start;
                b->length = (uint32_t)(content_end - content_start);
                b->next_offset = (uint32_t)pos;
            }
            count++;
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

        if (c == '$') {
            /* Inline math: $...$ on one line.  A "$$" inside is not a math
             * span; leaving it as text keeps display lines readable. */
            int close = find_marker(src, i + 1, end, '$');
            if (close > i + 1 && close + 1 < end && src[close + 1] == '$') {
                /* "$$" inside inline text: fall through to text. */
                i += 2;
                continue;
            }
            if (close > i + 1 && !(close == i + 1)) {
                emit(&sink, text_start, i - text_start, MD_SPAN_TEXT);
                emit(&sink, i + 1, close - (i + 1), MD_SPAN_MATH);
                i = close + 1;
                text_start = i;
                continue;
            }
            i++;
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

/* ---- tables ------------------------------------------------------------- */

/* Trim one cell: spaces only, so a "|   |" cell is empty but a cell holding a
 * single space survives as a space. */
static void cell_bounds(char const *line, int *start, int *end)
{
    while (*start < *end && is_space(line[*start]))
        (*start)++;
    while (*end > *start && is_space(line[*end - 1]))
        (*end)--;
    if (*end <= *start)
        *end = *start;
}

int md_parse_table_row(char const *line, int len, md_cell_t *cells, int max)
{
    int pos = 0, count = 0;

    if (!line || len <= 0)
        return 0;

    /* Optional leading pipe. */
    while (pos < len && is_space(line[pos]))
        pos++;
    if (pos < len && line[pos] == '|')
        pos++;

    for (;;) {
        int start = pos, end;
        while (pos < len && line[pos] != '|')
            pos++;
        end = pos;
        cell_bounds(line, &start, &end);
        if (count < max) {
            cells[count].offset = (uint32_t)start;
            cells[count].length = (uint16_t)(end - start);
            cells[count].pad = 0;
        }
        count++;
        if (pos >= len)
            break;
        pos++;   /* step over the separator pipe */
        /* A trailing pipe ends the row; do not invent a cell after it. */
        {
            int t = pos;
            while (t < len && is_space(line[t]))
                t++;
            if (t >= len)
                break;
        }
    }
    return count;
}

/* Classifies a separator row: dashes with optional colons.  Returns the
 * alignment of the cell, or -1 when the cell is not a separator. */
static int separator_align(char const *line, md_cell_t cell)
{
    int i = (int)cell.offset;
    int end = i + (int)cell.length;
    int left = 0, right = 0, dashes = 0;

    if (cell.length == 0)
        return -1;
    if (line[i] == ':') {
        left = 1;
        i++;
    }
    if (end > i && line[end - 1] == ':') {
        right = 1;
        end--;
    }
    for (; i < end; i++) {
        if (line[i] == '-')
            dashes++;
        else if (!is_space(line[i]))
            return -1;
    }
    if (dashes < 1)
        return -1;
    if (left && right)
        return MD_ALIGN_CENTER;
    if (right)
        return MD_ALIGN_RIGHT;
    return MD_ALIGN_LEFT;
}

int md_parse_table(char const *src, int len, md_table_t *out)
{
    int pos = 0;

    if (!src || !out || len <= 0)
        return 0;

    /* Scan for a header row that has a real separator row under it: a note may
     * open with prose, and a pipe line on its own is not a table. */
    while (pos < len) {
        md_cell_t head[MD_MAX_COLS], sep[MD_MAX_COLS];
        md_table_t t;
        int next, end = line_bounds(src, len, pos, &next);
        int cols, rows = 0;

        cols = md_parse_table_row(src + pos, end - pos, head, MD_MAX_COLS);
        if (cols == 0 || cols > MD_MAX_COLS || next >= len) {
            pos = next;
            continue;
        }

        {
            int n2, e2 = line_bounds(src, len, next, &n2);
            int n = md_parse_table_row(src + next, e2 - next, sep, MD_MAX_COLS);

            if (n != cols) {
                pos = next;
                continue;
            }
            memset(&t, 0, sizeof t);
            t.cols = (uint8_t)cols;
            t.header_offset = (uint32_t)pos;
            t.body_offset = (uint32_t)n2;
            {
                bool ok = true;
                for (int c = 0; c < cols; c++) {
                    int a = separator_align(src + next, sep[c]);
                    if (a < 0) {
                        ok = false;
                        break;
                    }
                    t.align[c] = (uint8_t)a;
                }
                if (!ok) {
                    pos = next;
                    continue;
                }
            }

            /* Data rows run until a blank line or the end of the note. */
            pos = n2;
            while (pos < len) {
                int n3, e3 = line_bounds(src, len, pos, &n3);
                if (md_line_is_blank(src + pos, e3 - pos))
                    break;
                if (md_parse_table_row(src + pos, e3 - pos, head, 1) == 0)
                    break;
                rows++;
                pos = n3;
            }
            if (rows == 0)
                return 0;
            *out = t;
            return rows;
        }
    }
    return 0;
}

/* ---- LaTeX -------------------------------------------------------------- */

/* One control word and the character it draws.  The font is ASCII-only in the
 * shipped build (see docs/spike.md), so these are the characters a reader has
 * to be able to show; scr_has_glyph() decides at draw time and falls back to
 * the ASCII spelling below when the glyph is missing. */
typedef struct {
    char const *cmd;
    char const *utf8;
    char const *ascii;   /* used when the font has no glyph for utf8 */
} tex_sym_t;

static tex_sym_t const TEX_SYMBOLS[] = {
    { "alpha", "\xce\xb1", "alpha" },        { "beta", "\xce\xb2", "b" },
    { "gamma", "\xce\xb3", "gamma" },        { "delta", "\xce\xb4", "d" },
    { "epsilon", "\xce\xb5", "epsilon" },      { "theta", "\xce\xb8", "0" },
    { "lambda", "\xce\xbb", "lambda" },       { "mu", "\xce\xbc", "u" },
    { "pi", "\xcf\x80", "pi" },          { "rho", "\xcf\x81", "p" },
    { "sigma", "\xcf\x83", "sigma" },        { "tau", "\xcf\x84", "t" },
    { "phi", "\xcf\x86", "phi" },         { "omega", "\xcf\x89", "w" },
    { "Delta", "\xce\x94", "Delta" },        { "Sigma", "\xce\xa3", "S" },
    { "Omega", "\xce\xa9", "Omega" },        { "times", "\xc3\x97", "x" },
    { "cdot", "\xc2\xb7", "*" },         { "div", "\xc3\xb7", "/" },
    { "pm", "\xc2\xb1", "+-" },          { "mp", "\xe2\x88\x93", "-+" },
    { "le", "\xe2\x89\xa4", "<=" },      { "leq", "\xe2\x89\xa4", "<=" },
    { "ge", "\xe2\x89\xa5", ">=" },      { "geq", "\xe2\x89\xa5", ">=" },
    { "ne", "\xe2\x89\xa0", "!=" },      { "neq", "\xe2\x89\xa0", "!=" },
    { "approx", "\xe2\x89\x88", "~=" },  { "propto", "\xe2\x88\x9d", "prop" },
    { "infty", "\xe2\x88\x9e", "inf" },  { "sum", "\xe2\x88\x91", "sum" },
    { "prod", "\xe2\x88\x8f", "prod" },  { "int", "\xe2\x88\xab", "int" },
    { "partial", "\xe2\x88\x82", "d" },  { "nabla", "\xe2\x88\x87", "grad" },
    { "to", "\xe2\x86\x92", "->" },      { "rightarrow", "\xe2\x86\x92", "->" },
    { "leftarrow", "\xe2\x86\x90", "<-" }, { "Rightarrow", "\xe2\x87\x92", "=>" },
    { "iff", "\xe2\x87\x94", "<=>" },    { "in", "\xe2\x88\x88", "in" },
    { "cap", "\xe2\x88\xa9", "cap" },    { "cup", "\xe2\x88\xaa", "cup" },
    { "subset", "\xe2\x8a\x82", "sub" }, { "emptyset", "\xe2\x88\x85", "{}" },
    { "degree", "\xc2\xb0", " deg" },    { "circ", "\xe2\x88\x98", "o" },
    { "angle", "\xe2\x88\xa0", "angle" },{ "perp", "\xe2\x8a\xa5", "_|_" },
    { "parallel", "\xe2\x88\xa5", "||" },{ "therefore", "\xe2\x88\xb4", "so" },
    { "dots", "\xe2\x80\xa6", "..." },   { "ldots", "\xe2\x80\xa6", "..." },
    { "sqrt", "\xe2\x88\x9a", "sqrt" },  { "pi2", "\xcf\x80", "pi" },
};

/* Escape hatch for the renderer: the ASCII spelling of a symbol. */
static tex_sym_t const *tex_lookup(char const *name, int len)
{
    for (unsigned i = 0; i < sizeof TEX_SYMBOLS / sizeof TEX_SYMBOLS[0]; i++) {
        char const *cmd = TEX_SYMBOLS[i].cmd;
        int n = 0;
        while (cmd[n])
            n++;
        if (n == len && strncmp(cmd, name, (size_t)len) == 0)
            return &TEX_SYMBOLS[i];
    }
    return 0;
}

/* Superscript and subscript tables: the characters a bitmap font can raise or
 * lower without changing the line box. */
static char const *sup_of(char c)
{
    switch (c) {
    case '0': return "\xe2\x81\xb0";
    case '1': return "\xc2\xb9";
    case '2': return "\xc2\xb2";
    case '3': return "\xc2\xb3";
    case '4': return "\xe2\x81\xb4";
    case '5': return "\xe2\x81\xb5";
    case '6': return "\xe2\x81\xb6";
    case '7': return "\xe2\x81\xb7";
    case '8': return "\xe2\x81\xb8";
    case '9': return "\xe2\x81\xb9";
    case '+': return "\xe2\x81\xba";
    case '-': return "\xe2\x81\xbb";
    case '=': return "\xe2\x81\xbc";
    case '(': return "\xe2\x81\xbd";
    case ')': return "\xe2\x81\xbe";
    default: return 0;
    }
}

static char const *sub_of(char c)
{
    switch (c) {
    case '0': return "\xe2\x82\x80";
    case '1': return "\xe2\x82\x81";
    case '2': return "\xe2\x82\x82";
    case '3': return "\xe2\x82\x83";
    case '4': return "\xe2\x82\x84";
    case '5': return "\xe2\x82\x85";
    case '6': return "\xe2\x82\x86";
    case '7': return "\xe2\x82\x87";
    case '8': return "\xe2\x82\x88";
    case '9': return "\xe2\x82\x89";
    case '+': return "\xe2\x82\x8a";
    case '-': return "\xe2\x82\x8b";
    default: return 0;
    }
}

typedef struct {
    char *out;
    int size;
    int len;
    bool full;
    bool ascii;     /* write ASCII spellings: the shipped font has no Greek */
} tex_out_t;

static void put_str(tex_out_t *o, char const *s)
{
    for (int i = 0; s[i]; i++) {
        if (o->len < o->size - 1)
            o->out[o->len++] = s[i];
        else
            o->full = true;
    }
}

static void put_char(tex_out_t *o, char c)
{
    if (o->len < o->size - 1)
        o->out[o->len++] = c;
    else
        o->full = true;
}

/* Index of the brace that closes the group opened at src[open].  -1 when it is
 * missing. */
static int matching_brace(char const *src, int len, int open)
{
    int depth = 0;

    for (int i = open; i < len; i++) {
        if (src[i] == '{')
            depth++;
        else if (src[i] == '}' && --depth == 0)
            return i;
    }
    return -1;
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int latex_run(char const *src, int len, char *out, int outsz, bool ascii)
{
    tex_out_t o;

    if (!src || !out || outsz <= 0 || len < 0)
        return -1;

    o.out = out;
    o.size = outsz;
    o.len = 0;
    o.full = false;
    o.ascii = ascii;

    for (int i = 0; i < len; i++) {
        char c = src[i];

        if (c == '\\' && i + 1 < len) {
            char n = src[i + 1];

            if (!is_alpha(n)) {
                /* Escaped punctuation is the character itself; spacing
                 * commands (\, \; \! \ ) vanish. */
                i++;
                if (n == ',' || n == ';' || n == ':' || n == '!' || n == ' ')
                    continue;
                if (n == '\\')
                    continue;               /* line break: handled upstream */
                put_char(&o, n);
                continue;
            }

            /* Control word: read the name. */
            {
                int start = i + 1, e = start;
                while (e < len && is_alpha(src[e]))
                    e++;
                int name_len = e - start;
                char const *name = src + start;

                i = e - 1;
                if (name_len == 4 && strncmp(name, "frac", 4) == 0) {
                    /* \frac{a}{b} -> a/b, evaluated as a plain ratio. */
                    if (i + 1 < len && src[i + 1] == '{') {
                        int close_a = matching_brace(src, len, i + 1);
                        if (close_a > 0) {
                            int a_start = i + 2, a_len = close_a - a_start;
                            int n2 = close_a + 1;
                            while (n2 < len && is_space(src[n2]))
                                n2++;
                            if (n2 < len && src[n2] == '{') {
                                int close_b = matching_brace(src, len, n2);
                                if (close_b > 0) {
                                    char num[32], den[32];
                                    int nlen = latex_run(src + a_start, a_len,
                                                         num, (int)sizeof num,
                                                         o.ascii);
                                    int dlen = latex_run(
                                        src + n2 + 1, close_b - (n2 + 1), den,
                                        (int)sizeof den, o.ascii);
                                    if (nlen < 0)
                                        nlen = 0;
                                    if (dlen < 0)
                                        dlen = 0;
                                    put_str(&o, num);
                                    put_char(&o, '/');
                                    put_str(&o, den);
                                    i = close_b;
                                    continue;
                                }
                            }
                        }
                    }
                    put_str(&o, "frac");
                    continue;
                }
                if (name_len == 4 && strncmp(name, "text", 4) == 0) {
                    if (i + 1 < len && src[i + 1] == '{') {
                        int close = matching_brace(src, len, i + 1);
                        if (close > 0) {
                            for (int k = i + 2; k < close; k++)
                                put_char(&o, src[k]);
                            i = close;
                            continue;
                        }
                    }
                    continue;
                }
                if (name_len == 4 && strncmp(name, "sqrt", 4) == 0) {
                    /* \sqrt{x}, \sqrt[3]{x}: a ranked font cannot draw a root
                     * sign with a hook, so the index travels with the word. */
                    int k = i + 1;
                    int idx_start = -1, idx_end = -1;

                    if (k < len && src[k] == '[') {
                        int close = k + 1;

                        while (close < len && src[close] != ']')
                            close++;
                        if (close < len) {
                            idx_start = k + 1;
                            idx_end = close;
                            k = close + 1;
                        }
                    }
                    if (o.ascii) {
                        /* "sqrt[3]8": unambiguous without a raised digit. */
                        put_str(&o, "sqrt");
                        if (idx_start >= 0) {
                            put_char(&o, '[');
                            for (int q = idx_start; q < idx_end; q++)
                                put_char(&o, src[q]);
                            put_char(&o, ']');
                        }
                    } else {
                        if (idx_start >= 0) {
                            if (idx_end - idx_start == 1) {
                                char const *sup = sup_of(src[idx_start]);

                                if (sup)
                                    put_str(&o, sup);
                                else
                                    put_char(&o, src[idx_start]);
                            } else {
                                for (int q = idx_start; q < idx_end; q++)
                                    put_char(&o, src[q]);
                            }
                        }
                        put_str(&o, "\xe2\x88\x9a");
                    }
                    if (k < len && src[k] == '{') {
                        int close = matching_brace(src, len, k);

                        if (close > 0) {
                            int w = latex_run(src + k + 1, close - (k + 1),
                                              o.out + o.len, o.size - o.len,
                                              o.ascii);

                            if (w > 0)
                                o.len += w;
                            i = close;
                            continue;
                        }
                    }
                    continue;
                }
                if ((name_len == 4 && strncmp(name, "left", 4) == 0) ||
                    (name_len == 5 && strncmp(name, "right", 5) == 0)) {
                    /* \left( \right) only size the delimiters; keep the
                     * delimiter, drop the command. */
                    continue;
                }

                tex_sym_t const *sym = tex_lookup(name, name_len);
                if (sym)
                    put_str(&o, o.ascii ? sym->ascii : sym->utf8);
                else
                    /* Unknown command: keep the name so nothing is lost. */
                    for (int k = 0; k < name_len; k++)
                        put_char(&o, name[k]);
                continue;
            }
        }

        if ((c == '^' || c == '_') && i + 1 < len) {
            char const *(*map)(char) = c == '^' ? sup_of : sub_of;
            int start, stop;
            char const *one[16];
            int count = 0;
            bool all = true;

            if (src[i + 1] == '{') {
                int close = matching_brace(src, len, i + 1);
                if (close < 0) {
                    put_char(&o, c);
                    continue;
                }
                start = i + 2;
                stop = close;
                i = close;
            } else {
                start = i + 1;
                stop = i + 2;
                i = i + 1;
            }
            for (int k = start; k < stop; k++) {
                char const *m = map(src[k]);
                if (count < 16)
                    one[count] = m;
                if (!m)
                    all = false;
                count++;
            }
            if (o.ascii) {
                /* ASCII has no raised or lowered forms: keep ^ and _ so the
                 * expression still reads as an exponent or an index. */
                put_char(&o, c);
                for (int k = start; k < stop; k++)
                    put_char(&o, src[k]);
            } else if (all) {
                for (int k = 0; k < count && k < 16; k++)
                    put_str(&o, one[k]);
            } else {
                /* Not every character has a raised form: keep the caret so the
                 * reader still knows it is an exponent. */
                put_char(&o, c);
                for (int k = start; k < stop; k++)
                    put_char(&o, src[k]);
            }
            continue;
        }

        if (c == '{' || c == '}')
            continue;                        /* grouping only */
        put_char(&o, c);
    }

    out[o.len] = '\0';
    return o.len;
}

int md_latex_to_text(char const *src, int len, char *out, int outsz)
{
    return latex_run(src, len, out, outsz, false);
}

int md_latex_to_ascii(char const *src, int len, char *out, int outsz)
{
    return latex_run(src, len, out, outsz, true);
}

/* ---- preparing a note for the viewer ------------------------------------ */

typedef struct {
    char *out;
    int size;
    int len;
    bool full;
} prep_t;

static void prep_str(prep_t *p, char const *s)
{
    for (int i = 0; s[i]; i++) {
        if (p->len < p->size - 1)
            p->out[p->len++] = s[i];
        else
            p->full = true;
    }
}

static void prep_char(prep_t *p, char c)
{
    if (p->len < p->size - 1)
        p->out[p->len++] = c;
    else
        p->full = true;
}

static void prep_pad(prep_t *p, int n)
{
    while (n-- > 0)
        prep_char(p, ' ');
}

/* Copy one logical line, converting $...$ math to its ASCII spelling. */
static void prep_line(prep_t *p, char const *line, int len)
{
    int i = 0, text_start = 0;

    while (i < len) {
        if (line[i] == '$' && (i == 0 || line[i - 1] != '\\')) {
            int close = i + 1;

            while (close < len && line[close] != '$')
                close++;
            if (close < len && close > i + 1) {
                char conv[160];
                int n;

                for (int k = text_start; k < i; k++)
                    prep_char(p, line[k]);
                n = md_latex_to_ascii(line + i + 1, close - (i + 1), conv,
                                      (int)sizeof conv);
                if (n > 0)
                    prep_str(p, conv);
                i = close + 1;
                text_start = i;
                continue;
            }
        }
        i++;
    }
    for (int k = text_start; k < len; k++)
        prep_char(p, line[k]);
}

/* Width of a column, capped so a long cell cannot push the table off-screen. */
#define MD_TABLE_CELL_MAX 30

static int cell_width(char const *line, int len, int col)
{
    md_cell_t cells[MD_MAX_COLS];
    int n = md_parse_table_row(line, len, cells, MD_MAX_COLS);

    if (col >= n)
        return 0;
    return cells[col].length > MD_TABLE_CELL_MAX ? MD_TABLE_CELL_MAX
                                                 : (int)cells[col].length;
}

/* Copy one line of display math: the whole line is LaTeX, so it is converted
 * rather than searched for $...$ spans. */
static void prep_tex_line(prep_t *p, char const *line, int len)
{
    char conv[256];
    int n = md_latex_to_ascii(line, len, conv, (int)sizeof conv);

    if (n > 0)
        prep_str(p, conv);
}

/* Write one padded row: "| a   |  12 |", padded per column and per its
 * alignment from the separator row. */
static void prep_row(prep_t *p, char const *line, int len, int const *width,
                     uint8_t const *align, int cols)
{
    md_cell_t cells[MD_MAX_COLS];
    int n = md_parse_table_row(line, len, cells, MD_MAX_COLS);

    for (int c = 0; c < cols; c++) {
        int const w = width[c];
        int const have = c < n ? (int)cells[c].length : 0;
        int const written = have > w ? w : have;
        int const pad = w - written;
        int const before = align[c] == MD_ALIGN_RIGHT ? pad
                           : align[c] == MD_ALIGN_CENTER ? pad / 2
                                                          : 0;

        prep_str(p, "| ");
        prep_pad(p, before);
        for (int k = 0; k < written; k++)
            prep_char(p, line[cells[c].offset + k]);
        prep_pad(p, pad - before);
        prep_str(p, " ");
    }
    prep_str(p, "|\n");
}

int md_prepare(char const *src, int len, char *out, int outsz, int cols)
{
    prep_t p;
    int pos = 0;

    if (!src || !out || outsz <= 0 || len < 0)
        return -1;
    p.out = out;
    p.size = outsz;
    p.len = 0;
    p.full = false;

    while (pos < len) {
        int next, end = line_bounds(src, len, pos, &next);
        md_table_t table;

        /* A "$$" line opens display math; every line up to the closing "$$" is
         * converted, and the markers are kept so the block still reads as
         * display math in the viewer. */
        {
            int q = pos;

            while (q < end && is_space(src[q]))
                q++;
            if (end - q >= 2 && src[q] == '$' && src[q + 1] == '$' &&
                q + 2 == end) {
                prep_tex_line(&p, src + pos, end - pos);
                prep_char(&p, '\n');
                pos = next;
                while (pos < len) {
                    int n2, e2 = line_bounds(src, len, pos, &n2);
                    int r = pos;

                    while (r < e2 && is_space(src[r]))
                        r++;
                    if (e2 - r >= 2 && src[r] == '$' && src[r + 1] == '$' &&
                        r + 2 == e2) {
                        prep_tex_line(&p, src + pos, e2 - pos);
                        prep_char(&p, '\n');
                        pos = n2;
                        break;
                    }
                    prep_tex_line(&p, src + pos, e2 - pos);
                    prep_char(&p, '\n');
                    pos = n2;
                }
                continue;
            }
        }

        /* A pipe table at this line: measure the columns, then write an
         * aligned header, a rule, and the data rows. */
        if (md_parse_table(src + pos, len - pos, &table) > 0 &&
            table.header_offset == 0) {
            int width[MD_MAX_COLS] = { 0 };
            int r = pos + (int)table.body_offset;
            int stop = r;

            for (int c = 0; c < table.cols; c++)
                width[c] = cell_width(src + pos, end - pos, c);
            while (r < len) {
                int n4, e4 = line_bounds(src, len, r, &n4);
                int cells;

                if (md_line_is_blank(src + r, e4 - r))
                    break;
                /* A row, not just any line: it needs a pipe and the same number
                 * of columns as the header, or the paragraph after the table
                 * would be swallowed into it. */
                cells = md_parse_table_row(src + r, e4 - r, 0, 0);
                if (cells != table.cols || !memchr(src + r, '|', (size_t)(e4 - r)))
                    break;
                for (int c = 0; c < table.cols; c++) {
                    int w = cell_width(src + r, e4 - r, c);

                    if (w > width[c])
                        width[c] = w;
                }
                stop = n4;
                r = n4;
            }

            /* Shrink until the row fits: "| " + cell + " " per column, plus the
             * closing bar.  Table cells cannot wrap, so a row wider than the
             * screen would simply be clipped. */
            if (cols > 8) {
                for (;;) {
                    /* Per column "| " + cell + " ", then the closing "|". */
                    int total = 1;
                    int widest = 0, widest_at = 0;

                    for (int c = 0; c < table.cols; c++) {
                        total += width[c] + 3;
                        if (width[c] > widest) {
                            widest = width[c];
                            widest_at = c;
                        }
                    }
                    if (total <= cols || widest <= 3)
                        break;
                    width[widest_at] = widest - 1;
                }
            }

            /* Header, then a rule of the same length: "| ----- | ---- |". */
            prep_row(&p, src + pos, end - pos, width, table.align, table.cols);
            for (int c = 0; c < table.cols; c++) {
                prep_str(&p, "| ");
                for (int k = 0; k < width[c]; k++)
                    prep_char(&p, '-');
                prep_str(&p, " ");
            }
            prep_str(&p, "|\n");

            r = pos + (int)table.body_offset;
            while (r < stop) {
                int n4, e4 = line_bounds(src, len, r, &n4);

                prep_row(&p, src + r, e4 - r, width, table.align, table.cols);
                r = n4;
            }
            pos = stop;
            continue;
        }

        prep_line(&p, src + pos, end - pos);
        if (next > end)
            prep_char(&p, '\n');
        pos = next;
    }

    out[p.len] = '\0';
    return p.len;
}
