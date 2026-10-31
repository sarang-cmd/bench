/* shapes.h - the Bauhaus shapes the home tiles are built from.
 *
 * The plan's home screen asks for "a square, circle, triangle, half circle and
 * bar. Plain shapes, no animation".  gint has no circle or triangle primitive,
 * so they are rasterised here with horizontal spans (cheap and exact). */

#ifndef BENCH_SHAPES_H
#define BENCH_SHAPES_H

void sh_square(int x, int y, int size, int color);
void sh_square_frame(int x, int y, int size, int thickness, int color);
void sh_circle(int cx, int cy, int r, int color);
void sh_half_circle(int cx, int cy, int r, int color, int fill_h);
void sh_triangle(int x, int y, int w, int h, int color);
void sh_bars(int x, int y, int w, int h, int color);
void sh_ring(int cx, int cy, int r, int thickness, int color);

/* ---- icons --------------------------------------------------------------
 *
 * Every tile icon is the same shape drawn twice: once in `edge`, grown by `ol`
 * pixels, then in `fill` at its nominal size.  That is the outline from
 * docs/design.md - it keeps a shape visible on any fill, which is the bug this
 * replaced (an icon painted in the tile's own colour disappeared). */

void sh_square_icon(int x, int y, int size, int fill, int edge, int ol);
void sh_circle_icon(int cx, int cy, int r, int fill, int edge, int ol);
void sh_half_circle_icon(int cx, int cy, int r, int fill_h, int fill, int edge,
                         int ol);
void sh_triangle_icon(int x, int y, int w, int h, int fill, int edge, int ol);
void sh_bars_icon(int x, int y, int w, int h, int fill, int edge, int ol);
void sh_ring_icon(int cx, int cy, int r, int thickness, int fill, int edge,
                  int ol);
void sh_square_frame_icon(int x, int y, int size, int fill, int edge, int ol);

#endif /* BENCH_SHAPES_H */
