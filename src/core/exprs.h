/* exprs.h - the six named expressions (V2 F1).
 *
 * Spec: the V2 master prompt, F1: "Store up to 6 named expressions f, g, h,
 * p, q, r in RAM.  Save them to Bench/state/exprs.txt only on exit from the
 * tool."
 *
 * The file is one "name=text" per line, in slot order, so it is readable and
 * editable on a PC:
 *
 *     f=x^2+1
 *     g=sin(x)
 *     h=
 *
 * Parsing is forgiving on purpose: a line with an unknown name, no '=', or
 * text too long to keep is skipped rather than throwing the file away, and a
 * missing file simply means six empty slots.
 */

#ifndef BENCH_EXPRS_H
#define BENCH_EXPRS_H

#define EXPR_SLOTS 6
#define EXPR_TEXT_MAX 64

typedef struct {
    char text[EXPR_TEXT_MAX];
} expr_slot_t;

/* "f", "g", "h", "p", "q", "r"; NULL outside 0..5. */
char const *exprs_name(int slot);

/* Fill `slots` from file text.  Returns how many lines were accepted.  All six
 * slots are cleared first, so a short file leaves empty slots rather than
 * stale text. */
int exprs_parse(char const *text, expr_slot_t slots[EXPR_SLOTS]);

/* Write the six slots as file text, always six lines.  Returns the length, or
 * -1 when the buffer is too small. */
int exprs_format(expr_slot_t const slots[EXPR_SLOTS], char *out, int size);

/* Copy `src` into slot, cutting to fit.  Returns the stored length. */
int exprs_set(expr_slot_t slots[EXPR_SLOTS], int slot, char const *src);

/* True when two sets of slots are identical (used to skip a write). */
int exprs_same(expr_slot_t const a[EXPR_SLOTS], expr_slot_t const b[EXPR_SLOTS]);

#endif /* BENCH_EXPRS_H */
