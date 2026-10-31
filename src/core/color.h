/* color.h - colours, luminance and contrast, without gint.
 *
 * Spec: docs/design.md section 4 (never draw foreground and background in the
 * same colour) and docs/plan.md section 5 (the palette).
 *
 * Two jobs:
 *   - split a 5-6-5 RGB colour into 8-bit channels, the way the hardware and
 *     the host preview both do
 *   - return the WCAG contrast ratio of two colours, so a test can assert that
 *     a label is readable on its tile, and that an icon never disappears
 *
 * Plain C11, no gint, no libm: the sRGB curve is a generated table
 * (lin_table.h), so the calculator build links nothing extra. */

#ifndef BENCH_COLOR_H
#define BENCH_COLOR_H

#include <stdint.h>

/* Pack 8-bit channels into a 5-6-5 colour, rounding rather than truncating. */
int col_rgb565(int r, int g, int b);

/* Unpack into 8-bit channels.  Any pointer may be NULL. */
void col_rgb888(int c565, int *r, int *g, int *b);

/* Relative luminance, 0.0 (black) to 1.0 (white). */
double col_luminance(int c565);

/* WCAG contrast ratio, 1.0 (identical) to 21.0 (black on white).  Symmetric:
 * col_contrast(a, b) == col_contrast(b, a). */
double col_contrast(int a, int b);

/* True when the two ratios are close enough for an icon to be visible on its
 * background.  docs/design.md sets the floor at 1.5:1, which is about the
 * smallest difference that still reads as two colours on the calculator's
 * screen in daylight. */
#define COL_ICON_MIN_CONTRAST 1.5

/* Text must clear the WCAG AA floor. */
#define COL_TEXT_MIN_CONTRAST 4.5
/* The tile title is the largest thing on the screen; WCAG allows 3:1 for large
 * text.  docs/design.md explains why the title is allowed this floor and the
 * subtitle is not. */
#define COL_TITLE_MIN_CONTRAST 3.0

/* Of two foreground colours, the one that reads better on `bg`.  The palette
 * only has ink and paper to put on a fill, and on some accents neither reaches
 * 4.5:1, so callers ask for the better one and decide what to do about the
 * rest (docs/design.md: the subtitle gets a paper plate). */
int col_better_on(int bg, int a, int b);

#endif /* BENCH_COLOR_H */
