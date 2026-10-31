/* suvat.h - constant acceleration and projectiles (V2 feature F10).
 *
 * Spec: the V2 master prompt, F10.  Five quantities, any three of which fix the
 * other two:
 *
 *      v = u + a t
 *      s = u t + 1/2 a t^2
 *      s = (u + v) t / 2
 *      v^2 = u^2 + 2 a s
 *
 * The solver walks the ten possible triples in a fixed order and reports which
 * equation it used, because the point of the screen is the working, not the
 * number.  When the equations give two physically sensible answers (a quadratic
 * in t, or a square root that could be negative) it says so rather than picking
 * one quietly.
 *
 * No gint and no libm: the root is bm_sqrt from core/mathfn.c.
 */

#ifndef BENCH_SUVAT_H
#define BENCH_SUVAT_H

#include <stdbool.h>

typedef enum {
    SU_S = 0,
    SU_U,
    SU_V,
    SU_A,
    SU_T,
    SU_COUNT
} su_var_t;

typedef struct {
    double v[SU_COUNT];    /* s, u, v, a, t */
    bool known[SU_COUNT];
    su_var_t solved[2];    /* which two were worked out, in order */
    int solved_count;
    char eq[3][40];        /* the equations used, as text */
    int eq_count;
    bool two_times;        /* a quadratic gave two times; the positive one kept */
    double other_time;
} suvat_t;

/* "s", "u", "v", "a", "t" - never NULL. */
char const *suvat_name(su_var_t var);
char const *suvat_unit(su_var_t var);

/* Fills the two unknowns from the three knowns.  On failure returns false and
 * writes a sentence into `msg` saying what is missing or degenerate. */
bool suvat_solve(suvat_t *x, char *msg, int size);

/* A projectile from a height, at an angle above the horizontal, in degrees. */
typedef struct {
    double range;      /* horizontal distance to the ground */
    double peak;       /* greatest height above the ground */
    double flight;     /* time in the air */
    double vx, vy;     /* launch components, for the drawing */
} projectile_t;

bool projectile(double speed, double angle_deg, double height, double g,
                projectile_t *out);

#endif /* BENCH_SUVAT_H */
