/* md_parse.h - block classifier and inline span parser for the Markdown subset.
 *
 * Spec: docs/plan.md section 6.4.  Supported:
 *   headings # ## ###, **bold**, *italic* / _italic_, `code`,
 *   fenced code blocks, - and * bullets, 1. lists, > quotes, --- rules,
 *   [text](url) as text, - [ ] and - [x] check boxes.
 * Everything else (tables, images, HTML, footnotes, deeper lists) is kept as
 * plain text, as the plan requires.
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

/* True when the line is blank (spaces/tabs only). */
bool md_line_is_blank(char const *line, int len);

#endif /* BENCH_MD_PARSE_H */
