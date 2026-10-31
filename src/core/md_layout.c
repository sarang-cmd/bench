/* md_layout.c - see md_layout.h. */

#include "md_layout.h"

#include <string.h>

/* One block's spans.  A block with more inline spans than this is laid out with
 * the first ones styled and the rest as plain text; the deck limit (120 chars a
 * side) is far below this. */
#define MD_BLOCK_SPANS_MAX 256

/* ---- UTF-8 -------------------------------------------------------------- */

uint32_t md_utf8_decode(char const *s, int len, int *i)
{
    unsigned char c;
    uint32_t cp;
    int n, k;

    if (!s || !i || *i < 0 || *i >= len)
        return 0xFFFD;

    c = (unsigned char)s[*i];

    if (c < 0x80) {
        (*i)++;
        return c;
    }
    if ((c & 0xE0) == 0xC0) {
        cp = c & 0x1Fu;
        n = 1;
    } else if ((c & 0xF0) == 0xE0) {
        cp = c & 0x0Fu;
        n = 2;
    } else if ((c & 0xF8) == 0xF0) {
        cp = c & 0x07u;
        n = 3;
    } else {
        (*i)++;
        return 0xFFFD;
    }

    if (*i + n >= len)
        return 0xFFFD; /* truncated sequence */
    for (k = 1; k <= n; k++) {
        unsigned char cc = (unsigned char)s[*i + k];
        if ((cc & 0xC0) != 0x80) {
            (*i)++;
            return 0xFFFD;
        }
        cp = (cp << 6) | (cc & 0x3Fu);
    }
    *i += n + 1;
    return cp;
}

int md_utf8_length(char const *s, int len)
{
    int i = 0, count = 0;

    while (i < len) {
        (void)md_utf8_decode(s, len, &i);
        count++;
    }
    return count;
}

/* ---- helpers ------------------------------------------------------------ */

static int glyph_width(md_metrics_t const *m, uint32_t cp, uint8_t style)
{
    int w;

    if (!m || !m->glyph_width)
        return 1;
    w = m->glyph_width(m->ctx, cp, style);
    return w < 0 ? 0 : w;
}

static void count_glyph(md_layout_stats_t *st, md_metrics_t const *m,
                        uint32_t cp)
{
    if (!st || !m || !m->has_glyph)
        return;
    if (!m->has_glyph(m->ctx, cp))
        st->unsupported++;
}

static uint8_t style_at(md_span_t const *spans, int nspans, int offset)
{
    int i;
    for (i = 0; i < nspans; i++) {
        if ((int)spans[i].offset <= offset &&
            offset < (int)spans[i].offset + (int)spans[i].length)
            return spans[i].style;
    }
    return MD_SPAN_TEXT;
}

static bool is_break_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

/* ---- layout builder ----------------------------------------------------- */

typedef struct {
    md_display_line_t *lines;
    int max_lines;
    md_display_span_t *spans;
    int max_spans;
    int nlines;
    int nspans;
    md_layout_stats_t *stats;
} builder_t;

static void add_span(builder_t *b, int offset, int length, uint8_t style)
{
    if (length <= 0)
        return;
    if (b->nspans < b->max_spans) {
        md_display_span_t *s = &b->spans[b->nspans];
        s->offset = (uint32_t)offset;
        s->length = (uint16_t)(length > 0xFFFF ? 0xFFFF : length);
        s->style = style;
        s->pad = 0;
    } else {
        b->stats->overflow = true;
    }
    b->nspans++;
}

/* Starts a new display line and returns its index, or -1 when out of space. */
static int begin_line(builder_t *b, uint8_t kind, uint8_t level,
                      uint8_t marker, uint16_t indent)
{
    int index = b->nlines;
    b->nlines++;
    if (index >= b->max_lines) {
        b->stats->overflow = true;
        return -1;
    }
    md_display_line_t *l = &b->lines[index];
    l->first_span = (uint16_t)b->nspans;
    l->span_count = 0;
    l->width = 0;
    l->indent = indent;
    l->kind = kind;
    l->level = level;
    l->marker = marker;
    l->pad = 0;
    return index;
}

static void close_line(builder_t *b, int index, int width)
{
    if (index < 0 || index >= b->max_lines)
        return;
    md_display_line_t *l = &b->lines[index];
    int used = b->nspans - (int)l->first_span;
    l->span_count = (uint16_t)(used < 0 ? 0 : used);
    l->width = (uint16_t)(width < 0 ? 0 : width > 0xFFFF ? 0xFFFF : width);
}

/* Adds the spans of src[from..to) to the current line, clipped to the block's
 * span list.  Returns the pixel width of the text added. */
static int add_clipped_spans(builder_t *b, char const *src, int from, int to,
                             md_span_t const *spans, int nspans,
                             md_metrics_t const *m)
{
    int i, width = 0;

    for (i = 0; i < nspans; i++) {
        int lo = (int)spans[i].offset;
        int hi = lo + (int)spans[i].length;
        int a = lo > from ? lo : from;
        int c = hi < to ? hi : to;

        if (a >= c)
            continue;
        add_span(b, a, c - a, spans[i].style);
        {
            int p = a;
            while (p < c) {
                uint32_t cp = md_utf8_decode(src, c, &p);
                width += glyph_width(m, cp, spans[i].style);
                count_glyph(b->stats, m, cp);
            }
        }
    }
    return width;
}

/* Chooses the end of one wrapped line starting at `start`.
 * *width gets the used pixels.  Returns the exclusive end offset. */
static int fit_line(char const *src, int start, int end, int width_limit,
                    md_span_t const *spans, int nspans, md_metrics_t const *m,
                    int *width)
{
    int i = start, last_break = -1, w = 0;

    while (i < end) {
        uint8_t style = style_at(spans, nspans, i);
        int next = i;
        uint32_t cp = md_utf8_decode(src, end, &next);
        int cw = glyph_width(m, cp, style);

        if (cp == '\n') {
            /* A merged paragraph line break behaves like a space. */
            last_break = next;
            cw = glyph_width(m, ' ', style);
        }

        if (w + cw > width_limit && i > start) {
            if (last_break > start) {
                *width = w;
                return last_break;
            }
            *width = w;
            return i; /* a single word longer than the screen: hard break */
        }
        w += cw;
        if (cp == ' ' || cp == '\t')
            last_break = next;
        i = next;
    }
    *width = w;
    return end;
}

/* ---- public API --------------------------------------------------------- */

int md_layout(char const *src, int len, md_block_t const *blocks, int nblocks,
              int max_width, md_metrics_t const *metrics,
              md_display_line_t *lines, int max_lines,
              md_display_span_t *spans, int max_spans,
              md_layout_stats_t *stats)
{
    builder_t b;
    md_layout_stats_t local;
    int bi;

    if (!src || len < 0 || !metrics || !lines || !spans)
        return -1;
    if (max_width <= 0)
        max_width = 1;

    memset(&b, 0, sizeof b);
    b.lines = lines;
    b.max_lines = max_lines > 0 ? max_lines : 0;
    b.spans = spans;
    b.max_spans = max_spans > 0 ? max_spans : 0;
    memset(&local, 0, sizeof local);
    b.stats = &local;

    for (bi = 0; bi < nblocks; bi++) {
        md_block_t const *blk = &blocks[bi];
        int indent = 0;

        if (blk->type == MD_BLOCK_BULLET || blk->type == MD_BLOCK_ORDERED)
            indent = ((int)blk->level + 1) * MD_LAYOUT_INDENT_PX;
        else if (blk->type == MD_BLOCK_QUOTE)
            indent = MD_LAYOUT_INDENT_PX;

        if (indent > max_width - 16)
            indent = max_width / 4; /* keep room for at least a few glyphs */

        if (blk->type == MD_BLOCK_RULE) {
            int line = begin_line(&b, MD_BLOCK_RULE, 0, 0, 0);
            close_line(&b, line, 0);
            continue;
        }

        if (blk->type == MD_BLOCK_CODE) {
            /* Code lines are not wrapped: the renderer clips them. */
            int pos = (int)blk->offset;
            int end = pos + (int)blk->length;

            if (blk->length == 0)
                continue; /* an empty fence draws nothing */

            while (pos <= end) {
                int line_end = pos, line_start = pos, width = 0;
                int line;
                while (line_end < end && src[line_end] != '\n')
                    line_end++;

                line = begin_line(&b, MD_BLOCK_CODE, 0, 0, (uint16_t)indent);
                add_span(&b, line_start, line_end - line_start, MD_SPAN_CODE);
                {
                    int p = line_start;
                    while (p < line_end) {
                        uint32_t cp = md_utf8_decode(src, line_end, &p);
                        width += glyph_width(metrics, cp, MD_SPAN_CODE);
                        count_glyph(&local, metrics, cp);
                    }
                }
                close_line(&b, line, width);

                if (line_end >= end)
                    break;
                pos = line_end + 1;
                if (pos > end)
                    break;
            }
            continue;
        }

        /* Text blocks: inline spans, then greedy wrapping. */
        {
            md_span_t block_spans[MD_BLOCK_SPANS_MAX];
            uint8_t marker = 0;
            int nspans = md_parse_spans(src, (int)blk->offset,
                                        (int)(blk->offset + blk->length),
                                        block_spans, MD_BLOCK_SPANS_MAX);
            int start = (int)blk->offset;
            int end = start + (int)blk->length;
            bool first_line = true;

            if (nspans > MD_BLOCK_SPANS_MAX)
                nspans = MD_BLOCK_SPANS_MAX;

            /* Ordered lists carry their number, bullets their check state;
             * only the first line of an item shows the marker. */
            if (blk->type == MD_BLOCK_ORDERED)
                marker = blk->marker;
            else if (blk->type == MD_BLOCK_BULLET)
                marker = blk->check;

            if (start >= end) {
                /* An empty item (a bare "-" or ">"): keep the marker line. */
                int line = begin_line(&b, blk->type, blk->level,
                                      first_line ? marker : 0,
                                      (uint16_t)indent);
                close_line(&b, line, 0);
                continue;
            }

            while (start < end) {
                int width = 0, stop, line;

                while (start < end && is_break_space(src[start]))
                    start++;
                if (start >= end)
                    break;

                stop = fit_line(src, start, end, max_width - indent,
                                block_spans, nspans, metrics, &width);
                /* Trim trailing blanks from the display line. */
                while (stop > start && is_break_space(src[stop - 1]))
                    stop--;

                line = begin_line(&b, blk->type, blk->level,
                                  first_line ? marker : 0,
                                  (uint16_t)indent);
                close_line(&b, line, add_clipped_spans(&b, src, start, stop,
                                                       block_spans, nspans,
                                                       metrics));
                (void)width;
                first_line = false;
                start = stop > start ? stop : start + 1;
            }
        }
    }

    local.lines = b.nlines;
    local.spans = b.nspans;
    if (stats)
        *stats = local;
    return b.nlines;
}

int md_layout_text(char const *src, int len, int max_width,
                   md_metrics_t const *metrics, md_display_line_t *lines,
                   int max_lines, md_display_span_t *spans, int max_spans,
                   md_layout_stats_t *stats)
{
    /* Walk the note in block chunks so a long file needs no big scratch
     * array: each chunk is laid out into the caller's line/span buffers. */
    md_block_t blocks[32];
    int from = 0, total_lines = 0, total_spans = 0, unsupported = 0;
    bool overflow = false;

    if (!src || len < 0)
        return -1;

    while (from < len) {
        int nblocks = md_parse_blocks_from(src, len, from, blocks, 32);
        md_layout_stats_t part;
        int i, consumed = 0;

        if (nblocks <= 0)
            break;
        if (nblocks > 32)
            nblocks = 32;

        memset(&part, 0, sizeof part);
        total_lines += md_layout(src, len, blocks, nblocks, max_width, metrics,
                                 lines + (total_lines < max_lines ? total_lines
                                                                  : 0),
                                 total_lines < max_lines
                                     ? max_lines - total_lines
                                     : 0,
                                 spans + (total_spans < max_spans ? total_spans
                                                                  : 0),
                                 total_spans < max_spans
                                     ? max_spans - total_spans
                                     : 0,
                                 &part);
        total_spans += part.spans;
        unsupported += part.unsupported;
        if (part.overflow)
            overflow = true;

        for (i = 0; i < nblocks; i++)
            if ((int)blocks[i].next_offset > consumed)
                consumed = (int)blocks[i].next_offset;
        if (consumed <= from)
            break; /* defensive: never loop forever */
        from = consumed;
    }

    if (stats) {
        stats->lines = total_lines;
        stats->spans = total_spans;
        stats->unsupported = unsupported;
        stats->overflow = overflow;
    }
    return total_lines;
}
