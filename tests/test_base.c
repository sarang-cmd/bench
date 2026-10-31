/* test_base.c - bases and bits (V2 feature F12).
 *
 * Vectors are the ones on a paper: 255 is FF and 1111 1111, -1 at 8 bits is
 * 1111 1111, 5 AND 3 is 1, and 0b1010 shifted left by two is 40.
 */

#include "test_util.h"

#include "base.h"

#include <string.h>

static void test_parse(void)
{
    int64_t v;

    T_TRUE(base_parse("255", 10, &v));
    T_INT(v, 255);
    T_TRUE(base_parse("FF", 16, &v));
    T_INT(v, 255);
    T_TRUE(base_parse("ff", 16, &v));
    T_INT(v, 255);
    T_TRUE(base_parse("11111111", 2, &v));
    T_INT(v, 255);
    T_TRUE(base_parse("777", 8, &v));
    T_INT(v, 511);
    T_TRUE(base_parse("ZZ", 36, &v));
    T_INT(v, 35 * 36 + 35);
    T_TRUE(base_parse("-1010", 2, &v));
    T_INT(v, -10);

    T_FALSE(base_parse("", 10, &v));
    T_FALSE(base_parse("-", 10, &v));
    T_FALSE(base_parse("2", 2, &v));
    T_FALSE(base_parse("G", 16, &v));
    T_FALSE(base_parse("12", 1, &v));
    T_FALSE(base_parse("12", 37, &v));
    T_FALSE(base_parse("99999999999999999999", 10, &v));
}

static void test_format(void)
{
    char buf[80];

    T_TRUE(base_format(255, 16, buf, sizeof buf));
    T_STR(buf, "FF");
    T_TRUE(base_format(255, 2, buf, sizeof buf));
    T_STR(buf, "11111111");
    T_TRUE(base_format(255, 8, buf, sizeof buf));
    T_STR(buf, "377");
    T_TRUE(base_format(255, 10, buf, sizeof buf));
    T_STR(buf, "255");
    T_TRUE(base_format(-10, 2, buf, sizeof buf));
    T_STR(buf, "-1010");
    T_TRUE(base_format(0, 36, buf, sizeof buf));
    T_STR(buf, "0");
    T_TRUE(base_format(1295, 36, buf, sizeof buf));
    T_STR(buf, "ZZ");

    /* A two-character buffer cannot hold "FF" and its terminator. */
    T_FALSE(base_format(255, 16, buf, 2));
}

static void test_twos(void)
{
    char buf[80];

    T_TRUE(base_twos(-1, 8, buf, sizeof buf));
    T_STR(buf, "1111 1111");
    T_TRUE(base_twos(5, 8, buf, sizeof buf));
    T_STR(buf, "0000 0101");
    T_TRUE(base_twos(-128, 8, buf, sizeof buf));
    T_STR(buf, "1000 0000");

    T_TRUE(base_twos(-1, 16, buf, sizeof buf));
    T_STR(buf, "1111 1111 1111 1111");
    T_TRUE(base_twos(-1, 32, buf, sizeof buf));
    T_STR(buf, "1111 1111 1111 1111 1111 1111 1111 1111");

    T_TRUE(base_twos_hex(-1, 8, buf, sizeof buf));
    T_STR(buf, "FF");
    T_TRUE(base_twos_hex(-1, 16, buf, sizeof buf));
    T_STR(buf, "FFFF");

    T_FALSE(base_twos(1, 7, buf, sizeof buf));

    T_INT(base_twos_max(8), 127);
    T_INT(base_twos_max(16), 32767);
    T_INT(base_twos_max(32), 2147483647);
}

static void test_ops(void)
{
    int64_t r;

    T_TRUE(base_op(5, 3, BIT_AND, &r));
    T_INT(r, 1);
    T_TRUE(base_op(5, 3, BIT_OR, &r));
    T_INT(r, 7);
    T_TRUE(base_op(5, 3, BIT_XOR, &r));
    T_INT(r, 6);
    T_TRUE(base_op(10, 2, BIT_SHL, &r));
    T_INT(r, 40);
    T_TRUE(base_op(40, 2, BIT_SHR, &r));
    T_INT(r, 10);

    /* A logical shift, not an arithmetic one: the top bit arrives as zero. */
    T_TRUE(base_op(-1, 1, BIT_SHR, &r));
    T_TRUE(r > 0);

    T_FALSE(base_op(1, 64, BIT_SHL, &r));
    T_FALSE(base_op(1, -1, BIT_SHR, &r));

    T_INT(base_popcount(255), 8);
    T_INT(base_popcount(0), 0);
    T_INT(base_popcount(0xF0F0), 8);

    T_STR(bit_op_name(BIT_XOR), "XOR");
    T_STR(bit_op_name(BIT_SHL), "<<");
}

static void test_digits_and_valid(void)
{
    T_STR(base_digits(2), "01");
    T_STR(base_digits(16), "0123456789ABCDEF");
    T_TRUE(base_valid("1010", 2));
    T_FALSE(base_valid("1012", 2));
    T_TRUE(base_valid("-7", 8));
}

static void run_tests(void)
{
    T_SECTION("parsing in any base");
    test_parse();
    T_SECTION("printing in any base");
    test_format();
    T_SECTION("two's complement");
    test_twos();
    T_SECTION("bit operations");
    test_ops();
    T_SECTION("validity and the digit list");
    test_digits_and_valid();
}

T_MAIN("test_base")
