/* md_layout.h - turn Markdown blocks into display lines with span records.
 *
 * Spec: docs/plan.md section 6.4 ("Layout: measure word widths with the font,
 * wrap to the screen width, and produce an array of display lines. Each display
 * line holds span records: offset, length, style.").
 *
 * The font is not known to the core, so widths come in through callbacks.  The
 * same code therefore works on the PC (a synthetic 1-pixel font in the tests)
 * and on the calculator (the platform layer measures with the real font).
 *
 * Plain C11, no gint. */

#ifndef BENCH_MD_LAYOUT_H
#define BENCH_MD_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

#include "md_parse.h"

#define MD_LAYOUT_INDENT_PX 16   /* pixels per list/quote level */

/* What one character costs.  glyph_width returns pixels for the code point in
 * that style; has_glyph may be NULL, meaning every code point is available. */
typedef struct {
    int (*glyph_width)(void *ctx, uint32_t codepoint, uint8_t style);
    bool (*has_glyph)(void *ctx, uint32_t codepoint);
    void *ctx;
} md_metrics_t;

typedef struct {
    uint32_t offset;    /* byte offset into the source buffer */
    uint16_t length;
    uint8_t style;      /* md_span_style_t */
    uint8_t pad;
} md_display_span_t;

/* A display line.  `kind` uses md_block_type_t so the renderer can colour
 * headings and draw bullets, quotes and rules. */
typedef struct {
    uint16_t first_span;
    uint16_t span_count;
    uint16_t width;     /* pixels actually used, indent excluded */
    uint16_t indent;    /* pixels of left indent, included in nothing above */
    uint8_t kind;
    uint8_t level;      /* heading level or list depth */
    uint8_t marker;     /* checkbox state, or the ordered list number (0 none) */
    uint8_t pad;
} md_display_line_t;

typedef struct {
    int lines;          /* total display lines */
    int spans;          /* total display spans */
    int unsupported;    /* code points the font does not have (shown as '?') */
    bool overflow;      /* the caller's buffers were too small */
} md_layout_stats_t;

/* Lay out blocks into display lines.
 *   max_width  available pixels for text (indent comes out of this)
 *   lines      output array, max_lines entries
 *   spans      output array, max_spans entries
 * Returns the number of display lines produced (also in stats->lines). */
int md_layout(char const *src, int len, md_block_t const *blocks, int nblocks,
              int max_width, md_metrics_t const *metrics,
              md_display_line_t *lines, int max_lines,
              md_display_span_t *spans, int max_spans,
              md_layout_stats_t *stats);

/* Convenience: parse and lay out in one call.  `blocks` is scratch space. */
int md_layout_text(char const *src, int len, int max_width,
                   md_metrics_t const *metrics, md_display_line_t *lines,
                   int max_lines, md_display_span_t *spans, int max_spans,
                   md_layout_stats_t *stats);

/* UTF-8 helpers, exported because the Notes viewer needs them for the glyph
 * check.  Decode returns the code point and advances *i; 0xFFFD on a bad
 * sequence (one byte is consumed either way). */
uint32_t md_utf8_decode(char const *s, int len, int *i);

/* Number of code points in a byte range. */
int md_utf8_length(char const *s, int len);

#endif /* BENCH_MD_LAYOUT_H */
