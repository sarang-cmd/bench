/* test_triangle.c - the triangle solver (V2 F2).
 *
 * The vectors are the ones every textbook uses: the 3-4-5 right triangle, the
 * SAS case with a 60 degree included angle, an ASA pair, and the two SSA
 * shapes - the ambiguous one and the impossible one. */

#include "test_util.h"
#include "triangle.h"

static tri_t given(void)
{
    tri_t t;

    tri_clear(&t);
    return t;
}

static void test_sss(void)
{
    tri_t g = given(), out;

    T_SECTION("three sides: 3, 4, 5");

    g.has_side[0] = g.has_side[1] = g.has_side[2] = true;
    g.side[0] = 3.0;
    g.side[1] = 4.0;
    g.side[2] = 5.0;

    T_INT(tri_solve(&g, &out, 0), 1);
    T_TRUE(tri_complete(&out));

    /* The angle opposite the longest side is the right angle. */
    T_NEAR(tri_deg(out.angle[2]), 90.0, 1e-6);
    T_NEAR(tri_deg(out.angle[0]), 36.86989765, 1e-6);
    T_NEAR(tri_deg(out.angle[1]), 53.13010235, 1e-6);
    T_NEAR(tri_area(&out), 6.0, 1e-9);
    T_NEAR(tri_perimeter(&out), 12.0, 1e-12);
}

static void test_sss_bad(void)
{
    tri_t g = given(), out;

    T_SECTION("sides that cannot close");

    g.has_side[0] = g.has_side[1] = g.has_side[2] = true;
    g.side[0] = 1.0;
    g.side[1] = 2.0;
    g.side[2] = 10.0;
    T_INT(tri_solve(&g, &out, 0), TRI_ERR_SHAPE);

    /* A degenerate triangle is refused, a near-degenerate one is not. */
    g.side[2] = 3.0;
    T_INT(tri_solve(&g, &out, 0), TRI_ERR_SHAPE);
    g.side[2] = 2.9;
    T_INT(tri_solve(&g, &out, 0), 1);

    g.side[0] = 0.0;
    T_INT(tri_solve(&g, &out, 0), TRI_ERR_ARG);
}

static void test_sas(void)
{
    tri_t g = given(), out;

    T_SECTION("two sides and the angle between them");

    /* a = 5, b = 8, C = 60 degrees: c^2 = 25 + 64 - 80*0.5 = 49. */
    g.has_side[0] = g.has_side[1] = true;
    g.side[0] = 5.0;
    g.side[1] = 8.0;
    g.has_angle[2] = true;
    g.angle[2] = tri_rad(60.0);

    T_INT(tri_solve(&g, &out, 0), 1);
    T_NEAR(out.side[2], 7.0, 1e-9);
    T_NEAR(tri_deg(out.angle[0]) + tri_deg(out.angle[1]) + 60.0, 180.0, 1e-6);
    T_NEAR(tri_area(&out), 0.5 * 5.0 * 8.0 * 0.86602540378, 1e-9);

    /* The angle is opposite the missing side even when the sides are b and c. */
    {
        tri_t g2 = given(), out2;

        g2.has_side[1] = g2.has_side[2] = true;
        g2.side[1] = 4.0;
        g2.side[2] = 5.0;
        g2.has_angle[0] = true;
        g2.angle[0] = tri_rad(90.0);
        T_INT(tri_solve(&g2, &out2, 0), 1);
        T_NEAR(out2.side[0], 6.4031242374, 1e-9);
    }
}

static void test_aas(void)
{
    tri_t g = given(), out;

    T_SECTION("two angles and one side");

    /* A = 50, C = 70, b = 10: B = 60, a = 10 sin50/sin60, c = 10 sin70/sin60. */
    g.has_side[1] = true;
    g.side[1] = 10.0;
    g.has_angle[0] = true;
    g.angle[0] = tri_rad(50.0);
    g.has_angle[2] = true;
    g.angle[2] = tri_rad(70.0);

    T_INT(tri_solve(&g, &out, 0), 1);
    T_NEAR(tri_deg(out.angle[1]), 60.0, 1e-9);
    T_NEAR(out.side[0], 10.0 * 0.7660444431 / 0.8660254038, 1e-9);
    T_NEAR(out.side[2], 10.0 * 0.9396926208 / 0.8660254038, 1e-9);

    /* Angles that already sum past 180 are refused. */
    g.angle[0] = tri_rad(120.0);
    g.angle[2] = tri_rad(90.0);
    T_INT(tri_solve(&g, &out, 0), TRI_ERR_SHAPE);
}

static void test_ssa(void)
{
    tri_t g = given(), out, alt;

    T_SECTION("two sides and an angle not between them");

    /* a = 10, b = 15, A = 30: sinB = 15*0.5/10 = 0.75, so B = 48.59 or
     * 131.41 - two triangles. */
    g.has_side[0] = g.has_side[1] = true;
    g.side[0] = 10.0;
    g.side[1] = 15.0;
    g.has_angle[0] = true;
    g.angle[0] = tri_rad(30.0);

    T_INT(tri_solve(&g, &out, &alt), 2);
    T_NEAR(tri_deg(out.angle[1]), 48.5903778, 1e-6);
    T_NEAR(tri_deg(alt.angle[1]), 131.4096222, 1e-6);
    T_NEAR(tri_deg(out.angle[2]), 101.4096222, 1e-6);
    T_NEAR(tri_deg(alt.angle[2]), 18.5903778, 1e-6);
    T_TRUE(tri_area(&alt) > 0.0);
    T_TRUE(tri_area(&out) > tri_area(&alt));

    /* The same shape with the angle opposite the longer side: one triangle. */
    g.side[0] = 15.0;
    g.side[1] = 10.0;
    T_INT(tri_solve(&g, &out, &alt), 1);
    T_NEAR(tri_deg(out.angle[1]), 19.4712206, 1e-6);

    /* No triangle at all: sinB would have to be 1.73. */
    g.side[0] = 5.0;
    g.side[1] = 10.0;
    g.angle[0] = tri_rad(60.0);
    T_INT(tri_solve(&g, &out, &alt), TRI_ERR_SINE);

    /* The right-angle boundary: b = a/sinA = 20 gives sinB = 1 exactly, so
     * B = 90 and there is one triangle, not two. */
    g.side[0] = 10.0;
    g.side[1] = 20.0;
    g.angle[0] = tri_rad(30.0);
    T_INT(tri_solve(&g, &out, &alt), 1);
    /* Near a right angle the sine is flat, so the recovered angle is
     * ill-conditioned: a 1e-16 error in sinB is a 1e-6 degree error in B. */
    T_NEAR(tri_deg(out.angle[1]), 90.0, 1e-4);
}

static void test_given_enough(void)
{
    tri_t g = given(), out;

    T_SECTION("not enough, or too much");

    T_INT(tri_solve(&g, &out, 0), TRI_ERR_DONE);

    g.has_side[0] = true;
    g.side[0] = 5.0;
    T_INT(tri_solve(&g, &out, 0), TRI_ERR_DONE);

    g.has_angle[0] = true;
    g.angle[0] = tri_rad(60.0);
    T_INT(tri_solve(&g, &out, 0), TRI_ERR_DONE);

    /* One side and two angles is enough. */
    g.has_angle[1] = true;
    g.angle[1] = tri_rad(60.0);
    T_INT(tri_solve(&g, &out, 0), 1);

    /* Three angles and no side is not. */
    {
        tri_t h = given();

        h.has_angle[0] = h.has_angle[1] = h.has_angle[2] = true;
        h.angle[0] = tri_rad(60.0);
        h.angle[1] = tri_rad(60.0);
        h.angle[2] = tri_rad(60.0);
        T_INT(tri_solve(&h, &out, 0), TRI_ERR_DONE);
    }
}

static void test_degrees(void)
{
    T_SECTION("angle conversion");

    T_NEAR(tri_rad(180.0), 3.14159265358979, 1e-12);
    T_NEAR(tri_deg(1.5707963267949), 90.0, 1e-12);
    T_NEAR(tri_deg(tri_rad(37.0)), 37.0, 1e-12);
}

static void run_tests(void)
{
    test_sss();
    test_sss_bad();
    test_sas();
    test_aas();
    test_ssa();
    test_given_enough();
    test_degrees();
}

T_MAIN("test_triangle")
