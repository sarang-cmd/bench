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

/* ---- icons -------------------------------------------------------------- */

void sh_square_icon(int x, int y, int size, int fill, int edge, int ol)
{
    if (ol > 0)
        sh_square(x - ol, y - ol, size + 2 * ol, edge);
    sh_square(x, y, size, fill);
}

void sh_circle_icon(int cx, int cy, int r, int fill, int edge, int ol)
{
    if (ol > 0)
        sh_circle(cx, cy, r + ol, edge);
    sh_circle(cx, cy, r, fill);
}

void sh_half_circle_icon(int cx, int cy, int r, int fill_h, int fill, int edge,
                         int ol)
{
    if (ol > 0)
        sh_half_circle(cx, cy, r + ol, edge, fill_h + ol);
    sh_half_circle(cx, cy, r, fill, fill_h);
}

void sh_triangle_icon(int x, int y, int w, int h, int fill, int edge, int ol)
{
    if (ol > 0)
        sh_triangle(x - 2 * ol, y - ol, w + 4 * ol, h + 2 * ol, edge);
    sh_triangle(x, y, w, h, fill);
}

void sh_bars_icon(int x, int y, int w, int h, int fill, int edge, int ol)
{
    int bw;
    int gap;

    if (ol > 0)
    {
        /* The outline is drawn per bar, so all three keep their shape. */
        int const ow = w + 2 * ol;
        int const obw = (ow / 5 > 1 ? ow / 5 : 1);
        int const ogap = (ow - 3 * obw) / 3;

        sh_square(x - ol, y + h / 2 - ol, obw + 2 * ol, edge);
        sh_square(x + obw + ogap - ol, y + h / 4 - ol, obw + 2 * ol, edge);
        sh_square(x + 2 * (obw + ogap) - ol, y - ol, obw + 2 * ol, edge);
        (void)ogap;
    }
    bw = w / 5 > 1 ? w / 5 : 1;
    gap = (w - 3 * bw) / 3;
    scr_fill(x, y + h / 2, bw, h - h / 2, fill);
    scr_fill(x + bw + gap, y + h / 4, bw, h - h / 4, fill);
    scr_fill(x + 2 * (bw + gap), y, bw, h, fill);
}

void sh_ring_icon(int cx, int cy, int r, int thickness, int fill, int edge,
                  int ol)
{
    if (ol > 0)
        sh_ring(cx, cy, r + ol, thickness + 2 * ol, edge);
    sh_ring(cx, cy, r, thickness, fill);
}

void sh_square_frame_icon(int x, int y, int size, int fill, int edge, int ol)
{
    int const t = 3;

    if (ol > 0)
        sh_square_frame(x - ol, y - ol, size + 2 * ol, t + 2 * ol, edge);
    sh_square_frame(x, y, size, t, fill);
}
