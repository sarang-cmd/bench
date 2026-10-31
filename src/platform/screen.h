/* screen.h - the only place that talks to the display.
 *
 * Spec: docs/plan.md section 5 (title bar, content, softkey bar) and the
 * 396x224 framebuffer with the OS's centred 384x216 window.
 *
 * Apps draw in window coordinates: (0,0) is the top left of the visible
 * 384x216 area.  This file adds the window offset so no app has to know it.
 *
 * Every function clips to the window, so a bad constant can never spill into
 * the strip the OS keeps for itself. */

#ifndef BENCH_SCREEN_H
#define BENCH_SCREEN_H

#include <stdbool.h>

#define SCR_W 384
#define SCR_H 216

/* Framebuffer offset of the visible window (gint: DWIDTH 396, DHEIGHT 224). */
#define SCR_X 6
#define SCR_Y 4

void scr_init(void);
void scr_present(void);

/* Height of one line of the default font, measured once in scr_init(). */
int scr_text_h(void);
int scr_text_w(char const *s);
/* Bytes of `s` that fit in `max_w` pixels (for clipping a label). */
int scr_text_fit(char const *s, int max_w);

void scr_clear(int color);
void scr_fill(int x, int y, int w, int h, int color);
void scr_frame(int x, int y, int w, int h, int color, int thickness);
void scr_hline(int x, int y, int w, int color);
void scr_vline(int x, int y, int h, int color);
void scr_line(int x1, int y1, int x2, int y2, int color);
void scr_pixel(int x, int y, int color);

/* Text.  A background of C_NONE leaves what is underneath. */
void scr_text(int x, int y, char const *s, int color);
void scr_text_bg(int x, int y, char const *s, int fg, int bg);
/* Text cut to `max_w` pixels, with "..." when it does not fit. */
void scr_text_ellipsis(int x, int y, char const *s, int color, int max_w);
void scr_text_right(int x_right, int y, char const *s, int color);
void scr_text_center(int x_left, int w, int y, char const *s, int color);

/* UTF-8: code point width in pixels, 0 when the font has no glyph. */
int scr_glyph_w(unsigned int codepoint);
bool scr_has_glyph(unsigned int codepoint);

/* Encode a code point as UTF-8 into `buf` (at least 5 bytes). */
int scr_utf8_encode(unsigned int cp, char *buf);

#endif /* BENCH_SCREEN_H */
