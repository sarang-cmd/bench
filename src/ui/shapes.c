/* shapes.c - see shapes.h. */

#include "shapes.h"

#include "platform/screen.h"

/* Integer square root, so a circle needs no libm. */
static int isqrt(int v)
{
    int r = 0;

    if (v <= 0)
        return 0;
    while ((r + 1) * (r + 1) <= v)
        r++;
    return r;
}

void sh_square(int x, int y, int size, int color)
{
    scr_fill(x, y, size, size, color);
}

void sh_square_frame(int x, int y, int size, int thickness, int color)
{
    scr_frame(x, y, size, size, color, thickness);
}

void sh_circle(int cx, int cy, int r, int color)
{
    int r2 = r * r;

    for (int dy = -r; dy <= r; dy++)
    {
        int dx = isqrt(r2 - dy * dy);

        scr_hline(cx - dx, cy + dy, 2 * dx + 1, color);
    }
}

void sh_half_circle(int cx, int cy, int r, int color, int fill_h)
{
    int r2 = r * r;
    int bottom = cy + fill_h;

    for (int dy = -r; dy <= fill_h; dy++)
    {
        int dx = isqrt(r2 - dy * dy);

        scr_hline(cx - dx, cy + dy, 2 * dx + 1, color);
    }
    /* Square off the bottom so it reads as a "half circle" tile. */
    if (fill_h > 0)
        scr_fill(cx - r, bottom, 2 * r + 1, 2, color);
}

void sh_triangle(int x, int y, int w, int h, int color)
{
    for (int row = 0; row < h; row++)
    {
        int half = (w * row) / (h > 1 ? (h - 1) : 1) / 2;

        scr_hline(x + w / 2 - half, y + row, 2 * half + 1, color);
    }
}

void sh_bars(int x, int y, int w, int h, int color)
{
    int bw = w / 5 > 1 ? w / 5 : 1;
    int gap = (w - 3 * bw) / 3;

    scr_fill(x, y + h / 2, bw, h - h / 2, color);
    scr_fill(x + bw + gap, y + h / 4, bw, h - h / 4, color);
    scr_fill(x + 2 * (bw + gap), y, bw, h, color);
}

void sh_ring(int cx, int cy, int r, int thickness, int color)
{
    int r2 = r * r;
    int in = r - thickness;
    int in2 = in * in;

    for (int dy = -r; dy <= r; dy++)
    {
        int dx = isqrt(r2 - dy * dy);
        int din = (dy >= -in && dy <= in) ? isqrt(in2 - dy * dy) : 0;

        if (din > 0)
        {
            scr_hline(cx - dx, cy + dy, dx - din, color);
            scr_hline(cx + din + 1, cy + dy, dx - din, color);
        }
        else
        {
            scr_hline(cx - dx, cy + dy, 2 * dx + 1, color);
        }
    }
}
