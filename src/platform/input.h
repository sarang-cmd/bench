/* input.h - keys, the monotonic clock and text entry.
 *
 * Spec: docs/plan.md section 5 and the fallback it names for text entry.
 *
 * gint 2.10.0 (headers checked): getkey_opt()/getkey() return a key_event_t
 * with {time, mod, shift, alpha, type, key} (keyboard.h:110); KEYEV_NONE,
 * KEYEV_DOWN, KEYEV_UP, KEYEV_HOLD; pollevent() and waitevent(volatile int *)
 * exist; timer_configure() takes a GINT_CALL and timer_timeout() sets a flag.
 *
 * Text entry never depends on the ALPHA mapping: the ALPHA letters were not
 * confirmed by the M1 spike, so `input_keyboard()` draws its own letter grid
 * (A-Z, space, delete, OK) driven by the arrow keys and EXE.  That works the
 * same whether the OS maps ALPHA or not.
 *
 * This file also owns the monotonic clock: rtc_ticks() counts 128 Hz ticks
 * since midnight, so it wraps at midnight.  bench_now_minutes() accumulates
 * the unsigned difference between calls, which makes a wrap harmless. */

#ifndef BENCH_INPUT_H
#define BENCH_INPUT_H

#include <stdbool.h>

typedef enum {
    BK_NONE = 0,
    BK_UP, BK_DOWN, BK_LEFT, BK_RIGHT,
    BK_EXE, BK_EXIT, BK_MENU,
    BK_F1, BK_F2, BK_F3, BK_F4, BK_F5, BK_F6,
    BK_DIGIT,      /* ch holds '0'..'9' */
    BK_DOT,        /* ch holds '.' */
    BK_MINUS,      /* ch holds '-' */
    BK_ADD, BK_SUB, BK_MUL, BK_DIV,
    BK_DEL,
    BK_ALPHA,
} bench_key_t;

typedef struct {
    bench_key_t key;
    char ch;
} bench_event_t;

/* Blocking read of one key press (with gint's arrow repeat). */
bench_event_t input_next(void);

/* Wait up to `ms` milliseconds for a key.  Returns BK_NONE on timeout. */
bench_event_t input_next_timeout(int ms);

/* True while ALPHA is held, for the on-screen keyboard's help line. */
bool input_alpha(void);

/* True while EXIT is held.  Long jobs (factorisation) poll this to cancel. */
bool input_exit_down(void);

/* Monotonic minutes since the add-in started (rtc_ticks, wrap safe). */
double bench_now_minutes(void);

/* Milliseconds since the add-in started, for the redraw budget. */
unsigned int bench_now_ms(void);

/* Short name for a key, for help lines. */
char const *input_key_name(bench_key_t key);

#endif /* BENCH_INPUT_H */
