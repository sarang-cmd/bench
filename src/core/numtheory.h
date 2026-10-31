/* numtheory.h - number theory for the calculator.
 *
 * Spec: docs/plan.md section 6.5.  Unsigned 64-bit inputs, at most 18 decimal
 * digits, no 128-bit integer type: modular multiplication is done with
 * double-and-add so it works on the SuperH build as well as on the PC.
 *
 * Plain C11, no gint, no allocation. */

#ifndef BENCH_NUMTHEORY_H
#define BENCH_NUMTHEORY_H

#include <stdbool.h>
#include <stdint.h>

#define NT_MAX_INPUT_DIGITS 18
#define NT_MAX_FACTORS 64
#define NT_MAX_CONGRUENCES 8

typedef enum {
    NT_OK = 0,
    NT_ERR_ARG,          /* NULL pointer or a modulus of 0 */
    NT_ERR_NO_INVERSE,   /* gcd(a, m) != 1 */
    NT_ERR_OVERFLOW,     /* the exact result does not fit in 64 bits */
    NT_ERR_TOO_LARGE,    /* input longer than 18 digits */
    NT_ERR_BAD_INPUT,    /* input text is not a number */
    NT_ERR_NO_SOLUTION,  /* CRT: the congruences disagree */
    NT_ERR_CANCELLED,    /* the caller asked to stop */
    NT_ERR_FAILED,       /* an algorithm gave up (should not happen) */
} nt_status_t;

char const *nt_error_text(nt_status_t status);

/* ---- basic arithmetic --------------------------------------------------- */

uint64_t nt_gcd(uint64_t a, uint64_t b);

/* Least common multiple.  Returns NT_ERR_OVERFLOW when it does not fit. */
nt_status_t nt_lcm(uint64_t a, uint64_t b, uint64_t *out);

/* Extended Euclid: a*x + b*y = g, with |x| and |y| bounded by the other input.
 * Returns the gcd in *g.  x and y may be NULL. */
uint64_t nt_egcd(uint64_t a, uint64_t b, int64_t *x, int64_t *y);

/* Modular inverse of a modulo m.  NT_ERR_NO_INVERSE when gcd(a, m) != 1. */
nt_status_t nt_modinv(uint64_t a, uint64_t m, uint64_t *inv);

/* (a + b) mod m and (a * b) mod m without ever overflowing 64 bits.
 * Requires 1 <= m; a and b are reduced first. */
uint64_t nt_addmod(uint64_t a, uint64_t b, uint64_t m);
uint64_t nt_mulmod(uint64_t a, uint64_t b, uint64_t m);

/* base^exp mod m.  m must be >= 1. */
uint64_t nt_modpow(uint64_t base, uint64_t exp, uint64_t m);

/* Deterministic Miller-Rabin, valid for every 64-bit input. */
bool nt_is_prime(uint64_t n);

/* ---- factorisation ------------------------------------------------------ */

typedef struct {
    uint64_t factor[NT_MAX_FACTORS];
    uint8_t exponent[NT_MAX_FACTORS];
    int count;
} nt_factorization_t;

/* Returns true to stop a long factorisation (the app polls EXIT). */
typedef bool (*nt_cancel_fn)(void *ctx);

/* Progress: number of trial divisions and rho rounds so far.  May be NULL. */
typedef struct {
    uint32_t steps;
    uint32_t trial_limit;
} nt_progress_t;

/* Factor n >= 1.  n == 1 gives an empty factorisation.
 * Factors are sorted ascending with exponents combined. */
nt_status_t nt_factor(uint64_t n, nt_factorization_t *out, nt_cancel_fn cancel,
                      void *ctx, nt_progress_t *progress);

/* Euler's totient from a factorisation, and directly (which factors n). */
nt_status_t nt_totient_from(nt_factorization_t const *f, uint64_t *out);
nt_status_t nt_totient(uint64_t n, uint64_t *out, nt_cancel_fn cancel,
                       void *ctx);

/* ---- Chinese remainder theorem ------------------------------------------ */

typedef struct {
    uint64_t residue;
    uint64_t modulus;
} nt_congruence_t;

/* x = r1 (mod m1), x = r2 (mod m2).  Writes the combined x and the lcm of the
 * moduli.  NT_ERR_NO_SOLUTION when they disagree. */
nt_status_t nt_crt2(uint64_t r1, uint64_t m1, uint64_t r2, uint64_t m2,
                    uint64_t *out_residue, uint64_t *out_modulus);

/* Folds a list of congruences. */
nt_status_t nt_crt(nt_congruence_t const *list, int n, uint64_t *out_residue,
                   uint64_t *out_modulus);

/* ---- input -------------------------------------------------------------- */

/* Parses an unsigned decimal of at most 18 digits.
 * NT_ERR_TOO_LARGE for 19 digits or more, NT_ERR_BAD_INPUT for other text. */
nt_status_t nt_parse_u64(char const *text, int len, uint64_t *out);

#endif /* BENCH_NUMTHEORY_H */
