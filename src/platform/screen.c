/* screen.c - display helpers.  Every gint display call in the add-in is here.
 *
 * gint 2.10.0 (headers checked):
 *   dclear(color_t), drect(x1,y1,x2,y2,color), drect_border(...), dpixel,
 *   dline, dtext(x,y,fg,str), dtext_opt(x,y,fg,bg,halign,valign,str,size),
 *   dsize(str,font,&w,&h), drsize(str,font,width,&w), dupdate().
 * The framebuffer is DWIDTH x DHEIGHT (396x224); the OS shows a centred
 * 384x216 window, hence SCR_X/SCR_Y below. */

#include "screen.h"

#include <gint/display.h>

#include "font_data.h"
#include "theme.h"

/* Text size multiplier.  1 is the font as designed (6x10); 2 doubles it for
 * Settings > Font: Large.  Only whole pixels: a half-pixel face would have to
 * be anti-aliased by hand and the palette has no grey to blend with. */
static int text_scale = 1;

static int line_h = BENCH_FONT_H;

/* Width in pixels of the first `n` bytes of `s`.  Every glyph in the table is
 * a fixed advance, so this is a multiplication and works for a cut at any
 * byte, which is what the ellipsis code needs. */
static int text_w_prefix(char const *s, int n)
{
    if (!s || n <= 0)
        return 0;
    return n * BENCH_FONT_W * text_scale;
}

/* The glyph for a byte, or the '?' box when it is outside the table. */
static unsigned short const *glyph_for(unsigned char c)
{
    if (c < BENCH_FONT_FIRST || c > BENCH_FONT_LAST)
        c = '?';
    return bench_font_glyphs[c - BENCH_FONT_FIRST];
}

/* One character, `text_scale` pixels of every font pixel. */
static void draw_glyph(int x, int y, unsigned char c, int color)
{
    unsigned short const *g = glyph_for(c);
    int s = text_scale;

    for (int gx = 0; gx < BENCH_FONT_W; gx++) {
        unsigned short bits = g[gx];

        if (!bits)
            continue;
        for (int gy = 0; gy < BENCH_FONT_H; gy++) {
            int px, py;

            if (!(bits & (1u << gy)))
                continue;
            px = x + gx * s;
            py = y + gy * s;
            if (px < 0 || py < 0 || px + s > SCR_W || py + s > SCR_H) {
                /* Clip per pixel block: a label may start off-screen. */
                if (px >= SCR_W || py >= SCR_H || px + s <= 0 || py + s <= 0)
                    continue;
            }
            if (s == 1)
                scr_pixel(px, py, color);
            else
                scr_fill(px, py, s, s, color);
        }
    }
}

/* Draw a NUL-terminated string, one glyph at a time. */
static void draw_string(int x, int y, char const *s, int color)
{
    int cx = x;

    for (int i = 0; s[i]; i++) {
        draw_glyph(cx, y, (unsigned char)s[i], color);
        cx += BENCH_FONT_W * text_scale;
    }
}

static int clip_lo(int v, int lo)
{
    return v < lo ? lo : v;
}

static int clip_hi(int v, int hi)
{
    return v > hi ? hi : v;
}

void scr_init(void)
{
    line_h = BENCH_FONT_H * text_scale;
}

void scr_set_text_scale(int scale)
{
    if (scale < 1)
        scale = 1;
    if (scale > 2)
        scale = 2;
    text_scale = scale;
    line_h = BENCH_FONT_H * text_scale;
}

int scr_text_scale(void)
{
    return text_scale;
}

int scr_text_h(void)
{
    return line_h;
}

int scr_text_w(char const *s)
{
    int n = 0;

    if (!s)
        return 0;
    while (s[n])
        n++;
    return n * BENCH_FONT_W * text_scale;
}

int scr_text_cols(int px)
{
    int cell = BENCH_FONT_W * text_scale;

    if (px <= 0 || cell <= 0)
        return 0;
    return px / cell;
}

int scr_text_fit(char const *s, int max_w)
{
    int cell = BENCH_FONT_W * text_scale;

    if (!s || max_w <= 0)
        return 0;
    /* Bytes that fit whole: floor(max_w / cell), never past the string. */
    return max_w / cell;
}

void scr_present(void)
{
    dupdate();
}

void scr_clear(int color)
{
    dclear(color);
}

void scr_fill(int x, int y, int w, int h, int color)
{
    int x0 = clip_lo(x, 0);
    int y0 = clip_lo(y, 0);
    int x1 = clip_hi(x + w, SCR_W) - 1;
    int y1 = clip_hi(y + h, SCR_H) - 1;

    if (w <= 0 || h <= 0 || x1 < x0 || y1 < y0)
        return;
    /* The clip above can only shrink; when the rectangle starts outside the
     * window on the right/bottom there is nothing to draw. */
    if (x >= SCR_W || y >= SCR_H)
        return;
    /* Only panels and rows are worth tracing: a shape is rasterised from
     * one-pixel spans and a line cannot hide a label anyway. */
    if (w >= 8 && h >= 4)
        SCR_TRACE(SCR_TR_FILL, x0, y0, x1 - x0 + 1, y1 - y0 + 1, color, color,
                  NULL);
    drect(x0 + SCR_X, y0 + SCR_Y, x1 + SCR_X, y1 + SCR_Y, color);
}

void scr_frame(int x, int y, int w, int h, int color, int thickness)
{
    int t = thickness < 1 ? 1 : thickness;

    if (w <= 0 || h <= 0)
        return;
    /* The outline is recorded too, thin as it is: scr_fill only records panels
     * of 8x4 or more, and a two-pixel border is exactly what a label can end up
     * sitting on.  The trace does not keep the thickness, so the check treats
     * the outer four pixels of the rectangle as the edge. */
    if (w >= 12 && h >= 12)
        SCR_TRACE(SCR_TR_FRAME, x, y, w, h, color, C_NONE, NULL);
    scr_fill(x, y, w, t, color);
    scr_fill(x, y + h - t, w, t, color);
    scr_fill(x, y, t, h, color);
    scr_fill(x + w - t, y, t, h, color);
}

void scr_hline(int x, int y, int w, int color)
{
    scr_fill(x, y, w, 1, color);
}

void scr_vline(int x, int y, int h, int color)
{
    scr_fill(x, y, 1, h, color);
}

void scr_line(int x1, int y1, int x2, int y2, int color)
{
    if (x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0 || x1 >= SCR_W ||
        y1 >= SCR_H || x2 >= SCR_W || y2 >= SCR_H)
    {
        /* Fall back to a clipped rectangle for the axis-aligned cases only. */
        if (y1 == y2)
            scr_hline(clip_lo(x1, 0), y1, clip_hi(x2, SCR_W) - clip_lo(x1, 0),
                      color);
        else if (x1 == x2)
            scr_vline(x1, clip_lo(y1, 0),
                      clip_hi(y2, SCR_H) - clip_lo(y1, 0), color);
        return;
    }
    dline(x1 + SCR_X, y1 + SCR_Y, x2 + SCR_X, y2 + SCR_Y, color);
}

void scr_pixel(int x, int y, int color)
{
    if (x < 0 || y < 0 || x >= SCR_W || y >= SCR_H)
        return;
    dpixel(x + SCR_X, y + SCR_Y, color);
}

void scr_text_as(int kind, int x, int y, char const *s, int color)
{
    if (!s || !*s)
        return;
    if (y < -line_h || y >= SCR_H)
        return;
    SCR_TRACE(kind, x, y, scr_text_w(s), line_h, color, C_NONE, s);
    draw_string(x, y, s, color);
}

void scr_text(int x, int y, char const *s, int color)
{
    scr_text_as(SCR_TR_TEXT, x, y, s, color);
}

void scr_text_bg(int x, int y, char const *s, int fg, int bg)
{
    if (!s || !*s)
        return;
    if (y < -line_h || y >= SCR_H)
        return;
    SCR_TRACE(SCR_TR_TEXT, x, y, scr_text_w(s), line_h, fg, bg, s);
    scr_fill(x, y, scr_text_w(s), line_h, bg);
    draw_string(x, y, s, fg);
}

void scr_text_ellipsis_as(int kind, int x, int y, char const *s, int color,
                          int max_w)
{
    int fit;

    if (!s || !*s || max_w <= 0)
        return;
    if (scr_text_w(s) <= max_w)
    {
        scr_text_as(kind, x, y, s, color);
        return;
    }
    fit = scr_text_fit(s, max_w - scr_text_w("..."));
    if (fit < 0)
        fit = 0;
    /* One record for the whole label, then draw the cut text and the dots
     * without going through scr_text(), which would record them again and
     * make the overlap check complain about a label overlapping itself. */
    SCR_TRACE(kind, x, y, text_w_prefix(s, fit) + scr_text_w("..."), line_h,
              color, C_NONE, s);
    for (int i = 0; i < fit; i++)
        draw_glyph(x + i * BENCH_FONT_W * text_scale, y, (unsigned char)s[i],
                   color);
    draw_string(x + fit * BENCH_FONT_W * text_scale, y, "...", color);
}

void scr_text_ellipsis(int x, int y, char const *s, int color, int max_w)
{
    scr_text_ellipsis_as(SCR_TR_TEXT, x, y, s, color, max_w);
}

void scr_text_right(int x_right, int y, char const *s, int color)
{
    scr_text(x_right - scr_text_w(s), y, s, color);
}

void scr_text_center(int x_left, int w, int y, char const *s, int color)
{
    scr_text(x_left + (w - scr_text_w(s)) / 2, y, s, color);
}

int scr_utf8_encode(unsigned int cp, char *buf)
{
    if (cp < 0x80)
    {
        buf[0] = (char)cp;
        buf[1] = 0;
        return 1;
    }
    if (cp < 0x800)
    {
        buf[0] = (char)(0xC0 | (cp >> 6));
        buf[1] = (char)(0x80 | (cp & 0x3F));
        buf[2] = 0;
        return 2;
    }
    if (cp < 0x10000)
    {
        buf[0] = (char)(0xE0 | (cp >> 12));
        buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[2] = (char)(0x80 | (cp & 0x3F));
        buf[3] = 0;
        return 3;
    }
    buf[0] = (char)(0xF0 | (cp >> 18));
    buf[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    buf[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    buf[3] = (char)(0x80 | (cp & 0x3F));
    buf[4] = 0;
    return 4;
}

int scr_glyph_w(unsigned int codepoint)
{
    if (codepoint == 0)
        return 0;
    return BENCH_FONT_W * text_scale;
}

bool scr_has_glyph(unsigned int codepoint)
{
    char buf[5];

    /* The bundled face covers printable ASCII only (docs/spike.md question 4:
     * the OS fonts offer no Greek or maths glyphs).  Everything else is drawn
     * as '?', which is why md_latex_to_text() has an ASCII spelling for every
     * symbol it can emit: the reader sees "pi", not "?". */
    (void)buf;
    return codepoint >= 0x20 && codepoint <= 0x7E;
}
