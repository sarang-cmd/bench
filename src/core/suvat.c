/* suvat.c - constant acceleration and projectiles.  See suvat.h.
 *
 * The solver is a table over the ten ways of choosing three of the five
 * quantities.  Each case names the rearranged equation it used, so the screen
 * can show the working rather than just the number.  A case table beats a
 * cascade of "if I know these three, work out that one" rules: every triple has
 * exactly one entry, nothing fires twice, and a test can walk all ten.
 */

#include "suvat.h"

#include "mathfn.h"

#include <stdio.h>
#include <string.h>

char const *suvat_name(su_var_t var)
{
    switch (var) {
    case SU_S: return "s";
    case SU_U: return "u";
    case SU_V: return "v";
    case SU_A: return "a";
    case SU_T: return "t";
    default:   return "?";
    }
}

char const *suvat_unit(su_var_t var)
{
    switch (var) {
    case SU_S: return "m";
    case SU_U: return "m/s";
    case SU_V: return "m/s";
    case SU_A: return "m/s^2";
    case SU_T: return "s";
    default:   return "";
    }
}

/* Small enough for the numbers this screen takes, large enough that 0.1 + 0.2
 * style noise does not look like a division by nothing. */
static bool nearly(double x) { return x > -1e-12 && x < 1e-12; }

static void note(suvat_t *x, char const *text)
{
    if (x->eq_count < 3) {
        snprintf(x->eq[x->eq_count], sizeof x->eq[0], "%s", text);
        x->eq_count++;
    }
}

static void give(suvat_t *x, su_var_t var, double value)
{
    x->v[var] = value;
    x->known[var] = true;
    if (x->solved_count < 2) x->solved[x->solved_count++] = var;
}

#define S (x->v[SU_S])
#define U (x->v[SU_U])
#define V (x->v[SU_V])
#define A (x->v[SU_A])
#define T (x->v[SU_T])

/* s = u t + a t^2 / 2 as a quadratic in t, given u, a and s.  Returns false if
 * there is no positive root.  Keeps the smaller positive root and mentions the
 * other one when both are positive: that is the throw-and-return case. */
static bool time_from_usa(suvat_t *x)
{
    if (nearly(A)) {
        /* Constant speed: one root, and no choice to make. */
        if (nearly(U)) return false;
        double t = S / U;
        if (!(t > 0)) return false;
        give(x, SU_T, t);
        return true;
    }
    double disc = U * U + 2.0 * A * S;
    if (disc < 0) return false;
    double root = bm_sqrt(disc);
    double t1 = (-U + root) / A;
    double t2 = (-U - root) / A;
    bool ok1 = t1 > 0, ok2 = t2 > 0;
    if (!ok1 && !ok2) return false;
    double pick = ok1 ? t1 : t2;
    if (ok1 && ok2 && !nearly(t1 - t2)) {
        double other = (pick == t1) ? t2 : t1;
        if (other < pick) {
            x->other_time = pick;
            pick = other;
        } else {
            x->other_time = other;
        }
        x->two_times = true;
    }
    give(x, SU_T, pick);
    return true;
}

/* The sign of the root: a body that starts moving forwards keeps the positive
 * one, so the screen does not claim it bounces back on its own. */
static double root_with_sign(double sq, double sign_source)
{
    double r = bm_sqrt(sq);
    return sign_source < 0 ? -r : r;
}

bool suvat_solve(suvat_t *x, char *msg, int size)
{
    int given = 0;
    unsigned mask = 0;
    for (int i = 0; i < SU_COUNT; i++) {
        if (x->known[i]) {
            given++;
            mask |= 1u << i;
        }
    }
    x->solved_count = 0;
    x->eq_count = 0;
    x->two_times = false;
    x->other_time = 0;

    if (given < 3) {
        snprintf(msg, size, "need three of s u v a t, have %d", given);
        return false;
    }

    /* A fourth known value is not an error: it just means the student already
     * filled it in.  Solve from the first three by the table below. */
    switch (mask & 0x1Fu) {
    case (1u << SU_S) | (1u << SU_U) | (1u << SU_V):
        if (nearly(S)) { snprintf(msg, size, "s is zero: a is undefined"); return false; }
        if (nearly(U + V)) { snprintf(msg, size, "u + v is zero: t is undefined"); return false; }
        note(x, "a = (v^2 - u^2) / 2s");
        give(x, SU_A, (V * V - U * U) / (2.0 * S));
        note(x, "t = 2s / (u + v)");
        give(x, SU_T, 2.0 * S / (U + V));
        break;

    case (1u << SU_S) | (1u << SU_U) | (1u << SU_A): {
        double sq = U * U + 2.0 * A * S;
        if (sq < 0) { snprintf(msg, size, "u^2 + 2 a s is negative: no real v"); return false; }
        note(x, "v = sqrt(u^2 + 2 a s)");
        give(x, SU_V, root_with_sign(sq, U));
        if (!time_from_usa(x)) {
            snprintf(msg, size, "the quadratic for t has no positive root");
            return false;
        }
        note(x, "s = u t + a t^2 / 2");
        break;
    }

    case (1u << SU_S) | (1u << SU_U) | (1u << SU_T):
        if (nearly(T)) { snprintf(msg, size, "t is zero: a is undefined"); return false; }
        note(x, "a = 2(s - u t) / t^2");
        give(x, SU_A, 2.0 * (S - U * T) / (T * T));
        note(x, "v = u + a t");
        give(x, SU_V, U + A * T);
        break;

    case (1u << SU_S) | (1u << SU_V) | (1u << SU_A): {
        double sq = V * V - 2.0 * A * S;
        if (sq < 0) { snprintf(msg, size, "v^2 - 2 a s is negative: no real u"); return false; }
        note(x, "u = sqrt(v^2 - 2 a s)");
        give(x, SU_U, root_with_sign(sq, V));
        if (nearly(A)) { snprintf(msg, size, "a is zero: t is undefined"); return false; }
        note(x, "t = (v - u) / a");
        give(x, SU_T, (V - U) / A);
        break;
    }

    case (1u << SU_S) | (1u << SU_V) | (1u << SU_T):
        if (nearly(T)) { snprintf(msg, size, "t is zero: u is undefined"); return false; }
        note(x, "u = 2s / t - v");
        give(x, SU_U, 2.0 * S / T - V);
        note(x, "a = (v - u) / t");
        give(x, SU_A, (V - U) / T);
        break;

    case (1u << SU_S) | (1u << SU_A) | (1u << SU_T):
        if (nearly(T)) { snprintf(msg, size, "t is zero: u is undefined"); return false; }
        note(x, "u = (s - a t^2 / 2) / t");
        give(x, SU_U, (S - 0.5 * A * T * T) / T);
        note(x, "v = u + a t");
        give(x, SU_V, U + A * T);
        break;

    case (1u << SU_U) | (1u << SU_V) | (1u << SU_A):
        if (nearly(A)) { snprintf(msg, size, "a is zero: t is undefined"); return false; }
        note(x, "t = (v - u) / a");
        give(x, SU_T, (V - U) / A);
        note(x, "s = (u + v) t / 2");
        give(x, SU_S, (U + V) * 0.5 * T);
        break;

    case (1u << SU_U) | (1u << SU_V) | (1u << SU_T):
        if (nearly(T)) { snprintf(msg, size, "t is zero: a is undefined"); return false; }
        note(x, "a = (v - u) / t");
        give(x, SU_A, (V - U) / T);
        note(x, "s = (u + v) t / 2");
        give(x, SU_S, (U + V) * 0.5 * T);
        break;

    case (1u << SU_U) | (1u << SU_A) | (1u << SU_T):
        note(x, "v = u + a t");
        give(x, SU_V, U + A * T);
        note(x, "s = u t + a t^2 / 2");
        give(x, SU_S, U * T + 0.5 * A * T * T);
        break;

    case (1u << SU_V) | (1u << SU_A) | (1u << SU_T):
        note(x, "u = v - a t");
        give(x, SU_U, V - A * T);
        note(x, "s = (u + v) t / 2");
        give(x, SU_S, (U + V) * 0.5 * T);
        break;

    default:
        snprintf(msg, size, "need three of s u v a t, have %d", given);
        return false;
    }

    for (int i = 0; i < SU_COUNT; i++) {
        if (!bm_isfinite(x->v[i])) {
            snprintf(msg, size, "%s came out undefined", suvat_name((su_var_t)i));
            return false;
        }
    }
    msg[0] = 0;
    return true;
}

bool projectile(double speed, double angle_deg, double height, double g,
                projectile_t *out)
{
    if (!(g > 0) || !(speed >= 0) || !(height >= 0)) return false;
    if (!bm_isfinite(speed) || !bm_isfinite(angle_deg) || !bm_isfinite(height))
        return false;

    double rad = angle_deg * BM_PI / 180.0;
    double s, c;
    bm_sincos(rad, &s, &c);
    out->vx = speed * c;
    out->vy = speed * s;

    /* Height above the ground is y(t) = h + vy t - g t^2 / 2; we want the
     * positive root of y = 0. */
    double a = -0.5 * g, b = out->vy, cc = height;
    double disc = b * b - 4.0 * a * cc;
    if (disc < 0) return false;
    double t = (-b - bm_sqrt(disc)) / (2.0 * a);
    if (!(t > 0)) t = (-b + bm_sqrt(disc)) / (2.0 * a);
    if (!(t > 0)) return false;

    out->flight = t;
    out->range = out->vx * t;
    out->peak = height + (out->vy > 0 ? out->vy * out->vy / (2.0 * g) : 0);
    return true;
}

#undef S
#undef U
#undef V
#undef A
#undef T
