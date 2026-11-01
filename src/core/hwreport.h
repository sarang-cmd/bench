/* hwreport.h - the eight assumptions Bench makes about the calculator, and the
 * plain-text report the hardware session fills in.
 *
 * docs/STATUS.md lists the same eight rows ("Assumptions that still need the
 * calculator").  The screen shows them, the user marks each one after trying it
 * on a real fx-CG 50, and F2 writes the result to state/hwreport.txt so it can
 * be copied off the calculator without retyping anything.
 *
 * No gint here: this module only formats text. */

#ifndef BENCH_HWREPORT_H
#define BENCH_HWREPORT_H

#include <stdbool.h>

#define HWREPORT_ITEMS 8

/* Every row starts unverified and stays that way until someone marks it. */
typedef enum {
    HW_UNKNOWN = 0,   /* not tried on a device yet */
    HW_WORKS,         /* the device agreed with the code */
    HW_FAILS          /* the device disagreed; the note says how */
} hw_state_t;

void hwreport_reset(hw_state_t *state);

/* Short label for row `i`, or "" outside 0..HWREPORT_ITEMS-1. */
char const *hwreport_label(int i);

/* The full assumption, one line, for the report file. */
char const *hwreport_detail(int i);

char const *hwreport_state_text(hw_state_t s);

/* unknown -> works -> fails -> unknown */
hw_state_t hwreport_next(hw_state_t s);

/* How many rows are no longer unknown. */
int hwreport_marked(hw_state_t const *state);

/* Build the report.  Writes at most `size` bytes including the NUL and returns
 * the length that was written, or -1 when it would not fit (nothing is
 * written then).  `version` may be NULL. */
int hwreport_format(char const *version, hw_state_t const *state, char *out,
                    int size);

#endif /* BENCH_HWREPORT_H */
