/* formula.h - line and heading arithmetic for the formula booklets.
 *
 * Spec: the request to put the IB AA HL, Physics 2025 and Chemistry 2025
 * booklet content into Bench.  A booklet is a long plain-text file; the viewer
 * needs three things that are pure arithmetic and belong here rather than in
 * the screen:
 *
 *   - which line a byte offset falls on (a search hit is an offset, the screen
 *     draws lines)
 *   - where a line starts and ends
 *   - whether a line is a heading, and of which level, so LEFT/RIGHT can jump
 *     between sections the way a reader turns to a topic
 *
 * Headings are '#', '##' and '###' at the start of a line, one space, then the
 * text.  Nothing else is markup: a formula is a line of ASCII.
 *
 * Plain C11, no gint. */

#ifndef BENCH_FORMULA_H
#define BENCH_FORMULA_H

/* Lines in text[0..len).  A file ending in '\n' has no empty last line; an
 * empty file has 0 lines. */
int formula_line_count(char const *text, int len);

/* Byte offset where line `index` starts (len when the index is past the end). */
int formula_line_start(char const *text, int len, int index);

/* Byte offset of the '\n' that ends line `index`, or len when it is the last
 * line and has no newline. */
int formula_line_end(char const *text, int len, int index);

/* The line holding byte `off`; -1 when `off` is outside the text. */
int formula_line_of_offset(char const *text, int len, int off);

/* Heading level 1..3, or 0 for a line that is not a heading. */
int formula_heading_level(char const *text, int len, int line);

/* The next heading at or after line `from` (for RIGHT), or the previous one
 * before `from` (for LEFT), counting only headings of level `depth` or above
 * (level 1 is `#`).  The viewer passes 2, so a jump lands on a topic and not on
 * every sub-heading inside it: with depth 3 a booklet with a heading every few
 * lines jumps five lines at a time, which is not a section.
 * Returns `from` when there is none, so a caller cannot accidentally jump
 * backwards on a file with one heading. */
int formula_next_heading(char const *text, int len, int from, int depth);
int formula_prev_heading(char const *text, int len, int from, int depth);

#endif /* BENCH_FORMULA_H */
