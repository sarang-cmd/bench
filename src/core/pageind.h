/* pageind.h - the "there is another page" indicator.
 *
 * The tile grid used to mark extra pages with two or three 4 px squares and a
 * hole punched in the current one.  On the calculator that read as dust: the
 * report was "the two dots at the bottom are too subtle", and someone with four
 * tools off-screen never found them.
 *
 * The replacement is a bordered pill with the page number in it - "1 / 2" - and
 * an arrow at each end, filled when there is a page that way.  The label and the
 * arrow states are pure arithmetic, so they live here and are tested by
 * tests/test_pageind.c; the drawing stays in the UI.
 */

#ifndef BENCH_PAGEIND_H
#define BENCH_PAGEIND_H

/* Write the label for a zero-based page of `pages` as "1 / 2" (one-based, the
 * way the reader counts).  Returns the length written, or -1 on bad arguments
 * or a buffer that is too small - never a truncated label. */
int pageind_label(int page, int pages, char *out, int size);

/* Non-zero when there is a page before / after `page`. */
int pageind_left(int page);
int pageind_right(int page, int pages);

/* Clamp a step: pageind_step(page, pages, +1) is the next page, or the last one
 * when the page is already at the end.  Never returns a page outside the range,
 * so a stale selection cannot draw an empty grid. */
int pageind_step(int page, int pages, int dir);

#endif /* BENCH_PAGEIND_H */
