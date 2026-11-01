/* numtheory.c - see numtheory.h. */

#include "numtheory.h"

#include <string.h>

char const *nt_error_text(nt_status_t status)
{
    switch (status) {
    case NT_OK:
        return "ok";
    case NT_ERR_ARG:
        return "bad input";
    case NT_ERR_NO_INVERSE:
        return "no inverse";
    case NT_ERR_OVERFLOW:
        return "too large to compute";
    case NT_ERR_TOO_LARGE:
        return "more than 18 digits";
    case NT_ERR_BAD_INPUT:
        return "not a whole number";
    case NT_ERR_NO_SOLUTION:
        return "no solution";
    case NT_ERR_CANCELLED:
        return "stopped";
    case NT_ERR_FAILED:
        return "gave up";
    }
    return "error";
}

/* ---- modular arithmetic ------------------------------------------------- */

uint64_t nt_addmod(uint64_t a, uint64_t b, uint64_t m)
{
    if (m == 0)
        return a + b; /* undefined use; callers keep m >= 1 */
    a %= m;
    b %= m;
    /* a + b would wrap; compare against the gap to m instead. */
    return a >= m - b ? a - (m - b) : a + b;
}

uint64_t nt_mulmod(uint64_t a, uint64_t b, uint64_t m)
{
    uint64_t result = 0;

    if (m == 0)
        return 0;
    a %= m;
    b %= m;
    if (m == 1)
        return 0;

    /* Double-and-add: at most 64 steps, never a product wider than 64 bits. */
    while (b) {
        if (b & 1u)
            result = nt_addmod(result, a, m);
        a = nt_addmod(a, a, m);
        b >>= 1;
    }
    return result;
}

uint64_t nt_modpow(uint64_t base, uint64_t exp, uint64_t m)
{
    uint64_t result;

    if (m == 0)
        return 0;
    if (m == 1)
        return 0;

    result = 1 % m;
    base %= m;
    while (exp) {
        if (exp & 1u)
            result = nt_mulmod(result, base, m);
        base = nt_mulmod(base, base, m);
        exp >>= 1;
    }
    return result;
}

/* ---- gcd, lcm, egcd, inverse -------------------------------------------- */

uint64_t nt_gcd(uint64_t a, uint64_t b)
{
    while (b) {
        uint64_t t = a % b;
        a = b;
        b = t;
    }
    return a;
}

/* a * b with an overflow flag. */
static bool mul_overflow(uint64_t a, uint64_t b, uint64_t *out)
{
    if (a == 0 || b == 0) {
        *out = 0;
        return false;
    }
    if (a > UINT64_MAX / b)
        return true;
    *out = a * b;
    return false;
}

nt_status_t nt_lcm(uint64_t a, uint64_t b, uint64_t *out)
{
    uint64_t g, prod;

    if (!out)
        return NT_ERR_ARG;
    if (a == 0 || b == 0) {
        *out = 0;
        return NT_OK;
    }
    g = nt_gcd(a, b);
    if (mul_overflow(a / g, b, &prod))
        return NT_ERR_OVERFLOW;
    *out = prod;
    return NT_OK;
}

uint64_t nt_egcd(uint64_t a, uint64_t b, int64_t *x, int64_t *y)
{
    /* Iterative extended Euclid on magnitudes.  The coefficients are signed
     * 64-bit, so the inputs must stay below 2^63; 18-digit input guarantees
     * that (the largest 18-digit number is 9.99e17). */
    int64_t old_r, r;
    int64_t old_s, s;
    int64_t old_t, t;

    if (a > (uint64_t)INT64_MAX || b > (uint64_t)INT64_MAX) {
        if (x)
            *x = 0;
        if (y)
            *y = 0;
        return 0; /* not representable: gcd only via nt_gcd */
    }

    old_r = (int64_t)a;
    r = (int64_t)b;
    old_s = 1;
    s = 0;
    old_t = 0;
    t = 1;

    while (r != 0) {
        int64_t q = old_r / r;

        int64_t tmp = old_r - q * r;
        old_r = r;
        r = tmp;

        tmp = old_s - q * s;
        old_s = s;
        s = tmp;

        tmp = old_t - q * t;
        old_t = t;
        t = tmp;
    }

    if (old_r < 0) { /* keep the gcd positive */
        old_r = -old_r;
        old_s = -old_s;
        old_t = -old_t;
    }
    if (x)
        *x = old_s;
    if (y)
        *y = old_t;
    return (uint64_t)old_r;
}

nt_status_t nt_modinv(uint64_t a, uint64_t m, uint64_t *inv)
{
    int64_t x = 0, y = 0;
    uint64_t g;

    if (!inv || m == 0)
        return NT_ERR_ARG;
    if (m == 1) { /* everything is 0 modulo 1 */
        *inv = 0;
        return NT_OK;
    }

    g = nt_egcd(a % m, m, &x, &y);
    if (g != 1)
        return NT_ERR_NO_INVERSE;

    /* x may be negative; bring it into 0..m-1 without overflowing. */
    {
        int64_t mm = (int64_t)m;
        int64_t r = x % mm;
        if (r < 0)
            r += mm;
        *inv = (uint64_t)r;
    }
    return NT_OK;
}

/* ---- primality ---------------------------------------------------------- */

static bool witness(uint64_t a, uint64_t d, unsigned s, uint64_t n)
{
    uint64_t x = nt_modpow(a, d, n);

    if (x == 1 || x == n - 1)
        return false; /* a is not a witness: n may be prime */

    for (unsigned r = 1; r < s; r++) {
        x = nt_mulmod(x, x, n);
        if (x == n - 1)
            return false;
        if (x == 1)
            return true; /* a nontrivial square root of 1 */
    }
    return true;
}

bool nt_is_prime(uint64_t n)
{
    static uint64_t const small[] = {
        2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37
    };
    uint64_t d;
    unsigned s = 0;
    size_t i;

    if (n < 2)
        return false;
    for (i = 0; i < sizeof small / sizeof *small; i++) {
        if (n == small[i])
            return true;
        if (n % small[i] == 0)
            return false;
    }

    /* n - 1 = d * 2^s with d odd. */
    d = n - 1;
    while ((d & 1u) == 0) {
        d >>= 1;
        s++;
    }

    /* These twelve bases are a deterministic test for every 64-bit n. */
    for (i = 0; i < sizeof small / sizeof *small; i++) {
        if (witness(small[i], d, s, n))
            return false;
    }
    return true;
}

/* ---- factorisation ------------------------------------------------------ */

/* Pollard's rho with Brent's improvement.  Returns a non-trivial divisor of n,
 * or 0 when this attempt failed. */
static uint64_t rho_brent(uint64_t n, uint64_t c, nt_cancel_fn cancel,
                          void *ctx, nt_progress_t *progress)
{
    uint64_t y = 2, x, ys = 0, q = 1, g = 1, r = 1, k = 0;
    uint32_t polls = 0;

    if (n % 2 == 0)
        return 2;

    while (g == 1) {
        x = y;
        q = 1;
        for (uint64_t i = 0; i < r; i++) {
            y = nt_addmod(nt_mulmod(y, y, n), c, n);
            if (progress)
                progress->steps++;
            if (cancel && ((++polls & 0x3FFu) == 0) && cancel(ctx))
                return 0;
        }

        k = 0;
        while (k < r && g == 1) {
            ys = y;
            for (uint64_t i = 0; i < 128 && i < r - k; i++) {
                y = nt_addmod(nt_mulmod(y, y, n), c, n);
                q = nt_mulmod(q, x > y ? x - y : y - x, n);
                if (progress)
                    progress->steps++;
            }
            g = nt_gcd(q, n);
            if (g == 0)
                g = 1;
            k += 128;
            if (cancel && ((++polls & 0x3FFu) == 0) && cancel(ctx))
                return 0;
        }
        r <<= 1;
        if (r == 0)
            return 0; /* defensive: r doubled past 64 bits */
    }

    if (g == n) {
        /* Backtrack one step at a time. */
        do {
            ys = nt_addmod(nt_mulmod(ys, ys, n), c, n);
            g = nt_gcd(x > ys ? x - ys : ys - x, n);
            if (cancel && cancel(ctx))
                return 0;
        } while (g == 1);
    }
    return g == n ? 0 : g;
}

static void add_factor(nt_factorization_t *f, uint64_t p)
{
    int i;

    for (i = 0; i < f->count; i++) {
        if (f->factor[i] == p) {
            if (f->exponent[i] < 255)
                f->exponent[i]++;
            return;
        }
    }
    if (f->count >= NT_MAX_FACTORS)
        return;
    f->factor[f->count] = p;
    f->exponent[f->count] = 1;
    f->count++;
}

static void sort_factors(nt_factorization_t *f)
{
    int i, j;
    for (i = 1; i < f->count; i++) {
        uint64_t v = f->factor[i];
        uint8_t e = f->exponent[i];
        j = i - 1;
        while (j >= 0 && f->factor[j] > v) {
            f->factor[j + 1] = f->factor[j];
            f->exponent[j + 1] = f->exponent[j];
            j--;
        }
        f->factor[j + 1] = v;
        f->exponent[j + 1] = e;
    }
}

nt_status_t nt_factor(uint64_t n, nt_factorization_t *out, nt_cancel_fn cancel,
                      void *ctx, nt_progress_t *progress)
{
    /* Composite cofactors waiting to be split. */
    uint64_t stack[NT_MAX_FACTORS];
    int depth = 0;
    uint64_t remaining = n;

    if (!out)
        return NT_ERR_ARG;
    memset(out, 0, sizeof *out);
    if (progress) {
        progress->steps = 0;
        progress->trial_limit = 1000;
    }
    if (n == 0)
        return NT_ERR_BAD_INPUT;

    /* Small trial division first: it settles most textbook numbers quickly. */
    {
        static uint16_t const small_primes[] = {
            3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59,
            61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127,
            131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191,
            193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257,
            263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331,
            337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401,
            409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467,
            479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563,
            569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631,
            641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709,
            719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797,
            809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877,
            881, 883, 887, 907, 911, 919, 929, 937, 941, 947, 953, 967,
            971, 977, 983, 991, 997
        };

        while (remaining % 2 == 0) {
            add_factor(out, 2);
            remaining /= 2;
            if (progress)
                progress->steps++;
            if (cancel && cancel(ctx))
                return NT_ERR_CANCELLED;
        }
        for (size_t i = 0; i < sizeof small_primes / sizeof *small_primes;
             i++) {
            uint64_t p = small_primes[i];
            if (p * p > remaining)
                break;
            while (remaining % p == 0) {
                add_factor(out, p);
                remaining /= p;
            }
            if (progress)
                progress->steps++;
            if (cancel && cancel(ctx))
                return NT_ERR_CANCELLED;
        }
    }

    if (remaining > 1) {
        stack[depth++] = remaining;

        while (depth > 0) {
            uint64_t m = stack[--depth];

            if (cancel && cancel(ctx))
                return NT_ERR_CANCELLED;

            if (m == 1)
                continue;
            if (nt_is_prime(m)) {
                add_factor(out, m);
                continue;
            }

            /* Split the composite with rho, retrying a few constants. */
            {
                uint64_t d = 0;
                for (uint64_t c = 1; c <= 16 && d == 0; c++) {
                    d = rho_brent(m, c, cancel, ctx, progress);
                    if (cancel && cancel(ctx))
                        return NT_ERR_CANCELLED;
                }
                if (d == 0 || d == m || d == 1)
                    return NT_ERR_FAILED;
                if (depth + 2 > NT_MAX_FACTORS)
                    return NT_ERR_FAILED;
                stack[depth++] = d;
                stack[depth++] = m / d;
            }
        }
    }

    sort_factors(out);
    return NT_OK;
}

nt_status_t nt_totient_from(nt_factorization_t const *f, uint64_t *out)
{
    uint64_t result = 1;

    if (!f || !out)
        return NT_ERR_ARG;

    for (int i = 0; i < f->count; i++) {
        uint64_t p = f->factor[i];
        uint64_t term = p - 1;
        uint64_t prod;

        for (int e = 1; e < f->exponent[i]; e++) {
            if (mul_overflow(term, p, &prod))
                return NT_ERR_OVERFLOW;
            term = prod;
        }
        if (mul_overflow(result, term, &prod))
            return NT_ERR_OVERFLOW;
        result = prod;
    }
    *out = result;
    return NT_OK;
}

nt_status_t nt_divisor_count(nt_factorization_t const *f, uint64_t *out)
{
    uint64_t result = 1;

    if (!f || !out)
        return NT_ERR_ARG;
    for (int i = 0; i < f->count; i++) {
        uint64_t prod;

        if (mul_overflow(result, (uint64_t)f->exponent[i] + 1u, &prod))
            return NT_ERR_OVERFLOW;
        result = prod;
    }
    *out = result;
    return NT_OK;
}

nt_status_t nt_divisor_sum(nt_factorization_t const *f, uint64_t *out)
{
    uint64_t result = 1;

    if (!f || !out)
        return NT_ERR_ARG;
    for (int i = 0; i < f->count; i++) {
        uint64_t p = f->factor[i];
        uint64_t term = 1;          /* 1 + p + p^2 + ... + p^e */
        uint64_t prod;

        for (int e = 0; e < f->exponent[i]; e++) {
            if (mul_overflow(term, p, &prod) || prod == UINT64_MAX)
                return NT_ERR_OVERFLOW;   /* prod + 1 would wrap */
            term = prod + 1;
        }
        if (mul_overflow(result, term, &prod))
            return NT_ERR_OVERFLOW;
        result = prod;
    }
    *out = result;
    return NT_OK;
}

nt_status_t nt_factor_radical(nt_factorization_t const *f, uint64_t *out)
{
    uint64_t result = 1;

    if (!f || !out)
        return NT_ERR_ARG;
    for (int i = 0; i < f->count; i++) {
        uint64_t prod;

        if (mul_overflow(result, f->factor[i], &prod))
            return NT_ERR_OVERFLOW;
        result = prod;
    }
    *out = result;
    return NT_OK;
}

bool nt_factor_is_square(nt_factorization_t const *f)
{
    if (!f)
        return false;
    for (int i = 0; i < f->count; i++)
        if (f->exponent[i] % 2 != 0)
            return false;
    return true;
}

nt_status_t nt_factor_text(nt_factorization_t const *f, char *out, int size)
{
    int n = 0;

    if (!f || !out || size <= 0)
        return NT_ERR_ARG;
    out[0] = 0;
    if (f->count == 0) {
        /* 1 has no prime factors, and "1 = " with nothing after it is not a
         * sentence. */
        if (size < 2)
            return NT_ERR_OVERFLOW;
        out[0] = '1';
        out[1] = 0;
        return NT_OK;
    }

    for (int i = 0; i < f->count; i++) {
        int d = 0;
        uint64_t v = f->factor[i];
        char digits[24];
        int expdigits = 0;
        int sep = i > 0 ? 3 : 0;
        int ex = f->exponent[i];

        while (v > 0) {
            digits[d++] = (char)('0' + (int)(v % 10));
            v /= 10;
        }
        if (ex > 1) {
            expdigits = 1;          /* the '^' */
            for (int t = ex; t > 0; t /= 10)
                expdigits++;
        }
        /* Room for the separator, the digits, the exponent and the NUL.  Exact,
         * so a buffer that holds the whole text is never refused. */
        if (n + sep + d + expdigits + 1 > size)
            return NT_ERR_OVERFLOW;
        if (i > 0) {
            out[n++] = ' ';
            out[n++] = 'x';
            out[n++] = ' ';
        }
        while (d > 0)
            out[n++] = digits[--d];
        if (ex > 1) {
            char ed[4];
            int k = 0;

            out[n++] = '^';
            while (ex > 0 && k < 3) {
                ed[k++] = (char)('0' + ex % 10);
                ex /= 10;
            }
            while (k > 0)
                out[n++] = ed[--k];
        }
    }
    out[n] = 0;
    return NT_OK;
}

nt_status_t nt_totient(uint64_t n, uint64_t *out, nt_cancel_fn cancel,
                       void *ctx)
{
    nt_factorization_t f;
    nt_status_t st;

    if (!out)
        return NT_ERR_ARG;
    if (n == 0)
        return NT_ERR_BAD_INPUT;

    st = nt_factor(n, &f, cancel, ctx, NULL);
    if (st != NT_OK)
        return st;
    return nt_totient_from(&f, out);
}

/* ---- CRT ---------------------------------------------------------------- */

nt_status_t nt_crt2(uint64_t r1, uint64_t m1, uint64_t r2, uint64_t m2,
                    uint64_t *out_residue, uint64_t *out_modulus)
{
    uint64_t g, reduced_m2, diff, inv, k, lcm, term, x;

    if (!out_residue || !out_modulus || m1 == 0 || m2 == 0)
        return NT_ERR_ARG;

    g = nt_gcd(m1, m2);

    /* The two congruences must agree modulo gcd(m1, m2). */
    {
        uint64_t a = r1 % g;
        uint64_t b = r2 % g;
        if (a != b)
            return NT_ERR_NO_SOLUTION;
    }

    if (nt_lcm(m1, m2, &lcm) != NT_OK)
        return NT_ERR_OVERFLOW;

    reduced_m2 = m2 / g;
    if (reduced_m2 == 1) {
        /* One modulus divides the other: the larger one already covers it. */
        *out_modulus = lcm;
        *out_residue = r1 % lcm;
        return NT_OK;
    }

    /* k = ((r2 - r1) / g) * (m1/g)^-1  (mod m2/g) */
    diff = r2 % m2;
    diff = (diff + m2 - (r1 % m2)) % m2;
    diff /= g;
    diff %= reduced_m2;

    if (nt_modinv((m1 / g) % reduced_m2, reduced_m2, &inv) != NT_OK)
        return NT_ERR_NO_SOLUTION;

    k = nt_mulmod(diff % reduced_m2, inv, reduced_m2);
    term = nt_mulmod(m1 % lcm, k % lcm, lcm);
    x = nt_addmod(r1 % lcm, term, lcm);

    *out_modulus = lcm;
    *out_residue = x;
    return NT_OK;
}

nt_status_t nt_crt(nt_congruence_t const *list, int n, uint64_t *out_residue,
                   uint64_t *out_modulus)
{
    uint64_t r, m;
    nt_status_t st;

    if (!list || n <= 0 || !out_residue || !out_modulus)
        return NT_ERR_ARG;

    if (list[0].modulus == 0)
        return NT_ERR_ARG;
    m = list[0].modulus;
    r = list[0].residue % m;

    for (int i = 1; i < n; i++) {
        st = nt_crt2(r, m, list[i].residue, list[i].modulus, &r, &m);
        if (st != NT_OK)
            return st;
    }
    *out_residue = r;
    *out_modulus = m;
    return NT_OK;
}

/* ---- input -------------------------------------------------------------- */

nt_status_t nt_parse_u64(char const *text, int len, uint64_t *out)
{
    uint64_t value = 0;
    int digits = 0, i;

    if (!text || !out || len <= 0)
        return NT_ERR_BAD_INPUT;

    for (i = 0; i < len; i++) {
        char c = text[i];
        if (c == ' ' || c == '\t') {
            if (digits == 0)
                continue; /* leading blanks are fine */
            break;        /* a blank ends the number */
        }
        if (c < '0' || c > '9')
            return NT_ERR_BAD_INPUT;
        digits++;
        if (digits > NT_MAX_INPUT_DIGITS)
            return NT_ERR_TOO_LARGE;
        value = value * 10u + (uint64_t)(c - '0');
    }
    if (digits == 0)
        return NT_ERR_BAD_INPUT;
    /* Only blanks may follow the number. */
    for (; i < len; i++)
        if (text[i] != ' ' && text[i] != '\t')
            return NT_ERR_BAD_INPUT;
    *out = value;
    return NT_OK;
}
