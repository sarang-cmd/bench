/* triangle.c - see triangle.h. */

#include "triangle.h"

#include <stddef.h>

#include "mathfn.h"

#define TRI_EPS 1e-9

static bool finite_pos(double x)
{
    return x > 0.0 && x < 1e12;
}

static double clamp1(double x)
{
    if (x > 1.0)
        return 1.0;
    if (x < -1.0)
        return -1.0;
    return x;
}

void tri_clear(tri_t *t)
{
    for (int i = 0; i < 3; i++)
    {
        t->side[i] = 0.0;
        t->angle[i] = 0.0;
        t->has_side[i] = false;
        t->has_angle[i] = false;
    }
}

double tri_deg(double rad)
{
    return bm_rad_to_deg(rad);
}

double tri_rad(double deg)
{
    return bm_deg_to_rad(deg);
}

bool tri_complete(tri_t const *t)
{
    for (int i = 0; i < 3; i++)
        if (!t->has_side[i] || !t->has_angle[i])
            return false;
    return true;
}

double tri_perimeter(tri_t const *t)
{
    double p = 0.0;

    if (!tri_complete(t))
        return 0.0;
    for (int i = 0; i < 3; i++)
        p += t->side[i];
    return p;
}

double tri_area(tri_t const *t)
{
    if (!tri_complete(t))
        return 0.0;
    /* Two sides and the angle between them: a = side[1], b = side[2], and the
     * angle opposite side[0] is the one between them. */
    return 0.5 * t->side[1] * t->side[2] * bm_sin(t->angle[0]);
}

/* ---- SSS ---------------------------------------------------------------- */

static int solve_sss_arr(double const s[3], tri_t *out)
{
    double const a = s[0], b = s[1], c = s[2];

    tri_clear(out);
    if (a + b <= c + TRI_EPS || b + c <= a + TRI_EPS || c + a <= b + TRI_EPS)
        return TRI_ERR_SHAPE;

    for (int i = 0; i < 3; i++)
    {
        out->side[i] = s[i];
        out->has_side[i] = true;
    }

    out->angle[0] = bm_acos(clamp1((b * b + c * c - a * a) / (2.0 * b * c)));
    out->angle[1] = bm_acos(clamp1((a * a + c * c - b * b) / (2.0 * a * c)));
    out->angle[2] = BM_PI - out->angle[0] - out->angle[1];
    for (int i = 0; i < 3; i++)
        out->has_angle[i] = true;
    return 1;
}

/* ---- one side and the rest of the angles -------------------------------- */

static int solve_aas(tri_t const *g, tri_t *out)
{
    int known = -1, ia = -1, ib = -1;

    for (int i = 0; i < 3; i++)
    {
        if (g->has_side[i] && known < 0)
            known = i;
        else if (g->has_side[i])
            return TRI_ERR_DONE;
        if (g->has_angle[i])
        {
            if (ia < 0)
                ia = i;
            else if (ib < 0)
                ib = i;
            else
                return TRI_ERR_DONE;
        }
    }
    if (known < 0 || ia < 0 || ib < 0)
        return TRI_ERR_DONE;

    *out = *g;
    {
        int const ic = 3 - ia - ib;

        out->angle[ic] = BM_PI - out->angle[ia] - out->angle[ib];
        if (out->angle[ic] <= TRI_EPS)
            return TRI_ERR_SHAPE;
        out->has_angle[ic] = true;
    }
    if (out->angle[known] <= TRI_EPS)
        return TRI_ERR_ARG;      /* the side is opposite a missing angle */

    {
        double const scale = out->side[known] / bm_sin(out->angle[known]);

        for (int i = 0; i < 3; i++)
        {
            if (out->has_side[i])
                continue;
            out->side[i] = scale * bm_sin(out->angle[i]);
            if (!finite_pos(out->side[i]))
                return TRI_ERR_SINE;
            out->has_side[i] = true;
        }
    }
    return 1;
}

/* ---- SAS: two sides and the angle between them -------------------------- */

/* vlo and vhi are the known sides at indices lo and hi; k is the missing side,
 * whose opposite angle is the one between them. */
static int solve_sas(double vlo, double vhi, int lo, int hi, int k,
                     double angle_k, tri_t *out)
{
    double s[3];
    double const third2 =
        vlo * vlo + vhi * vhi - 2.0 * vlo * vhi * bm_cos(angle_k);

    if (angle_k <= 0.0 || angle_k >= BM_PI)
        return TRI_ERR_ARG;
    if (third2 <= 0.0)
        return TRI_ERR_SHAPE;
    s[lo] = vlo;
    s[hi] = vhi;
    s[k] = bm_sqrt(third2);
    return solve_sss_arr(s, out);
}

/* ---- SSA: two sides and an angle opposite one of them ------------------- */

/* One solution: side a at index `ap`, side b at index `bp`, and the third side
 * at `kp`.  angle_b is the second candidate for the angle opposite b. */
static int ssa_with_angle(double a, int ap, double b, int bp, double angle_a,
                          double angle_b, tri_t *out)
{
    int const kp = 3 - ap - bp;

    tri_clear(out);
    out->side[ap] = a;
    out->side[bp] = b;
    out->has_side[ap] = true;
    out->has_side[bp] = true;
    out->angle[ap] = angle_a;
    out->angle[bp] = angle_b;
    out->has_angle[ap] = true;
    out->has_angle[bp] = true;

    out->angle[kp] = BM_PI - angle_a - angle_b;
    if (out->angle[kp] <= TRI_EPS)
        return TRI_ERR_SHAPE;
    out->has_angle[kp] = true;

    out->side[kp] = a * bm_sin(out->angle[kp]) / bm_sin(angle_a);
    if (!finite_pos(out->side[kp]))
        return TRI_ERR_SINE;
    out->has_side[kp] = true;
    return 1;
}

/* a is the side the given angle is opposite; b the other known side. */
static int solve_ssa(double a, int ap, double b, int bp, double angle_a,
                     tri_t *first, tri_t *alt)
{
    double sin_b;
    double b1;

    if (angle_a <= 0.0 || angle_a >= BM_PI)
        return TRI_ERR_ARG;
    if (ap == bp || ap < 0 || ap > 2 || bp < 0 || bp > 2)
        return TRI_ERR_ARG;
    sin_b = b * bm_sin(angle_a) / a;
    /* A hair over 1 is rounding, not an impossible triangle. */
    if (sin_b > 1.0 + 1e-9)
        return TRI_ERR_SINE;
    b1 = bm_asin(clamp1(sin_b));

    {
        int const rc1 = ssa_with_angle(a, ap, b, bp, angle_a, b1, first);
        double const b2 = BM_PI - b1;

        if (rc1 < 0)
            return rc1;
        /* The two candidates are the same triangle when the angle is 90
         * degrees: do not offer the same answer twice. */
        if (b2 - b1 > 1e-6 && angle_a + b2 < BM_PI - TRI_EPS)
        {
            tri_t cand;

            if (ssa_with_angle(a, ap, b, bp, angle_a, b2, &cand) > 0)
            {
                if (alt)
                    *alt = cand;
                return 2;
            }
        }
        return 1;
    }
}

/* ---- entry point -------------------------------------------------------- */

int tri_solve(tri_t const *g, tri_t *out, tri_t *alt)
{
    int ns = 0, na = 0;
    int si = -1, sj = -1;
    tri_t local;

    if (!g)
        return TRI_ERR_ARG;
    if (!out)
        out = &local;
    tri_clear(out);

    for (int i = 0; i < 3; i++)
    {
        if (g->has_side[i])
        {
            if (!finite_pos(g->side[i]))
                return TRI_ERR_ARG;
            ns++;
            sj = si;
            si = i;
        }
        if (g->has_angle[i])
        {
            if (g->angle[i] <= 0.0 || g->angle[i] >= BM_PI)
                return TRI_ERR_ARG;
            na++;
        }
    }

    if (ns == 3)
    {
        double const s[3] = { g->side[0], g->side[1], g->side[2] };

        return solve_sss_arr(s, out);
    }

    if (ns == 2 && na >= 1)
    {
        int const lo = si < sj ? si : sj;
        int const hi = si < sj ? sj : si;
        int const k = 3 - lo - hi;      /* the missing side, and its angle */
        int a;

        for (a = 0; a < 3; a++)
            if (g->has_angle[a])
                break;
        if (a == 3)
            return TRI_ERR_DONE;

        /* An angle opposite the missing side is the one between the two known
         * sides: SAS.  Any other makes it SSA, where the angle must be paired
         * with the side opposite it. */
        if (a == k)
            return solve_sas(g->side[lo], g->side[hi], lo, hi, k, g->angle[k],
                             out);
        if (a == lo)
            return solve_ssa(g->side[lo], lo, g->side[hi], hi, g->angle[lo],
                             out, alt);
        return solve_ssa(g->side[hi], hi, g->side[lo], lo, g->angle[hi], out,
                         alt);
    }

    if (ns == 1 && na >= 2)
        return solve_aas(g, out);

    return TRI_ERR_DONE;
}
