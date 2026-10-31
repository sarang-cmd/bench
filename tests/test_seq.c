/* test_seq.c - sequences, series and binomials (V2 F5). */

#include <stdint.h>

#include "seq.h"
#include "test_util.h"

static void test_arithmetic(void)
{
    T_SECTION("arithmetic sequences");

    /* 3, 7, 11, ...: d = 4, the 10th term is 39, the sum of ten is 210. */
    T_NEAR(seq_arith_nth(3.0, 4.0, 1), 3.0, 0.0);
    T_NEAR(seq_arith_nth(3.0, 4.0, 10), 39.0, 0.0);
    T_NEAR(seq_arith_sum(3.0, 4.0, 10), 210.0, 0.0);
    T_NEAR(seq_arith_sum(3.0, 4.0, 0), 0.0, 0.0);

    /* A decreasing one, and a fractional difference. */
    T_NEAR(seq_arith_nth(20.0, -2.5, 5), 10.0, 0.0);
    T_NEAR(seq_arith_sum(20.0, -2.5, 5), 75.0, 0.0);

    {
        double terms[4];

        seq_arith_terms(3.0, 4.0, 4, terms);
        T_NEAR(terms[0], 3.0, 0.0);
        T_NEAR(terms[3], 15.0, 0.0);
    }
}

static void test_geometric(void)
{
    T_SECTION("geometric sequences");

    /* 2, 6, 18, ...: r = 3, the 5th term is 162, the sum of five is 242. */
    T_NEAR(seq_geom_nth(2.0, 3.0, 5), 162.0, 1e-9);
    T_NEAR(seq_geom_sum(2.0, 3.0, 5), 242.0, 1e-9);

    /* 1, 1/2, 1/4, ... sums to 2. */
    {
        double s = 0.0;

        T_TRUE(seq_geom_infinite(1.0, 0.5, &s));
        T_NEAR(s, 2.0, 1e-12);
    }

    /* r = 1 is a flat sequence; |r| >= 1 has no sum to infinity. */
    T_NEAR(seq_geom_sum(5.0, 1.0, 7), 35.0, 1e-12);
    T_TRUE(!seq_geom_infinite(1.0, 1.0, 0));
    T_TRUE(!seq_geom_infinite(1.0, -2.0, 0));

    /* A shrinking one with a negative ratio. */
    T_NEAR(seq_geom_nth(8.0, -0.5, 4), -1.0, 1e-12);
    T_NEAR(seq_geom_sum(8.0, -0.5, 3), 6.0, 1e-12);

    {
        double terms[3];

        seq_geom_terms(1.0, 3.0, 3, terms);
        T_NEAR(terms[2], 9.0, 1e-12);
    }
}

static void test_binomial(void)
{
    T_SECTION("binomial coefficients");

    int64_t v = 0;

    T_TRUE(seq_binom(5, 2, &v));
    T_INT(v, 10);
    T_TRUE(seq_binom(10, 0, &v));
    T_INT(v, 1);
    T_TRUE(seq_binom(10, 10, &v));
    T_INT(v, 1);
    T_TRUE(seq_binom(52, 5, &v));
    T_INT(v, 2598960);           /* a poker hand */

    T_TRUE(!seq_binom(5, 6, &v));
    T_TRUE(!seq_binom(-1, 2, &v));
}

static void test_pascal(void)
{
    T_SECTION("Pascal's triangle");

    int64_t row[SEQ_PASCAL_MAX + 1];

    T_TRUE(seq_pascal_row(5, row, 16));
    T_INT(row[0], 1);
    T_INT(row[1], 5);
    T_INT(row[2], 10);
    T_INT(row[3], 10);
    T_INT(row[4], 5);
    T_INT(row[5], 1);

    T_TRUE(seq_pascal_row(0, row, 16));
    T_INT(row[0], 1);

    /* Row 9 is the one students memorise. */
    T_TRUE(seq_pascal_row(9, row, 16));
    T_INT(row[4], 126);

    T_TRUE(!seq_pascal_row(4, row, 3));     /* too small */
    T_TRUE(seq_pascal_row(62, row, 63));
}

static void run_tests(void)
{
    test_arithmetic();
    test_geometric();
    test_binomial();
    test_pascal();
}

T_MAIN("test_seq")
