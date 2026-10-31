/* pacer.h - Pacer maths: minutes per mark, checkpoints, gap to pace.
 *
 * Core module: plain C11, no gint, fully testable on the PC.
 * Spec: docs/plan.md section 6.1.
 *
 * All times are in minutes as double.  The run clock is fed by the caller, so
 * the module never reads a timer itself. */

#ifndef BENCH_PACER_H
#define BENCH_PACER_H

#include <stdbool.h>
#include <stdint.h>

/* Upper bound on marks, so a typo cannot produce an absurd plan.
 * 999 is the smallest value the plan calls out; 1e6 leaves room for drills. */
#define PACER_MAX_MARKS 1000000u

typedef enum {
    PACER_OK = 0,
    PACER_ERR_NO_TIME,     /* total minutes <= 0, or not a finite number */
    PACER_ERR_NO_MARKS,    /* total marks == 0 */
    PACER_ERR_RANGE,       /* a value is outside what the tool accepts */
    PACER_ERR_CHECKPOINT,  /* checkpoint size == 0 */
} pacer_status_t;

typedef struct {
    double total_minutes;
    uint32_t total_marks;
    uint32_t checkpoint_marks;
} pacer_setup_t;

/* Validate the inputs and fill *s.  On error *s is zeroed and the caller shows
 * an error line; there is no next screen. */
pacer_status_t pacer_setup(pacer_setup_t *s, double total_minutes,
                           uint32_t total_marks, uint32_t checkpoint_marks);

/* Minutes per mark: T / M.  Returns 0 for an invalid setup. */
double pacer_mpm(pacer_setup_t const *s);

/* Checkpoint k (1-based) sits at k * C * mpm minutes.
 * Returns a negative value for k == 0 or for an invalid setup. */
double pacer_checkpoint_minutes(pacer_setup_t const *s, uint32_t k);

/* Number of checkpoints inside the paper: floor(M / C).  A paper with fewer
 * marks than one checkpoint has none. */
uint32_t pacer_checkpoint_count(pacer_setup_t const *s);

/* Target marks at minute t: t / mpm. */
double pacer_target_marks(pacer_setup_t const *s, double minutes);

/* Gap in minutes: t - done * mpm.  Positive means behind, negative ahead. */
double pacer_gap_minutes(pacer_setup_t const *s, double minutes,
                         uint32_t done);

typedef enum {
    PACER_GAP_AHEAD = -1,
    PACER_GAP_ON_PACE = 0,
    PACER_GAP_BEHIND = 1,
} pacer_gap_state_t;

/* Classify a gap.  Anything within half a second of zero counts as on pace. */
pacer_gap_state_t pacer_gap_state(double gap_minutes);

/* Index (1-based) of the next checkpoint not yet reached at minute t.
 * Equals pacer_checkpoint_count() + 1 when every checkpoint has passed. */
uint32_t pacer_next_checkpoint(pacer_setup_t const *s, double minutes);

/* True once the clock has reached the total time. */
bool pacer_finished(pacer_setup_t const *s, double minutes);

/* Minutes remaining, never negative. */
double pacer_remaining_minutes(pacer_setup_t const *s, double minutes);

/* ---------------------------------------------------------------- run clock */

/* A pausable run clock.  The caller passes "now" in minutes from any monotonic
 * source; the module only accumulates.  No gint, no timers. */
typedef struct {
    double accumulated;  /* minutes already banked */
    double started_at;   /* "now" when the clock last started */
    bool running;
} pacer_clock_t;

void pacer_clock_init(pacer_clock_t *c);
void pacer_clock_start(pacer_clock_t *c, double now_minutes);
void pacer_clock_pause(pacer_clock_t *c, double now_minutes);
void pacer_clock_resume(pacer_clock_t *c, double now_minutes);
bool pacer_clock_running(pacer_clock_t const *c);

/* Elapsed run minutes at "now".  Paused time is excluded, the result never
 * decreases and never goes negative. */
double pacer_clock_elapsed(pacer_clock_t const *c, double now_minutes);

#endif /* BENCH_PACER_H */
