/* test_util.h - tiny assertion harness for the native core tests.
 *
 * One test file per core module, each with its own main().  The harness keeps
 * a running count of checks and failures so `make test` can print a summary.
 * No external dependency, no gint. */

#ifndef BENCH_TEST_UTIL_H
#define BENCH_TEST_UTIL_H

#include <math.h>
#include <stdio.h>
#include <string.h>

static int t_checks = 0;
static int t_fails = 0;
static const char *t_suite = "suite";

#define T_SECTION(name) \
    do { printf("  -- %s\n", (name)); } while (0)

#define T_CHECK(cond, ...)                                                   \
    do {                                                                     \
        t_checks++;                                                          \
        if (!(cond)) {                                                       \
            t_fails++;                                                       \
            printf("  FAIL %s:%d: ", __FILE__, __LINE__);                    \
            printf(__VA_ARGS__);                                             \
            printf("\n");                                                    \
        }                                                                    \
    } while (0)

#define T_TRUE(cond) T_CHECK(cond, "expected <%s>", #cond)
#define T_FALSE(cond) T_CHECK(!(cond), "expected !(%s)", #cond)

#define T_INT(got, want)                                                     \
    T_CHECK((long long)(got) == (long long)(want),                           \
            "%s: got %lld, want %lld", #got, (long long)(got),               \
            (long long)(want))

#define T_UINT(got, want)                                                    \
    T_CHECK((unsigned long long)(got) == (unsigned long long)(want),         \
            "%s: got %llu, want %llu", #got,                                 \
            (unsigned long long)(got), (unsigned long long)(want))

#define T_STR(got, want)                                                     \
    T_CHECK(strcmp((const char *)(got), (const char *)(want)) == 0,          \
            "%s: got \"%s\", want \"%s\"", #got, (const char *)(got),        \
            (const char *)(want))

#define T_NEAR(got, want, eps)                                               \
    T_CHECK(fabs((double)(got) - (double)(want)) <= (double)(eps),           \
            "%s: got %.17g, want %.17g (eps %.3g)", #got, (double)(got),     \
            (double)(want), (double)(eps))

/* Exact comparison for values produced by the decimal formatter. */
#define T_EXACT(got, want) T_NEAR(got, want, 0.0)

static int t_report(void)
{
    printf("%-24s %3d checks, %d failed\n", t_suite, t_checks, t_fails);
    return t_fails == 0 ? 0 : 1;
}

#define T_MAIN(suite_name)                                                   \
    int main(void)                                                           \
    {                                                                        \
        t_suite = (suite_name);                                              \
        printf("%s\n", t_suite);                                             \
        run_tests();                                                         \
        return t_report();                                                   \
    }

#endif /* BENCH_TEST_UTIL_H */
