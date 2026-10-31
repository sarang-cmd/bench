/* triangle.h - the triangle solver (V2 F2).
 *
 * Spec: the V2 master prompt, F2.  Three sides and three angles; give three of
 * them, at least one a side, and the rest follows.
 *
 * Angles are radians inside this module.  The screens convert to and from
 * degrees; nothing here reads the angle setting.
 *
 * Fill a tri_t with what is known (has_side / has_angle), then call tri_solve().
 * The awkward case is SSA - two sides and an angle not between them - which can
 * have no triangle, one, or two.  tri_solve() returns the number of solutions
 * and fills `alt` with the second one.
 */

#ifndef BENCH_TRIANGLE_H
#define BENCH_TRIANGLE_H

#include <stdbool.h>

typedef struct {
    double side[3];      /* a, b, c - opposite A, B, C */
    double angle[3];     /* A, B, C in radians */
    bool has_side[3];
    bool has_angle[3];
} tri_t;

#define TRI_OK 0
#define TRI_ERR_ARG (-1)        /* a side or an angle that cannot be one */
#define TRI_ERR_SHAPE (-2)      /* sides that cannot close, angles past pi */
#define TRI_ERR_SINE (-3)       /* the law of sines has no solution */
#define TRI_ERR_DONE (-4)       /* not enough given, or too much */

void tri_clear(tri_t *t);

/* Degrees <-> radians, for the screens and for reading test vectors. */
double tri_deg(double rad);
double tri_rad(double deg);

/* Solve what `given` holds.  Returns 1 or 2 (the number of solutions), or one
 * of the errors above.  `out` is always filled on success; `alt` only when the
 * result is 2.  Both outputs may be NULL. */
int tri_solve(tri_t const *given, tri_t *out, tri_t *alt);

double tri_area(tri_t const *t);       /* 0 when the triangle is not complete */
double tri_perimeter(tri_t const *t);
bool tri_complete(tri_t const *t);

#endif /* BENCH_TRIANGLE_H */
