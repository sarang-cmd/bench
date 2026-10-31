/* color.c - see color.h. */

#include "color.h"

#include "lin_table.h"

int col_rgb565(int r, int g, int b)
{
    int r5 = (r * 31 + 127) / 255;
    int g6 = (g * 63 + 127) / 255;
    int b5 = (b * 31 + 127) / 255;

    if (r5 < 0)
        r5 = 0;
    if (r5 > 31)
        r5 = 31;
    if (g6 < 0)
        g6 = 0;
    if (g6 > 63)
        g6 = 63;
    if (b5 < 0)
        b5 = 0;
    if (b5 > 31)
        b5 = 31;
    return (r5 << 11) | (g6 << 5) | b5;
}

void col_rgb888(int c565, int *r, int *g, int *b)
{
    int r5 = (c565 >> 11) & 0x1F;
    int g6 = (c565 >> 5) & 0x3F;
    int b5 = c565 & 0x1F;

    /* Replicate the high bits into the low ones, as the hardware does. */
    if (r)
        *r = (r5 * 255 + 15) / 31;
    if (g)
        *g = (g6 * 255 + 31) / 63;
    if (b)
        *b = (b5 * 255 + 15) / 31;
}

double col_luminance(int c565)
{
    int r = 0;
    int g = 0;
    int b = 0;

    col_rgb888(c565, &r, &g, &b);
    /* WCAG weights, applied to linear light. */
    return (0.2126 * lin_lut[r] + 0.7152 * lin_lut[g] + 0.0722 * lin_lut[b]) /
           65535.0;
}

double col_contrast(int a, int b)
{
    double la = col_luminance(a);
    double lb = col_luminance(b);
    double hi = la > lb ? la : lb;
    double lo = la > lb ? lb : la;

    return (hi + 0.05) / (lo + 0.05);
}

int col_better_on(int bg, int a, int b)
{
    return col_contrast(bg, b) > col_contrast(bg, a) ? b : a;
}
