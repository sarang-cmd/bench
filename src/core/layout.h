/* layout.h - centring arithmetic for the tile grid.
 *
 * The Tools category holds two tiles. It used to draw them hard against the
 * left margin with an empty third of the screen beside them, which the first
 * person to use the add-in called "awkward" - and it was: the eye expects a
 * short row to sit under the middle of the one above it.
 *
 * Rows are centred when they are not full. A full row lands on the margin
 * anyway, so the same code draws both, and the arithmetic is testable without a
 * screen (tests/test_layout.c).
 */

#ifndef BENCH_LAYOUT_H
#define BENCH_LAYOUT_H

/* Left offset of a row of `tiles` items, each `item` wide with `gap` between
 * them, centred in `span`.  Never negative: when the row is wider than the
 * span it starts at 0 and the caller clips.  With no tiles the offset is the
 * span's own left edge, 0. */
int layout_centre_row(int tiles, int item, int gap, int span);

/* How many tiles the row starting at `first` of `total` on the page holds,
 * given `per_row` columns.  Used to decide whether a row is short. */
int layout_row_count(int first, int total, int per_row);

#endif /* BENCH_LAYOUT_H */
