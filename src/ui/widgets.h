/* widgets.h - the five pieces every screen is made of.
 *
 * Spec: docs/plan.md section 5 and TASK-M2 ("list, number field, modal,
 * softkey bar").
 *
 * Nothing here reads a key or a file: the widgets draw, the app decides.  The
 * one exception is w_keyboard(), which is a small modal loop because typing on
 * a calculator needs its own event handling. */

#ifndef BENCH_WIDGETS_H
#define BENCH_WIDGETS_H

#include <stdbool.h>

/* Title bar: ink strip, paper text, optional right-hand hint. */
void w_title(char const *title, char const *hint);

/* Softkey bar at the bottom.  `labels` has 6 entries; a NULL or empty entry is
 * drawn as a blank slot.  `active` inverts one slot (0..5) or -1. */
void w_softkeys(char const *const labels[6], int active);

/* The softkey bar without softkeys: a plain strip with an optional left hint
 * and a right-hand action.  Used by screens whose keys are all on the keypad
 * (help, About, an empty list), so the bar does not show six empty slots. */
void w_strip(char const *left, char const *right);

/* A raised panel for grouping content. */
void w_panel(int x, int y, int w, int h);

/* The line above a file list: "Notes in Bench/notes", or "Notes in SAVE-F -
 * Bench cannot write there" when the files came from another folder.  Keeps
 * the three list screens saying the same thing, and keeps the difference
 * visible instead of silent. */
void w_source_line(char const *what, int at, char const *bench_path);

/* Centred message in the content area, with an optional second line. */
void w_message(char const *title, char const *line);

/* Framed box over the content area, used for help and confirmations. */
void w_modal(char const *title, char const *const *lines, int count,
             char const *hint);

/* One row of a scrollable list.  `sel` draws the selection bar. */
void w_row(int x, int y, int w, char const *text, char const *right,
           bool sel);

/* How many list rows fit in `h` pixels. */
int w_rows_in(int h);

/* Scroll window helpers: keep `sel` inside [top, top+visible). */
int w_scroll_follow(int sel, int top, int visible);
/* Draws the scrollbar on the right edge of a list area. */
void w_scrollbar(int x, int y, int h, int count, int visible, int top);

/* Text field box.  `label` on the left, the value right of it. */
/* Height of a field box: text plus five pixels of padding above and below,
 * so a doubled text size gets a doubled box instead of text on the border. */
int w_field_h(void);

void w_field(int x, int y, int w, int h, char const *label,
             char const *value, bool active);

/* On-screen keyboard (see platform/input.h for why it exists).
 * Draws a modal over the content area; arrows move, EXE picks, EXIT cancels,
 * DEL or the DEL cell removes the last character.
 * Returns 1 when the text was accepted, 0 when cancelled. */
int w_keyboard(char *buf, int size, char const *title);

/* Progress bar with a text label; used by the number theory tool. */
void w_progress(int x, int y, int w, int h, char const *label, int permille);

#endif /* BENCH_WIDGETS_H */
