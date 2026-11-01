/* nav.h - where EXIT goes.
 *
 * Spec: the V2 master prompt's "smart back" item, and docs/plan.md section 5.
 *
 * Before this, EXIT on any tool dropped the user on the home screen with the
 * first category selected and the first tile highlighted: someone working
 * through the Maths tools was sent back to Study every time, and had to walk
 * back to where they were.  The add-in keeps a small stack of the screens it
 * came through instead, so EXIT walks back the way you came.
 *
 * The stack is bounded and every push beyond it drops the oldest entry, so a
 * long session cannot grow it without limit and cannot corrupt it.
 *
 * Plain C11, no gint: main.c owns the screens, this owns the history.
 */

#ifndef BENCH_NAV_H
#define BENCH_NAV_H

#define NAV_DEPTH 6

/* Forget the history.  Called once at start-up. */
void nav_reset(void);

/* Remember `screen` as the one to come back to.  Pushing the screen that is
 * already on top is ignored, so opening the same tool twice does not need two
 * EXIT presses to get out. */
void nav_push(int screen);

/* The screen EXIT should go to, and remove it from the stack.
 * Returns -1 when there is nothing to go back to, which means the home
 * screen. */
int nav_pop(void);

/* The screen EXIT would go to, without changing anything (-1 when the home
 * screen is the answer). */
int nav_peek(void);

/* Entries on the stack, 0..NAV_DEPTH. */
int nav_depth(void);

#endif /* BENCH_NAV_H */
