/* help.h - paging for the help screen.
 *
 * Spec: docs/plan.md (help) and the V2 plan, F: one help page per tool, under
 * 20 lines of 40 columns, taken from the tool's registry row.
 *
 * This is the part of the help screen that can be tested without a display:
 * counting lines, copying one line out, and clamping the scroll position to
 * the text.  app_help.c draws what these return.
 *
 * A help string is plain text with '\n' between lines and no trailing '\n'.
 */

#ifndef BENCH_HELP_H
#define BENCH_HELP_H

/* Lines in `text`; 1 for an empty string, 0 for NULL. */
int help_line_count(char const *text);

/* Copies line `index` (0-based) into `out` without its '\n', always NUL
 * terminated, and returns the number of bytes written.  A line longer than the
 * buffer is cut; an index past the end writes "". */
int help_line(char const *text, int index, char *out, int size);

/* The first visible line after scrolling `first` by `delta` with `fit` lines
 * on screen.  Never below 0 and never past the last full page, so the last
 * line of the text is always on screen. */
int help_scroll_pos(char const *text, int first, int fit, int delta);

/* ---- word wrapping ------------------------------------------------------
 *
 * Help text is stored pre-wrapped at the width it was written for.  When the
 * reader raises the text size, fewer columns fit, so the same text has to be
 * wrapped again at draw time.  These four functions do that in core, where the
 * vectors are cheap: a logical line becomes one or more display lines, broken
 * at the last space that fits, and hard-broken when a word is longer than the
 * line.  `cols` is a character count, so no font is involved. */

/* Display lines when every logical line is wrapped at `cols` characters. */
int help_wrap_count(char const *text, int cols);

/* Copy display line `index` into `out` (NUL terminated, cut to `size`).
 * Returns the bytes written; an index past the end writes "". */
int help_wrap_line(char const *text, int cols, int index, char *out, int size);

/* help_scroll_pos() and help_needs_scroll() for wrapped text. */
int help_wrap_scroll(char const *text, int cols, int first, int fit, int delta);
int help_wrap_needs_scroll(char const *text, int cols, int fit);

/* True when `text` needs more than `fit` lines. */
int help_needs_scroll(char const *text, int fit);

/* The line index of the tool's name at the top of its help text: 1 when the
 * first line repeats the title (the title bar already shows it), else 0. */
int help_skip_title(char const *text, char const *title);

#endif /* BENCH_HELP_H */
