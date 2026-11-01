/* md_parse.h - block classifier and inline span parser for the Markdown subset.
 *
 * Spec: docs/plan.md section 6.4.  Supported:
 *   headings # ## ###, **bold**, *italic* / _italic_, `code`,
 *   fenced code blocks, - and * bullets, 1. lists, > quotes, --- rules,
 *   [text](url) as text, - [ ] and - [x] check boxes,
 *   $x^2$ inline math and $$ display math (1.3.0), pipe tables (1.3.0).
 * Everything else (images, HTML, footnotes, deeper lists) is kept as plain
 * text, as the plan requires.
 *
 * LaTeX is parsed, not typeset: md_latex_to_text() turns the subset a school
 * formula needs into single characters the built-in font can draw (Greek,
 * operators, superscript digits).  A stacked fraction cannot be drawn by a
 * bitmap font, so \frac becomes a/b; the structure is documented in md_parse.c.
 *
 * No recursion: inline spans come from a single left-to-right state machine.
 *
 * Plain C11, no gint.  Every offset is a byte offset into the source buffer. */

#ifndef BENCH_MD_PARSE_H
#define BENCH_MD_PARSE_H

#include <stdbool.h>
#include <stdint.h>

#define MD_MAX_HEADING_LEVEL 3
#define MD_MAX_LIST_LEVEL 2      /* three levels: 0, 1, 2 */
#define MD_INDENT_SPACES 2       /* spaces per list level */

typedef enum {
    MD_BLOCK_PARAGRAPH = 0,
    MD_BLOCK_HEADING,
    MD_BLOCK_BULLET,
    MD_BLOCK_ORDERED,
    MD_BLOCK_QUOTE,
    MD_BLOCK_RULE,
    MD_BLOCK_CODE,
    MD_BLOCK_MATH,     /* $$ ... $$ on their own lines */
    MD_BLOCK_TABLE,    /* one row of a table, already padded by md_prepare() */
} md_block_type_t;

#define MD_CHECK_NONE 0
#define MD_CHECK_TODO 1
#define MD_CHECK_DONE 2

typedef struct {
    uint8_t type;        /* md_block_type_t */
    uint8_t level;       /* heading level 1..3, or list depth 0..2 */
    uint8_t check;       /* MD_CHECK_* for bullet blocks */
    uint8_t marker;      /* ordered list number, else 0 */
    uint16_t first_line; /* 0-based source line of the block */
    uint16_t line_count; /* source lines in the block (code blocks) */
    uint32_t offset;     /* content start (markers stripped) */
    uint32_t length;     /* content length, no trailing newline */
    uint32_t next_offset;/* first byte after the block, for resuming a parse */
} md_block_t;

typedef enum {
    MD_SPAN_TEXT = 0,
    MD_SPAN_BOLD,
    MD_SPAN_ITALIC,
    MD_SPAN_CODE,
    MD_SPAN_MATH,      /* $...$: the source between the dollars */
} md_span_style_t;

typedef struct {
    uint32_t offset;
    uint16_t length;
    uint8_t style;      /* md_span_style_t */
    uint8_t pad;
} md_span_t;

/* Classify `len` bytes of `src` into blocks.
 * Writes at most `max` blocks.  Returns the total number of blocks found; when
 * that is larger than `max`, the caller knows the rest did not fit. */
int md_parse_blocks(char const *src, int len, md_block_t *blocks, int max);

/* Same, starting at byte offset `from`.  Used to lay out a long note in
 * chunks; returns the number of blocks found from that offset. */
int md_parse_blocks_from(char const *src, int len, int from,
                         md_block_t *blocks, int max);

/* Parse inline spans of src[start..end) (one block's content).
 * Writes at most `max` spans and returns the total found.  Spans cover the text
 * only: markup characters are gaps between spans, so a renderer that walks the
 * spans never draws them. */
int md_parse_spans(char const *src, int start, int end, md_span_t *spans,
                   int max);

/* ---- tables ------------------------------------------------------------- */

#define MD_MAX_COLS 4

typedef enum {
    MD_ALIGN_LEFT = 0,
    MD_ALIGN_CENTER,
    MD_ALIGN_RIGHT,
} md_align_t;

/* One cell of a table row: byte range into the row, without the outer pipes
 * and without surrounding spaces. */
typedef struct {
    uint32_t offset;
    uint16_t length;
    uint16_t pad;
} md_cell_t;

typedef struct {
    uint8_t cols;                  /* number of columns, <= MD_MAX_COLS */
    uint8_t align[MD_MAX_COLS];    /* md_align_t per column */
    uint32_t header_offset;        /* byte offset of the header row */
    uint32_t body_offset;          /* byte offset of the first data row */
} md_table_t;

/* Split one table row into cells.  Writes at most `max` cells, returns the
 * number the row holds; padded and unpadded rows both work, "| |" is empty. */
int md_parse_table_row(char const *line, int len, md_cell_t *cells, int max);

/* Look for a table at the start of `src`.  A table is a header row, then a
 * separator row whose every cell is dashes with optional colons.  Returns the
 * number of data rows (0 when there is no table, in which case *out is
 * untouched). */
int md_parse_table(char const *src, int len, md_table_t *out);

/* ---- LaTeX -------------------------------------------------------------- */

/* Convert the LaTeX subset in src[0..len) to plain characters in out (always
 * NUL terminated, never overruns `outsz`).  Returns the number of bytes
 * written, or -1 when the arguments are bad.  See md_parse.c for the table. */
int md_latex_to_text(char const *src, int len, char *out, int outsz);

/* Same conversion, but every symbol becomes its ASCII spelling ("pi", "<=",
 * "x^2", "1/2").  This is what the notes viewer uses: the shipped font covers
 * ASCII only, so a UTF-8 symbol would draw as '?'. */
int md_latex_to_ascii(char const *src, int len, char *out, int outsz);

/* ---- preparing a note for the viewer ------------------------------------
 *
 * The layout layer measures the source bytes it is given, so anything the
 * renderer would draw differently has to be rewritten first.  md_prepare()
 * copies `src` into `out` and:
 *   - replaces $...$ and $$...$$ math with its ASCII spelling,
 *   - expands a pipe table into aligned, padded rows with a rule under the
 *     header, using the alignment row to pad left, centre or right.  `cols` is
 *     the number of characters the viewer can show on one line, so the widest
 *     column shrinks until the row fits.
 * It never writes more than `outsz` bytes and always NUL terminates.  Returns
 * the bytes written (excluding the NUL), or -1 on bad arguments. */
int md_prepare(char const *src, int len, char *out, int outsz, int cols);

/* True when the line is blank (spaces/tabs only). */
bool md_line_is_blank(char const *line, int len);

#endif /* BENCH_MD_PARSE_H */
