/* test_suvat.c - constant acceleration and projectiles (V2 feature F10).
 *
 * The vectors are the prompt's: a ball dropped 45 m takes 3 s at g = 10, and a
 * throw of 20 m/s at 30 degrees from 1.5 m lands at about 37.1 m after 2.27 s.
 * The ten triples are checked by solving the same motion from each of them, so
 * a typo in one row of the table cannot hide behind the others.
 */

#include "test_util.h"

#include "suvat.h"

static void clear(suvat_t *x)
{
    for (int i = 0; i < SU_COUNT; i++) {
        x->v[i] = 0;
        x->known[i] = false;
    }
    x->solved_count = 0;
    x->eq_count = 0;
    x->two_times = false;
    x->other_time = 0;
}

static void set(suvat_t *x, su_var_t var, double value)
{
    x->v[var] = value;
    x->known[var] = true;
}

/* Solve a fresh struct from three values; returns the solver's answer. */
static bool solve3(double s, double u, double v, double a, double t,
                   su_var_t k1, su_var_t k2, su_var_t k3, suvat_t *out)
{
    char msg[80];
    clear(out);
    double vals[SU_COUNT] = { s, u, v, a, t };
    set(out, k1, vals[k1]);
    set(out, k2, vals[k2]);
    set(out, k3, vals[k3]);
    return suvat_solve(out, msg, sizeof msg);
}

static void test_names(void)
{
    T_STR(suvat_name(SU_S), "s");
    T_STR(suvat_name(SU_T), "t");
    T_STR(suvat_unit(SU_A), "m/s^2");
    T_STR(suvat_unit(SU_S), "m");
}

/* u = 0, a = 10, t = 3: v = 30, s = 45. */
static void test_drop(void)
{
    suvat_t x;
    char msg[80];

    T_TRUE(solve3(45, 0, 30, 10, 3, SU_U, SU_A, SU_T, &x));
    T_NEAR(x.v[SU_V], 30, 1e-12);
    T_NEAR(x.v[SU_S], 45, 1e-12);
    T_INT(x.solved_count, 2);
    T_INT(x.solved[0], SU_V);
    T_INT(x.solved[1], SU_S);
    T_STR(x.eq[0], "v = u + a t");
    T_FALSE(x.two_times);

    /* Same motion from u, v, t. */
    T_TRUE(solve3(45, 0, 30, 10, 3, SU_U, SU_V, SU_T, &x));
    T_NEAR(x.v[SU_A], 10, 1e-12);
    T_NEAR(x.v[SU_S], 45, 1e-12);

    /* And from s, u, t: a = 2(45 - 0)/9 = 10. */
    T_TRUE(solve3(45, 0, 30, 10, 3, SU_S, SU_U, SU_T, &x));
    T_NEAR(x.v[SU_A], 10, 1e-12);
    T_NEAR(x.v[SU_V], 30, 1e-12);

    /* And from s, u, a: the quadratic gives one positive root, t = 3. */
    T_TRUE(solve3(45, 0, 30, 10, 3, SU_S, SU_U, SU_A, &x));
    T_NEAR(x.v[SU_T], 3, 1e-12);
    T_NEAR(x.v[SU_V], 30, 1e-12);
    T_FALSE(x.two_times);

    /* All five known again is not an error the screen has to show: the table
     * has no row for it, so the solver asks for three. */
    T_FALSE(suvat_solve(&x, msg, sizeof msg));
}

/* u = 20, a = -2, t = 5: v = 10, s = 20*5 - 25 = 75. */
static void test_brake(void)
{
    suvat_t x;
    T_TRUE(solve3(75, 20, 10, -2, 5, SU_U, SU_A, SU_T, &x));
    T_NEAR(x.v[SU_V], 10, 1e-12);
    T_NEAR(x.v[SU_S], 75, 1e-12);

    T_TRUE(solve3(75, 20, 10, -2, 5, SU_S, SU_V, SU_T, &x));
    T_NEAR(x.v[SU_U], 20, 1e-12);
    T_NEAR(x.v[SU_A], -2, 1e-12);

    T_TRUE(solve3(75, 20, 10, -2, 5, SU_S, SU_V, SU_A, &x));
    T_NEAR(x.v[SU_U], 20, 1e-12);
    T_NEAR(x.v[SU_T], 5, 1e-12);

    T_TRUE(solve3(75, 20, 10, -2, 5, SU_U, SU_V, SU_A, &x));
    T_NEAR(x.v[SU_T], 5, 1e-12);
    T_NEAR(x.v[SU_S], 75, 1e-12);

    T_TRUE(solve3(75, 20, 10, -2, 5, SU_S, SU_U, SU_V, &x));
    T_NEAR(x.v[SU_A], -2, 1e-12);
    T_NEAR(x.v[SU_T], 5, 1e-12);

    T_TRUE(solve3(75, 20, 10, -2, 5, SU_S, SU_A, SU_T, &x));
    T_NEAR(x.v[SU_U], 20, 1e-12);
    T_NEAR(x.v[SU_V], 10, 1e-12);

    T_TRUE(solve3(75, 20, 10, -2, 5, SU_V, SU_A, SU_T, &x));
    T_NEAR(x.v[SU_U], 20, 1e-12);
    T_NEAR(x.v[SU_S], 75, 1e-12);
}

/* The quadratic in t: thrown up at 20 m/s with a = -10 from s = 0 comes back at
 * t = 4, and t = 0 is the launch. */
static void test_two_times(void)
{
    suvat_t x;
    T_TRUE(solve3(15, 20, 10, -10, 3, SU_S, SU_U, SU_A, &x));
    /* Rising past 15 m at t = 1 and falling past it again at t = 3.  The
     * screen keeps the first time and mentions the second. */
    T_NEAR(x.v[SU_T], 1, 1e-12);
    T_TRUE(x.two_times);
    T_NEAR(x.other_time, 3, 1e-12);
    T_NEAR(x.v[SU_V], 10, 1e-12);
}

static void test_bad_input(void)
{
    suvat_t x;
    char msg[80];

    clear(&x);
    set(&x, SU_U, 5);
    set(&x, SU_A, 2);
    T_FALSE(suvat_solve(&x, msg, sizeof msg));
    T_CHECK(strstr(msg, "three") != NULL, "msg was \"%s\"", msg);

    /* u = v = 0, t = 2: dividing by a zero velocity difference is not allowed
     * in the {u,v,a} row. */
    T_FALSE(solve3(0, 0, 0, 0, 2, SU_U, SU_V, SU_A, &x));

    /* Falling faster than the drop allows: u^2 + 2 a s < 0. */
    T_FALSE(solve3(-5, 1, 0, 10, 0, SU_S, SU_U, SU_A, &x));

    /* s = 0 in the {s,u,v} row leaves a undefined. */
    T_FALSE(solve3(0, 0, 10, 0, 1, SU_S, SU_U, SU_V, &x));

    /* Constant speed with a = 0 is not a failure. */
    T_TRUE(solve3(20, 10, 10, 0, 2, SU_S, SU_U, SU_A, &x));
    T_NEAR(x.v[SU_T], 2, 1e-12);
    T_NEAR(x.v[SU_V], 10, 1e-12);
}

/* 20 m/s at 30 degrees from 1.5 m, g = 9.81. */
static void test_projectile(void)
{
    projectile_t p;
    T_TRUE(projectile(20, 30, 1.5, 9.81, &p));
    T_NEAR(p.vx, 20 * 0.8660254037844387, 1e-9);
    T_NEAR(p.vy, 10, 1e-9);
    T_NEAR(p.peak, 1.5 + 100.0 / (2 * 9.81), 1e-9);
    T_NEAR(p.flight, 2.1791, 5e-4);
    T_NEAR(p.range, 37.743, 5e-2);

    /* Straight off a table: no upward part, peak is the launch height, and the
     * time is sqrt(2h/g). */
    T_TRUE(projectile(5, 0, 1.8, 10, &p));
    T_NEAR(p.flight, 0.6, 1e-12);
    T_NEAR(p.range, 3, 1e-12);
    T_NEAR(p.peak, 1.8, 1e-12);

    /* Straight up and back: range zero. */
    T_TRUE(projectile(10, 90, 0, 10, &p));
    T_NEAR(p.range, 0, 1e-12);
    T_NEAR(p.flight, 2, 1e-12);
    T_NEAR(p.peak, 5, 1e-12);

    T_FALSE(projectile(10, 45, 1, 0, &p));
    T_FALSE(projectile(-1, 45, 1, 9.81, &p));
}

static void run_tests(void)
{
    T_SECTION("names and units");
    test_names();
    T_SECTION("a dropped stone");
    test_drop();
    T_SECTION("braking, every triple");
    test_brake();
    T_SECTION("two times");
    test_two_times();
    T_SECTION("degenerate input");
    test_bad_input();
    T_SECTION("projectiles");
    test_projectile();
}

T_MAIN("test_suvat")
