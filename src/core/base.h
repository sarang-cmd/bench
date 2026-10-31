/* base.h - bases and bits (V2 feature F12).
 *
 * Spec: the V2 master prompt, F12.  Three things a computer science paper asks
 * for and a calculator's decimal display cannot show:
 *
 *      a number in base b, for b from 2 to 36
 *      the same number in two's complement at 8, 16 or 32 bits
 *      the bitwise operations, and a shift with a count
 *
 * Everything works on 64-bit integers; two's complement stops at 32 bits
 * because the screen shows three widths and the paper never asks for 64.
 *
 * No gint, no libm.
 */

#ifndef BENCH_BASE_H
#define BENCH_BASE_H

#include <stdbool.h>
#include <stdint.h>

#define BASE_MIN 2
#define BASE_MAX 36

typedef enum {
    BIT_AND = 0,
    BIT_OR,
    BIT_XOR,
    BIT_SHL,
    BIT_SHR,
    BIT_OPS
} bit_op_t;

char const *bit_op_name(bit_op_t op);   /* "AND", "OR", "XOR", "<<", ">>" */

/* Largest magnitude a two's complement width can hold, as a positive number. */
int64_t base_twos_max(int bits);

/* Parses `s` in `base`.  A leading '-' is allowed in any base.  Empty strings,
 * digits the base does not have and overflow all return false. */
bool base_parse(char const *s, int base, int64_t *out);

/* Writes `v` in `base`, upper case, with a leading '-' when negative.  Returns
 * false when the buffer is too small. */
bool base_format(int64_t v, int base, char *buf, int size);

/* Two's complement of `v` at `bits` bits: binary, with a space every four
 * digits so a student can convert to hex by eye.  `bits` is 8, 16 or 32. */
bool base_twos(int64_t v, int bits, char *buf, int size);

/* The same two's complement as hex, which is what the paper usually wants. */
bool base_twos_hex(int64_t v, int bits, char *buf, int size);

/* Bitwise operation; shifts use `b` as the count, and a shift of 64 or more is
 * refused rather than undefined. */
bool base_op(int64_t a, int64_t b, bit_op_t op, int64_t *out);

int base_popcount(uint64_t v);

/* True when `s` is a valid number in `base`. */
bool base_valid(char const *s, int base);

/* The largest base-specific digit for a key press, for the keypad hint. */
char const *base_digits(int base);

#endif /* BENCH_BASE_H */
