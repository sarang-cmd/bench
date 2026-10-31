/* textsearch.h - case-insensitive search and line helpers for the Notes viewer.
 *
 * Spec: docs/plan.md section 6.4 ("Search: case-insensitive substring. F-key
 * jumps to the next match and highlights the line.").
 *
 * Text is a byte range; length is passed explicitly so a file buffer needs no
 * terminator.  Case folding covers ASCII only: bytes >= 0x80 compare exactly,
 * which is what the viewer wants because the font layer maps them separately.
 *
 * Plain C11, no gint. */

#ifndef BENCH_TEXTSEARCH_H
#define BENCH_TEXTSEARCH_H

#include <stdbool.h>

/* ASCII lower case; every other byte is returned unchanged. */
char textsearch_lower(char c);

/* Finds `needle` in hay[from..hlen).  Returns the byte offset of the match, or
 * -1.  An empty needle never matches (that would match everywhere). */
int textsearch_find(char const *hay, int hlen, int from, char const *needle,
                    int nlen);

/* Finds the last match that starts before `before` (use hlen to search the
 * whole text).  Returns the offset or -1. */
int textsearch_find_back(char const *hay, int hlen, int before,
                         char const *needle, int nlen);

/* Number of non-overlapping matches, left to right. */
int textsearch_count(char const *hay, int hlen, char const *needle, int nlen);

/* Next match at or after `from`, wrapping once to the start of the text.
 * Returns -1 only when there is no match at all. */
int textsearch_find_wrap(char const *hay, int hlen, int from,
                         char const *needle, int nlen);

/* ---- line helpers ------------------------------------------------------- */

/* Number of lines in the text.  An empty text has 0 lines; "a" has 1 line;
 * "a\n" has 1 line (the trailing newline ends the last line, it does not start
 * a new one); "\n\n" has 2 empty lines. */
int textsearch_line_count(char const *text, int len);

/* Byte offset where line `index` starts, or len when the line is past the end.
 * Line 0 starts at 0. */
int textsearch_line_start(char const *text, int len, int index);

/* Byte offset just past the last character of line `index`, excluding the
 * newline (and a CR before it).  Returns -1 for a line past the end. */
int textsearch_line_end(char const *text, int len, int index);

/* 0-based line that contains byte offset `pos` (pos may be len). */
int textsearch_line_of(char const *text, int len, int pos);

/* True when the byte at `pos` is a newline or the offset is at the end. */
bool textsearch_at_line_end(char const *text, int len, int pos);

#endif /* BENCH_TEXTSEARCH_H */
