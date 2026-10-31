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

/* #f4f3ee */
#define TH_PAPER 0xF79D   /* #f4f3ee */
/* #2f2d2e */
#define TH_INK 0x2965     /* #2f2d2e */
/* #d92d27 */
#define TH_RED 0xD964     /* #d92d27 */
/* #d9d7cd, panels and softkey strip */
#define TH_SHADE 0xDEB9   /* #d9d7cd */
/* #f2c230 */
#define TH_YELLOW 0xF606  /* #f2c230 */
/* #2f5fd0 */
#define TH_BLUE 0x2AFA    /* #2f5fd0 */
/* #1f9c8a */
#define TH_TEAL 0x1CF1    /* #1f9c8a */
/* #8b5cf6 */
#define TH_VIOLET 0x8AFE  /* #8b5cf6 */

/* ---- layout ------------------------------------------------------------- */

#define TH_TITLE_H 18
#define TH_SOFT_H 16
#define TH_CONTENT_Y (TH_TITLE_H + 1)
#define TH_CONTENT_H (SCR_H - TH_TITLE_H - TH_SOFT_H - 2)
#define TH_PAD 6
#define TH_GAP 4
#define TH_LINE (scr_text_h() + 3)
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
#define TH_MD_CODE TH_TEAL
#define TH_MD_CODE_BG TH_SHADE
#define TH_MD_QUOTE_BAR TH_RED
#define TH_MD_RULE TH_SHADE
#define TH_MD_BULLET TH_RED
#define TH_MD_SEARCH_HIT TH_YELLOW

#endif /* BENCH_THEME_H */
