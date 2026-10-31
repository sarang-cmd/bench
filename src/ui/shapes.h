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

#endif /* BENCH_SHAPES_H */
