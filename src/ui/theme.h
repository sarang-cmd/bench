/* theme.h - Bench palette, spacing and the Markdown style map.
 *
 * Palette from docs/plan.md section 5 (Bauhaus): paper #f4f3ee, ink #2f2d2e,
 * red #d92d27.  The three accents are used for the home tiles and for
 * headings so the add-in looks like the guide.
 *
 * gint colours are 5-6-5: C_RGB(r5, g6, b5). */

#ifndef BENCH_THEME_H
#define BENCH_THEME_H

#include <gint/display.h>

#include "platform/screen.h"

/* ---- palette ------------------------------------------------------------ */

/* The palette from docs/design.md, converted with col_rgb565() and checked
 * against the hex values by tools/sim on every run (the "palette" check), so a
 * hand-edited constant cannot drift. */
#define TH_PAPER 0xF79D   /* #f4f3ee */
#define TH_INK 0x3166     /* #2f2d2e */
#define TH_RED 0xD165     /* #d92d27 */
#define TH_YELLOW 0xEE06  /* #f2c230 */
#define TH_BLUE 0x32F9    /* #2f5fd0 */
#define TH_TEAL 0x24F1    /* #1f9c8a */
#define TH_VIOLET 0x8AFE  /* #8b5cf6 */
#define TH_SHADE 0xD6B9   /* #d9d7cd */

/* ---- layout ------------------------------------------------------------- */

#define TH_TITLE_H 18
#define TH_SOFT_H 16
#define TH_CONTENT_Y (TH_TITLE_H + 1)
#define TH_CONTENT_H (SCR_H - TH_TITLE_H - TH_SOFT_H - 2)
#define TH_PAD 6
#define TH_GAP 4
#define TH_LINE (scr_text_h() + 3)

/* Neobrutalist rules from docs/design.md section 3: flat fills, square
 * corners, hard offset shadows.  These are the only border widths and shadow
 * offsets the add-in may use. */
#define TH_BORDER 2       /* panel outline */
#define TH_BORDER_SEL 3   /* the selected tile */
#define TH_SHADOW 3       /* offset shadow, right and bottom */
#define TH_ICON_OL 2      /* icon outline on a selected tile, 1 when not */
#define TH_SOFT_W (SCR_W / 6)

/* ---- Markdown style map (Phase A: colour and underline only) ------------ */

#define TH_MD_PAPER TH_PAPER
#define TH_MD_INK TH_INK
#define TH_MD_H1 TH_RED
#define TH_MD_H2 TH_BLUE
#define TH_MD_H3 TH_INK
#define TH_MD_BOLD TH_INK
#define TH_MD_BOLD_BG TH_SHADE
#define TH_MD_ITALIC TH_INK
#define TH_MD_CODE TH_INK
#define TH_MD_CODE_BG TH_SHADE
#define TH_MD_QUOTE_BAR TH_RED
#define TH_MD_RULE TH_SHADE
#define TH_MD_BULLET TH_RED
#define TH_MD_SEARCH_HIT TH_YELLOW

#endif /* BENCH_THEME_H */
