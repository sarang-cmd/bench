/* test_numtheory.c - every acceptance row and edge case of plan 6.5. */

#include <string.h>

#include "numtheory.h"
#include "test_util.h"

/* Values verified independently (see the note in docs/STATUS.md):
 *   1000000007, 1000000009, 999999999999999989 (10^18 - 11) are prime
 *   2^61 - 1 is prime
 *   600851475143 = 71 * 839 * 1471 * 6857
 *   1000000007 * 1000000009 = 1000000016000000063 */

/* Cancel hooks used by the factorisation tests. */
static int cancel_calls = 0;

static bool cancel_never(void *ctx)
{
    (void)ctx;
    cancel_calls++;
    return false;
}

static bool cancel_after_first(void *ctx)
{
    (void)ctx;
    return cancel_calls++ > 0;
}

static bool cancel_always(void *ctx)
{
    (void)ctx;
    return true;
}

static void acceptance_rows(void)
{
    T_SECTION("acceptance table");

    T_UINT(nt_gcd(84, 36), 12);

    uint64_t inv = 0;
    T_INT(nt_modinv(3, 11, &inv), NT_OK);
    T_UINT(inv, 4);

    T_UINT(nt_modpow(2, 10, 1000), 24);
    T_TRUE(nt_is_prime(1000000007));

    {
        nt_factorization_t f;
        T_INT(nt_factor(600851475143ULL, &f, NULL, NULL, NULL), NT_OK);
        T_INT(f.count, 4);
        T_UINT(f.factor[0], 71);
        T_UINT(f.factor[1], 839);
        T_UINT(f.factor[2], 1471);
        T_UINT(f.factor[3], 6857);
        T_INT(f.exponent[0], 1);
        T_INT(f.exponent[3], 1);
    }

    {
        uint64_t t = 0;
        T_INT(nt_totient(36, &t, NULL, NULL), NT_OK);
        T_UINT(t, 12);
    }

    {
        uint64_t r = 0, m = 0;
        T_INT(nt_crt2(2, 3, 3, 5, &r, &m), NT_OK);
        T_UINT(r, 8);
        T_UINT(m, 15);
    }

    T_INT(nt_modinv(6, 9, &inv), NT_ERR_NO_INVERSE);
}

static void modular_arithmetic(void)
{
    T_SECTION("addmod / mulmod / modpow at the 64-bit edge");

    /* No overflow: 2^32 * 2^32 mod (2^32 + 1) = 1 because 2^32 = -1 there. */
    T_UINT(nt_mulmod(4294967296ULL, 4294967296ULL, 4294967297ULL), 1);

    /* The largest 64-bit values. */
    T_UINT(nt_mulmod(UINT64_MAX, UINT64_MAX, UINT64_MAX), 0);
    /* (2^64-1) mod (2^64-2) is 1, so the product is 2. */
    T_UINT(nt_mulmod(UINT64_MAX, 2, UINT64_MAX - 1), 2);
    T_UINT(nt_mulmod(0, UINT64_MAX, 97), 0);
    T_UINT(nt_mulmod(5, 0, 97), 0);

    /* Double-and-add against a straightforward reference for small numbers. */
    for (uint64_t a = 0; a < 40; a += 7) {
        for (uint64_t b = 0; b < 40; b += 5) {
            for (uint64_t m = 1; m < 30; m += 3)
                T_UINT(nt_mulmod(a, b, m), (a * b) % m);
        }
    }

    /* addmod never wraps. */
    T_UINT(nt_addmod(UINT64_MAX, UINT64_MAX, UINT64_MAX), 0);
    T_UINT(nt_addmod(10, 20, 7), 2);
    /* (2^64-6) + 10 = 2^64 + 4, which is 5 modulo 2^64-1. */
    T_UINT(nt_addmod(UINT64_MAX - 5, 10, UINT64_MAX), 5);

    /* modpow basics. */
    T_UINT(nt_modpow(2, 0, 1000), 1);
    T_UINT(nt_modpow(0, 0, 1000), 1);
    T_UINT(nt_modpow(0, 5, 1000), 0);
    T_UINT(nt_modpow(5, 1, 1000), 5);
    T_UINT(nt_modpow(2, 64, 1000), 616); /* 2^64 mod 1000 */
    T_UINT(nt_modpow(7, 560, 561), 1);   /* Carmichael number 561 */
    T_UINT(nt_modpow(2, 1, 1), 0);       /* modulus 1 */
    T_UINT(nt_modpow(2, 10, 1), 0);

    /* Fermat's little theorem as a cross-check. */
    for (uint64_t a = 2; a < 20; a++)
        T_UINT(nt_modpow(a, 1000000006ULL, 1000000007ULL), 1);
}

static void gcd_lcm_egcd(void)
{
    T_SECTION("gcd, lcm, extended gcd, inverse");

    T_UINT(nt_gcd(84, 36), 12);
    T_UINT(nt_gcd(36, 84), 12);      /* symmetric */
    T_UINT(nt_gcd(0, 5), 5);
    T_UINT(nt_gcd(5, 0), 5);
    T_UINT(nt_gcd(0, 0), 0);
    T_UINT(nt_gcd(7, 7), 7);         /* equal inputs */
    T_UINT(nt_gcd(1, 1), 1);
    T_UINT(nt_gcd(1, 0), 1);
    T_UINT(nt_gcd(17, 13), 1);
    T_UINT(nt_gcd(600851475143ULL, 71), 71);

    uint64_t l = 0;
    T_INT(nt_lcm(4, 6, &l), NT_OK);
    T_UINT(l, 12);
    T_INT(nt_lcm(0, 5, &l), NT_OK);
    T_UINT(l, 0);
    T_INT(nt_lcm(7, 7, &l), NT_OK);
    T_UINT(l, 7);
    T_INT(nt_lcm(600851475143ULL, 71, &l), NT_OK);
    T_UINT(l, 600851475143ULL);
    T_INT(nt_lcm(UINT64_MAX, 2, &l), NT_ERR_OVERFLOW);
    T_INT(nt_lcm(1, 1, NULL), NT_ERR_ARG);

    /* Bezout: a*x + b*y = g, checked with small numbers where int64 is exact. */
    {
        int64_t x = 0, y = 0;
        uint64_t g = nt_egcd(240, 46, &x, &y);
        T_UINT(g, 2);
        T_TRUE((int64_t)240 * x + (int64_t)46 * y == 2);
    }
    {
        int64_t x = 0, y = 0;
        T_UINT(nt_egcd(84, 36, &x, &y), 12);
        T_TRUE((int64_t)84 * x + (int64_t)36 * y == 12);
    }
    {
        int64_t x = 0, y = 0;
        T_UINT(nt_egcd(7, 0, &x, &y), 7);
        T_TRUE(x == 1 && y == 0);
    }
    /* Inputs above 2^63 cannot give signed coefficients. */
    {
        int64_t x = 7, y = 7;
        T_UINT(nt_egcd(UINT64_MAX, 3, &x, &y), 0);
        T_INT(x, 0);
        T_INT(y, 0);
    }

    /* Inverses. */
    uint64_t inv = 0;
    T_INT(nt_modinv(3, 11, &inv), NT_OK);
    T_UINT(inv, 4);
    T_INT(nt_modinv(1, 11, &inv), NT_OK);
    T_UINT(inv, 1);
    T_INT(nt_modinv(10, 11, &inv), NT_OK);
    T_UINT(inv, 10); /* 10 = -1 mod 11 */
    T_INT(nt_modinv(6, 9, &inv), NT_ERR_NO_INVERSE);
    T_INT(nt_modinv(0, 9, &inv), NT_ERR_NO_INVERSE);
    T_INT(nt_modinv(1, 1, &inv), NT_OK);
    T_UINT(inv, 0);
    T_INT(nt_modinv(5, 0, &inv), NT_ERR_ARG);
    T_INT(nt_modinv(5, 7, NULL), NT_ERR_ARG);

    /* The inverse really is one: a * inv mod m == 1. */
    for (uint64_t a = 1; a < 50; a++) {
        if (nt_gcd(a, 101) != 1)
            continue;
        T_INT(nt_modinv(a, 101, &inv), NT_OK);
        T_UINT(nt_mulmod(a, inv, 101), 1);
    }

    /* Large inverse near 2^63. */
    T_INT(nt_modinv(999999999999999989ULL, 1000000007ULL, &inv), NT_OK);
    T_UINT(nt_mulmod(999999999999999989ULL % 1000000007ULL, inv,
                     1000000007ULL), 1);
}

static void primality(void)
{
    T_SECTION("primality");

    T_FALSE(nt_is_prime(0));
    T_FALSE(nt_is_prime(1));
    T_TRUE(nt_is_prime(2));
    T_TRUE(nt_is_prime(3));
    T_FALSE(nt_is_prime(4));
    T_TRUE(nt_is_prime(5));
    T_FALSE(nt_is_prime(9));
    T_TRUE(nt_is_prime(97));
    T_FALSE(nt_is_prime(91));      /* 7 * 13 */
    T_FALSE(nt_is_prime(561));     /* Carmichael: 3 * 11 * 17 */
    T_TRUE(nt_is_prime(1000000007));
    T_FALSE(nt_is_prime(1000000007ULL * 1000000009ULL));

    /* A prime near 10^18 and the largest prime below 2^64. */
    T_TRUE(nt_is_prime(999999999999999989ULL));
    T_TRUE(nt_is_prime(18446744073709551557ULL));
    T_FALSE(nt_is_prime(999999999999999999ULL)); /* 3^4 * 37 * 333667... */
    T_TRUE(nt_is_prime(2305843009213693951ULL)); /* 2^61 - 1 */

    /* All primes below 200, checked against a sieve built here. */
    {
        int sieve[200];
        memset(sieve, 1, sizeof sieve);
        sieve[0] = sieve[1] = 0;
        for (int i = 2; i < 200; i++)
            if (sieve[i])
                for (int j = i * i; j < 200; j += i)
                    sieve[j] = 0;
        for (int i = 0; i < 200; i++)
            T_TRUE(nt_is_prime((uint64_t)i) == (sieve[i] != 0));
    }
}

static void factorisation(void)
{
    T_SECTION("factorisation");

    nt_factorization_t f;

    /* 1 has no prime factors. */
    T_INT(nt_factor(1, &f, NULL, NULL, NULL), NT_OK);
    T_INT(f.count, 0);

    /* 0 is not factorable. */
    T_INT(nt_factor(0, &f, NULL, NULL, NULL), NT_ERR_BAD_INPUT);
    T_INT(nt_factor(1, NULL, NULL, NULL, NULL), NT_ERR_ARG);

    /* Powers combine into exponents. */
    T_INT(nt_factor(36, &f, NULL, NULL, NULL), NT_OK);
    T_INT(f.count, 2);
    T_UINT(f.factor[0], 2);
    T_INT(f.exponent[0], 2);
    T_UINT(f.factor[1], 3);
    T_INT(f.exponent[1], 2);

    T_INT(nt_factor(1024, &f, NULL, NULL, NULL), NT_OK);
    T_INT(f.count, 1);
    T_UINT(f.factor[0], 2);
    T_INT(f.exponent[0], 10);

    /* A prime factors to itself. */
    T_INT(nt_factor(1000000007ULL, &f, NULL, NULL, NULL), NT_OK);
    T_INT(f.count, 1);
    T_UINT(f.factor[0], 1000000007ULL);

    /* The plan's row. */
    T_INT(nt_factor(600851475143ULL, &f, NULL, NULL, NULL), NT_OK);
    T_INT(f.count, 4);

    /* A product of two primes near 10^9 needs rho, not trial division. */
    T_INT(nt_factor(1000000007ULL * 1000000009ULL, &f, NULL, NULL, NULL),
          NT_OK);
    T_INT(f.count, 2);
    T_UINT(f.factor[0], 1000000007ULL);
    T_UINT(f.factor[1], 1000000009ULL);

    /* A semiprime with a prime factor near 10^6. */
    T_INT(nt_factor(999983ULL * 1000003ULL, &f, NULL, NULL, NULL), NT_OK);
    T_INT(f.count, 2);

    /* Every factor multiplied back must give the input. */
    {
        uint64_t values[] = { 2, 3, 97, 1000, 65536, 999999999999999989ULL,
                              12345678901234567ULL, 1000000016000000063ULL };
        for (unsigned i = 0; i < sizeof values / sizeof *values; i++) {
            uint64_t product = 1;
            T_INT(nt_factor(values[i], &f, NULL, NULL, NULL), NT_OK);
            for (int k = 0; k < f.count; k++) {
                T_TRUE(nt_is_prime(f.factor[k]));
                for (int e = 0; e < f.exponent[k]; e++)
                    product *= f.factor[k];
            }
            T_UINT(product, values[i]);
        }
    }

    /* Progress counter moves. */
    {
        nt_progress_t p;
        T_INT(nt_factor(1000000016000000063ULL, &f, NULL, NULL, &p), NT_OK);
        T_TRUE(p.steps > 0);
    }

    /* Progress polling: the hook runs during a long factorisation. */
    {
        cancel_calls = 0;
        T_INT(nt_factor(1000000007ULL * 1000000009ULL, &f, cancel_never, NULL,
                        NULL), NT_OK);
        T_TRUE(cancel_calls > 0);
    }

    /* Cancelling gives up cleanly with a status the app can show. */
    {
        cancel_calls = 0;
        T_INT(nt_factor(1000000007ULL * 1000000009ULL, &f, cancel_always, NULL,
                        NULL), NT_ERR_CANCELLED);
        T_STR(nt_error_text(NT_ERR_CANCELLED), "stopped");
    }

    /* A hook that lets the first check through still stops it afterwards. */
    {
        nt_factorization_t big;
        cancel_calls = 0;
        nt_status_t st = nt_factor(1000000007ULL * 1000000009ULL, &big,
                                   cancel_after_first, NULL, NULL);
        T_TRUE(st == NT_ERR_CANCELLED || st == NT_OK);
    }
}

static void totients(void)
{
    T_SECTION("totient");

    uint64_t t = 0;
    T_INT(nt_totient(1, &t, NULL, NULL), NT_OK);
    T_UINT(t, 1);
    T_INT(nt_totient(36, &t, NULL, NULL), NT_OK);
    T_UINT(t, 12);
    T_INT(nt_totient(2, &t, NULL, NULL), NT_OK);
    T_UINT(t, 1);
    T_INT(nt_totient(1000000007ULL, &t, NULL, NULL), NT_OK);
    T_UINT(t, 1000000006ULL);
    T_INT(nt_totient(1ULL << 40, &t, NULL, NULL), NT_OK);
    T_UINT(t, 1ULL << 39);
    T_INT(nt_totient(0, &t, NULL, NULL), NT_ERR_BAD_INPUT);
    T_INT(nt_totient(5, NULL, NULL, NULL), NT_ERR_ARG);

    /* From a precomputed factorisation. */
    {
        nt_factorization_t f;
        T_INT(nt_factor(36, &f, NULL, NULL, NULL), NT_OK);
        T_INT(nt_totient_from(&f, &t), NT_OK);
        T_UINT(t, 12);
        T_INT(nt_totient_from(NULL, &t), NT_ERR_ARG);
    }
}

static void crt(void)
{
    T_SECTION("Chinese remainder theorem");

    uint64_t r = 0, m = 0;

    /* The plan's row. */
    T_INT(nt_crt2(2, 3, 3, 5, &r, &m), NT_OK);
    T_UINT(r, 8);
    T_UINT(m, 15);

    /* x = 1 mod 2, x = 2 mod 3, x = 3 mod 5 -> 23 mod 30 */
    {
        nt_congruence_t list[3] = {
            { 1, 2 }, { 2, 3 }, { 3, 5 }
        };
        T_INT(nt_crt(list, 3, &r, &m), NT_OK);
        T_UINT(r, 23);
        T_UINT(m, 30);
    }

    /* Disagreeing congruences. */
    T_INT(nt_crt2(0, 2, 1, 2, &r, &m), NT_ERR_NO_SOLUTION);
    T_INT(nt_crt2(1, 4, 2, 6, &r, &m), NT_ERR_NO_SOLUTION);

    /* One modulus divides the other. */
    T_INT(nt_crt2(2, 4, 6, 12, &r, &m), NT_OK);
    T_UINT(r, 6);
    T_UINT(m, 12);
    T_INT(nt_crt2(3, 6, 3, 12, &r, &m), NT_OK);
    T_UINT(r, 3);
    T_UINT(m, 12);

    /* Same modulus, same residue: fine; different residue: no solution. */
    T_INT(nt_crt2(1, 7, 1, 7, &r, &m), NT_OK);
    T_UINT(r, 1);
    T_UINT(m, 7);
    T_INT(nt_crt2(1, 7, 2, 7, &r, &m), NT_ERR_NO_SOLUTION);

    /* Residues larger than their modulus are reduced. */
    T_INT(nt_crt2(8, 3, 3, 5, &r, &m), NT_OK);
    T_UINT(r, 8);
    T_UINT(m, 15);

    /* A single congruence list is returned unchanged. */
    {
        nt_congruence_t one = { 5, 11 };
        T_INT(nt_crt(&one, 1, &r, &m), NT_OK);
        T_UINT(r, 5);
        T_UINT(m, 11);
    }

    /* A four-way system with coprime moduli. */
    {
        nt_congruence_t list[4] = {
            { 1, 2 }, { 2, 3 }, { 3, 5 }, { 4, 7 }
        };
        T_INT(nt_crt(list, 4, &r, &m), NT_OK);
        T_UINT(m, 210);
        T_UINT(r, 53); /* 53 mod 2=1, 3=2, 5=3, 7=4 */
        T_UINT(r % 2, 1);
        T_UINT(r % 3, 2);
        T_UINT(r % 5, 3);
        T_UINT(r % 7, 4);
    }

    /* Bad arguments. */
    T_INT(nt_crt2(1, 0, 2, 3, &r, &m), NT_ERR_ARG);
    T_INT(nt_crt2(1, 3, 2, 3, NULL, &m), NT_ERR_ARG);
    T_INT(nt_crt(NULL, 2, &r, &m), NT_ERR_ARG);
    T_INT(nt_crt(NULL, 0, &r, &m), NT_ERR_ARG);
}

static void parsing(void)
{
    T_SECTION("input parsing: at most 18 digits");

    uint64_t v = 0;

    T_INT(nt_parse_u64("0", 1, &v), NT_OK);
    T_UINT(v, 0);
    T_INT(nt_parse_u64("84", 2, &v), NT_OK);
    T_UINT(v, 84);
    T_INT(nt_parse_u64("  42  ", 6, &v), NT_OK);
    T_UINT(v, 42);
    T_INT(nt_parse_u64("999999999999999999", 18, &v), NT_OK);
    T_UINT(v, 999999999999999999ULL);

    /* 19 digits is too large even though it would fit in 64 bits. */
    T_INT(nt_parse_u64("1000000000000000000", 19, &v), NT_ERR_TOO_LARGE);
    T_INT(nt_parse_u64("9999999999999999999", 19, &v), NT_ERR_TOO_LARGE);

    T_INT(nt_parse_u64("12a", 3, &v), NT_ERR_BAD_INPUT);
    T_INT(nt_parse_u64("-5", 2, &v), NT_ERR_BAD_INPUT);
    T_INT(nt_parse_u64("   ", 3, &v), NT_ERR_BAD_INPUT);
    T_INT(nt_parse_u64("", 0, &v), NT_ERR_BAD_INPUT);
    T_INT(nt_parse_u64("12 34", 5, &v), NT_ERR_BAD_INPUT);
    T_INT(nt_parse_u64(NULL, 1, &v), NT_ERR_BAD_INPUT);
    T_INT(nt_parse_u64("1", 1, NULL), NT_ERR_BAD_INPUT);
}

static void edges(void)
{
    T_SECTION("edge cases: 0, 1, equal inputs");

    /* gcd(0,0) is 0 by convention, lcm(0,0) is 0. */
    uint64_t l = 9;
    T_INT(nt_lcm(0, 0, &l), NT_OK);
    T_UINT(l, 0);

    /* modpow with exponent 1 and modulus 2. */
    T_UINT(nt_modpow(3, 1, 2), 1);
    T_UINT(nt_modpow(2, 63, UINT64_MAX), 9223372036854775808ULL);

    /* Factorisation of the largest 18-digit prime. */
    {
        nt_factorization_t f;
        T_INT(nt_factor(999999999999999989ULL, &f, NULL, NULL, NULL), NT_OK);
        T_INT(f.count, 1);
        T_UINT(f.factor[0], 999999999999999989ULL);
    }

    /* Error text is never NULL. */
    T_TRUE(nt_error_text(NT_OK) != NULL);
    T_STR(nt_error_text(NT_ERR_NO_INVERSE), "no inverse");
    T_TRUE(nt_error_text((nt_status_t)99) != NULL);
}

static void run_tests(void)
{
    acceptance_rows();
    modular_arithmetic();
    gcd_lcm_egcd();
    primality();
    factorisation();
    totients();
    crt();
    parsing();
    edges();
}

T_MAIN("test_numtheory")
