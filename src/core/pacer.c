/* pacer.c - Pacer maths.  See pacer.h and docs/plan.md section 6.1. */

#include "pacer.h"

#include <math.h>

#define PACER_ON_PACE_TOLERANCE (0.5 / 60.0) /* half a second, in minutes */

static bool finite_positive(double x)
{
    return isfinite(x) && x > 0.0;
}

pacer_status_t pacer_setup(pacer_setup_t *s, double total_minutes,
                           uint32_t total_marks, uint32_t checkpoint_marks)
{
    if (!s)
        return PACER_ERR_RANGE;

    s->total_minutes = 0.0;
    s->total_marks = 0;
    s->checkpoint_marks = 0;

    if (!finite_positive(total_minutes))
        return PACER_ERR_NO_TIME;
    if (total_marks == 0)
        return PACER_ERR_NO_MARKS;
    if (total_marks > PACER_MAX_MARKS)
        return PACER_ERR_RANGE;
    if (checkpoint_marks == 0)
        return PACER_ERR_CHECKPOINT;
    if (checkpoint_marks > PACER_MAX_MARKS)
        return PACER_ERR_RANGE;

    s->total_minutes = total_minutes;
    s->total_marks = total_marks;
    s->checkpoint_marks = checkpoint_marks;
    return PACER_OK;
}

static bool setup_ok(pacer_setup_t const *s)
{
    return s && finite_positive(s->total_minutes) && s->total_marks > 0 &&
           s->checkpoint_marks > 0;
}

double pacer_mpm(pacer_setup_t const *s)
{
    if (!setup_ok(s))
        return 0.0;
    return s->total_minutes / (double)s->total_marks;
}

double pacer_checkpoint_minutes(pacer_setup_t const *s, uint32_t k)
{
    if (!setup_ok(s) || k == 0)
        return -1.0;
    return (double)k * (double)s->checkpoint_marks * pacer_mpm(s);
}

uint32_t pacer_checkpoint_count(pacer_setup_t const *s)
{
    if (!setup_ok(s))
        return 0;
    return s->total_marks / s->checkpoint_marks;
}

double pacer_target_marks(pacer_setup_t const *s, double minutes)
{
    double mpm = pacer_mpm(s);
    if (mpm <= 0.0 || !isfinite(minutes) || minutes < 0.0)
        return 0.0;
    return minutes / mpm;
}

double pacer_gap_minutes(pacer_setup_t const *s, double minutes,
                         uint32_t done)
{
    double mpm = pacer_mpm(s);
    if (mpm <= 0.0 || !isfinite(minutes))
        return 0.0;
    return minutes - (double)done * mpm;
}

pacer_gap_state_t pacer_gap_state(double gap_minutes)
{
    if (gap_minutes > PACER_ON_PACE_TOLERANCE)
        return PACER_GAP_BEHIND;
    if (gap_minutes < -PACER_ON_PACE_TOLERANCE)
        return PACER_GAP_AHEAD;
    return PACER_GAP_ON_PACE;
}

uint32_t pacer_next_checkpoint(pacer_setup_t const *s, double minutes)
{
    uint32_t count = pacer_checkpoint_count(s);
    uint32_t k;

    if (count == 0 || !isfinite(minutes))
        return count + 1;

    /* Walk up while the checkpoint time is at or before "minutes".  Counts are
     * at most PACER_MAX_MARKS / 1, so an arithmetic step is used instead of a
     * loop: k = floor(minutes / (C * mpm)) + 1. */
    double step = (double)s->checkpoint_marks * pacer_mpm(s);
    if (step <= 0.0)
        return count + 1;

    double passed = minutes / step;
    if (passed >= (double)count)
        return count + 1;
    if (passed <= 0.0)
        return 1;

    k = (uint32_t)passed + 1u; /* truncation = floor for positive values */
    if (k > count)
        return count + 1;
    return k;
}

bool pacer_finished(pacer_setup_t const *s, double minutes)
{
    if (!setup_ok(s) || !isfinite(minutes))
        return false;
    return minutes >= s->total_minutes;
}

double pacer_remaining_minutes(pacer_setup_t const *s, double minutes)
{
    if (!setup_ok(s) || !isfinite(minutes))
        return 0.0;
    double left = s->total_minutes - minutes;
    return left > 0.0 ? left : 0.0;
}

/* ---------------------------------------------------------------- run clock */

void pacer_clock_init(pacer_clock_t *c)
{
    if (!c)
        return;
    c->accumulated = 0.0;
    c->started_at = 0.0;
    c->running = false;
}

void pacer_clock_start(pacer_clock_t *c, double now_minutes)
{
    if (!c || !isfinite(now_minutes))
        return;
    c->accumulated = 0.0;
    c->started_at = now_minutes;
    c->running = true;
}

void pacer_clock_pause(pacer_clock_t *c, double now_minutes)
{
    if (!c || !c->running || !isfinite(now_minutes))
        return;
    double delta = now_minutes - c->started_at;
    if (delta > 0.0)
        c->accumulated += delta;
    c->running = false;
}

void pacer_clock_resume(pacer_clock_t *c, double now_minutes)
{
    if (!c || c->running || !isfinite(now_minutes))
        return;
    c->started_at = now_minutes;
    c->running = true;
}

bool pacer_clock_running(pacer_clock_t const *c)
{
    return c && c->running;
}

double pacer_clock_elapsed(pacer_clock_t const *c, double now_minutes)
{
    if (!c || !isfinite(now_minutes))
        return 0.0;
    if (!c->running)
        return c->accumulated;
    double delta = now_minutes - c->started_at;
    return c->accumulated + (delta > 0.0 ? delta : 0.0);
}
