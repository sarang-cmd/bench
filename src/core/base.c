/* base.c - bases and bits.  See base.h. */

#include "base.h"

#include <string.h>

static char const DIGITS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

char const *bit_op_name(bit_op_t op)
{
    switch (op)
    {
    case BIT_AND: return "AND";
    case BIT_OR:  return "OR";
    case BIT_XOR: return "XOR";
    case BIT_SHL: return "<<";
    case BIT_SHR: return ">>";
    default:      return "?";
    }
}

int64_t base_twos_max(int bits)
{
    if (bits <= 0 || bits > 62) return 0;
    return ((int64_t)1 << (bits - 1)) - 1;
}

static int digit_of(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

bool base_valid(char const *s, int base)
{
    int64_t v;

    return base_parse(s, base, &v);
}

bool base_parse(char const *s, int base, int64_t *out)
{
    bool negative = false;
    bool any = false;
    int64_t acc = 0;

    if (!s || base < BASE_MIN || base > BASE_MAX) return false;
    if (*s == '-') {
        negative = true;
        s++;
    }
    if (!*s) return false;

    for (; *s; s++) {
        int const d = digit_of(*s);

        if (d < 0 || d >= base) return false;
        /* Stop before the multiply can wrap. */
        if (acc > (INT64_MAX - d) / base) return false;
        acc = acc * base + d;
        any = true;
    }
    if (!any) return false;
    *out = negative ? -acc : acc;
    return true;
}

bool base_format(int64_t v, int base, char *buf, int size)
{
    char tmp[72];
    int n = 0;
    uint64_t u;
    bool negative = v < 0;

    if (base < BASE_MIN || base > BASE_MAX || size < 2) return false;
    /* -INT64_MIN has no positive counterpart, so shift by one bit first. */
    u = negative ? (uint64_t)(-(v + 1)) + 1 : (uint64_t)v;

    do {
        tmp[n++] = DIGITS[u % (uint64_t)base];
        u /= (uint64_t)base;
    } while (u && n < (int)sizeof tmp);

    if (n + (negative ? 1 : 0) + 1 > size) return false;

    /* Digits come out backwards; reverse them into the caller's buffer. */
    {
        int out = 0;

        if (negative) buf[out++] = '-';
        while (n > 0) buf[out++] = tmp[--n];
        buf[out] = 0;
    }
    return true;
}

static bool twos_bits(int64_t v, int bits, char *buf, int size)
{
    if (bits != 8 && bits != 16 && bits != 32) return false;
    if (size < bits + bits / 4 + 1) return false;

    {
        uint64_t const mask = ((uint64_t)1 << bits) - 1;
        uint64_t const u = (uint64_t)v & mask;
        int out = 0;

        for (int i = bits - 1; i >= 0; i--) {
            buf[out++] = ((u >> i) & 1) ? '1' : '0';
            /* A space after every four digits, but not at the end. */
            if (i && i % 4 == 0) buf[out++] = ' ';
        }
        buf[out] = 0;
    }
    return true;
}

bool base_twos(int64_t v, int bits, char *buf, int size)
{
    return twos_bits(v, bits, buf, size);
}

bool base_twos_hex(int64_t v, int bits, char *buf, int size)
{
    uint64_t mask;
    int const digits = bits / 4;   /* two's complement is padded to the width */

    if (bits != 8 && bits != 16 && bits != 32) return false;
    if (size < digits + 1) return false;
    mask = ((uint64_t)1 << bits) - 1;
    {
        char raw[24];
        int n;

        if (!base_format((int64_t)((uint64_t)v & mask), 16, raw, (int)sizeof raw))
            return false;
        n = (int)strlen(raw);
        if (n > digits) return false;
        for (int i = 0; i < digits - n; i++) buf[i] = '0';
        memcpy(buf + digits - n, raw, (size_t)n + 1);
    }
    return true;
}

bool base_op(int64_t a, int64_t b, bit_op_t op, int64_t *out)
{
    if (!out) return false;

    switch (op) {
    case BIT_AND: *out = a & b; return true;
    case BIT_OR:  *out = a | b; return true;
    case BIT_XOR: *out = a ^ b; return true;
    case BIT_SHL:
        if (b < 0 || b >= 63) return false;
        *out = (int64_t)((uint64_t)a << b);
        return true;
    case BIT_SHR:
        if (b < 0 || b >= 63) return false;
        /* Logical shift, because a paper's >> is on unsigned patterns. */
        *out = (int64_t)((uint64_t)a >> b);
        return true;
    default:
        return false;
    }
}

int base_popcount(uint64_t v)
{
    int n = 0;

    while (v) {
        n += (int)(v & 1u);
        v >>= 1;
    }
    return n;
}

char const *base_digits(int base)
{
    static char text[40];
    int n = 0;

    if (base < BASE_MIN) base = BASE_MIN;
    if (base > BASE_MAX) base = BASE_MAX;
    for (int i = 0; i < base && n < (int)sizeof text - 1; i++)
        text[n++] = DIGITS[i];
    text[n] = 0;
    return text;
}
