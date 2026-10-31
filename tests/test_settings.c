/* test_settings.c - native tests for src/core/settings.c. */

#include "../src/core/settings.h"
#include "test_util.h"

#include <string.h>

static void test_defaults(void)
{
    bench_settings_t s;

    T_SECTION("defaults");
    settings_defaults(&s);
    T_INT(s.uncert_sig, 1);
    T_INT(s.pacer_checkpoint, 10);
    T_INT(s.notes_wrap, 0);

    /* NULL is tolerated everywhere. */
    settings_defaults(NULL);
}

static void test_format_round_trip(void)
{
    bench_settings_t s;
    bench_settings_t out;
    char buf[SETTINGS_BUFSIZE];
    int ignored = -1;
    int n;

    T_SECTION("format and parse round trip");
    settings_defaults(&s);
    n = settings_format(&s, buf, sizeof buf);
    T_TRUE(n > 0);
    T_STR(buf, "# Bench settings v1\nuncert_sig 1\npacer_checkpoint 10\n"
               "notes_wrap 0\n");

    settings_defaults(&out);
    out.uncert_sig = 2;
    T_TRUE(settings_parse(buf, n, &out, &ignored) == 3);
    T_INT(ignored, 0);
    T_INT(out.uncert_sig, 1);
    T_TRUE(settings_equal(&s, &out));

    /* Non-default values survive a round trip. */
    s.uncert_sig = 2;
    s.pacer_checkpoint = 25;
    s.notes_wrap = 75;
    n = settings_format(&s, buf, sizeof buf);
    T_TRUE(n > 0);
    settings_defaults(&out);
    T_TRUE(settings_parse(buf, n, &out, NULL) == 3);
    T_TRUE(settings_equal(&s, &out));

    /* Every checkpoint value in range formats and parses back. */
    for (int c = 1; c <= 200; c++)
    {
        s.pacer_checkpoint = (uint8_t)c;
        n = settings_format(&s, buf, sizeof buf);
        T_TRUE(n > 0);
        settings_defaults(&out);
        T_TRUE(settings_parse(buf, n, &out, NULL) > 0);
        T_INT(out.pacer_checkpoint, c);
    }
}

static void test_format_small_buffer(void)
{
    bench_settings_t s;
    char tiny[8];

    T_SECTION("format refuses a small buffer");
    settings_defaults(&s);
    T_INT(settings_format(&s, tiny, (int)sizeof tiny), -1);
    T_INT(settings_format(&s, tiny, 0), -1);
    T_INT(settings_format(NULL, tiny, (int)sizeof tiny), -1);
}

static void test_parse_tolerance(void)
{
    bench_settings_t s;
    int ignored = 0;

    T_SECTION("parse tolerates a hand-edited file");
    settings_defaults(&s);
    char const *text = "# comment\n"
                       "\n"
                       "   \n"
                       "uncert_sig 2\n"
                       "pacer_checkpoint 20\n"
                       "notes_wrap 0\n";
    T_TRUE(settings_parse(text, (int)strlen(text), &s, &ignored) == 3);
    T_INT(ignored, 0);
    T_INT(s.uncert_sig, 2);
    T_INT(s.pacer_checkpoint, 20);

    /* Unknown keys and junk values are counted, not fatal. */
    settings_defaults(&s);
    char const *bad = "banana 3\nuncert_sig x\npacer_checkpoint\n"
                      "notes_wrap 12 34\n";
    T_INT(settings_parse(bad, (int)strlen(bad), &s, &ignored), 0);
    T_INT(ignored, 4);
    T_INT(s.uncert_sig, 1);

    /* Out-of-range values are clamped. */
    settings_defaults(&s);
    char const *wide = "uncert_sig 9\npacer_checkpoint 0\nnotes_wrap 900\n";
    T_TRUE(settings_parse(wide, (int)strlen(wide), &s, &ignored) == 3);
    T_INT(s.uncert_sig, 2);
    T_INT(s.pacer_checkpoint, 1);
    T_INT(s.notes_wrap, 150);

    settings_defaults(&s);
    char const *narrow = "uncert_sig -4\npacer_checkpoint 999\nnotes_wrap -3\n";
    T_TRUE(settings_parse(narrow, (int)strlen(narrow), &s, &ignored) == 3);
    T_INT(s.uncert_sig, 1);
    T_INT(s.pacer_checkpoint, 200);
    T_INT(s.notes_wrap, 0);

    /* Missing final newline, CRLF line endings, tabs. */
    settings_defaults(&s);
    char const *crlf = "uncert_sig\t2\r\nnotes_wrap 50";
    T_TRUE(settings_parse(crlf, (int)strlen(crlf), &s, &ignored) == 2);
    T_INT(ignored, 0);
    T_INT(s.uncert_sig, 2);
    T_INT(s.notes_wrap, 50);

    /* Empty file keeps the defaults. */
    settings_defaults(&s);
    T_INT(settings_parse("", 0, &s, &ignored), 0);
    T_INT(s.uncert_sig, 1);

    /* A version line is accepted and does not count as junk. */
    settings_defaults(&s);
    char const *ver = "version 1\nuncert_sig 2\n";
    T_TRUE(settings_parse(ver, (int)strlen(ver), &s, &ignored) == 2);
    T_INT(ignored, 0);

    /* Bad arguments. */
    T_INT(settings_parse(NULL, 4, &s, &ignored), -1);
    T_INT(settings_parse("x 1", 3, NULL, &ignored), -1);
    T_INT(settings_parse("x 1", -1, &s, &ignored), -1);
}

static void test_clamps(void)
{
    T_SECTION("clamp helpers");
    T_INT(settings_clamp_uncert_sig(0), 1);
    T_INT(settings_clamp_uncert_sig(1), 1);
    T_INT(settings_clamp_uncert_sig(2), 2);
    T_INT(settings_clamp_uncert_sig(7), 2);

    T_INT(settings_clamp_checkpoint(-5), 1);
    T_INT(settings_clamp_checkpoint(10), 10);
    T_INT(settings_clamp_checkpoint(201), 200);

    T_INT(settings_clamp_wrap(-1), 0);
    T_INT(settings_clamp_wrap(0), 0);
    T_INT(settings_clamp_wrap(49), 50);
    T_INT(settings_clamp_wrap(100), 100);
    T_INT(settings_clamp_wrap(200), 150);
}

static void test_equality(void)
{
    bench_settings_t a, b;

    T_SECTION("equality");
    settings_defaults(&a);
    settings_defaults(&b);
    T_TRUE(settings_equal(&a, &b));
    b.notes_wrap = 50;
    T_FALSE(settings_equal(&a, &b));
    T_FALSE(settings_equal(&a, NULL));
    T_FALSE(settings_equal(NULL, &b));
}

static void test_text(void)
{
    T_SECTION("setting text");
    T_STR(settings_uncert_sig_text(1), "1 figure");
    T_STR(settings_uncert_sig_text(2), "1 or 2 if digit is 1");
}

static void run_tests(void)
{
    test_defaults();
    test_format_round_trip();
    test_format_small_buffer();
    test_parse_tolerance();
    test_clamps();
    test_equality();
    test_text();
}

T_MAIN("test_settings")
