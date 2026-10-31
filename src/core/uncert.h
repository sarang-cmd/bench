/* uncert.h - uncertainty arithmetic and display.
 *
 * Spec: docs/plan.md section 6.2.
 *
 * A quantity is a value and an absolute uncertainty, both double.  The maths is
 * double; everything that reaches the screen goes through numfmt, so the
 * display never needs float printf.
 *
 *   A + B   a+b                      da + db
 *   A - B   a-b                      da + db
 *   A * B   a*b                      |r| * (da/|a| + db/|b|)
 *   A / B   a/b                      |r| * (da/|a| + db/|b|)
 *   A ^ n   a^n                      |r| * |n| * da/|a|
 *   k * A   k*a                      |k| * da
 *
 * Plain C11, no gint. */

#ifndef BENCH_UNCERT_H
#define BENCH_UNCERT_H

#include <stdbool.h>

#define UNCERT_BUFSIZE 40

typedef struct {
    double value;
    double unc;   /* absolute uncertainty, >= 0 */
} uncert_qty_t;

typedef enum {
    UNCERT_OK = 0,
    UNCERT_ERR_ARG,        /* NULL pointer or non-finite input */
    UNCERT_ERR_NEG_UNC,    /* uncertainty < 0 */
    UNCERT_ERR_ZERO_A,     /* |a| == 0 where a relative uncertainty is needed */
    UNCERT_ERR_ZERO_B,     /* |b| == 0 in a division */
    UNCERT_ERR_NEG_BASE,   /* negative base with a non-integer exponent */
    UNCERT_ERR_OVERFLOW,   /* result is not finite */
} uncert_status_t;

typedef struct {
    uncert_qty_t q;
    uncert_status_t status;
} uncert_result_t;

typedef enum {
    UNCERT_OP_ADD = 0,
    UNCERT_OP_SUB,
    UNCERT_OP_MUL,
    UNCERT_OP_DIV,
    UNCERT_OP_POW,    /* b.value holds the exponent n */
    UNCERT_OP_SCALE,  /* b.value holds the constant k */
} uncert_op_t;

uncert_result_t uncert_add(uncert_qty_t a, uncert_qty_t b);
uncert_result_t uncert_sub(uncert_qty_t a, uncert_qty_t b);
uncert_result_t uncert_mul(uncert_qty_t a, uncert_qty_t b);
uncert_result_t uncert_div(uncert_qty_t a, uncert_qty_t b);
uncert_result_t uncert_pow(uncert_qty_t a, double n);
uncert_result_t uncert_scale(uncert_qty_t a, double k);

/* One entry point for the app's operation picker.  For UNCERT_OP_POW and
 * UNCERT_OP_SCALE only b.value is read. */
uncert_result_t uncert_apply(uncert_op_t op, uncert_qty_t a, uncert_qty_t b);

/* Short, printable reason for an error status ("no inverse", "value is 0"...).
 * Returns a static string, never NULL. */
char const *uncert_error_text(uncert_status_t status);

/* ---- display ------------------------------------------------------------ */

/* How many significant figures the uncertainty gets.
 *   UNCERT_SIG_ONE          always 1 (the default in the plan)
 *   UNCERT_SIG_ONE_OR_TWO   2 when the first digit is 1, else 1 */
typedef enum {
    UNCERT_SIG_ONE = 1,
    UNCERT_SIG_ONE_OR_TWO = 2,
} uncert_sig_mode_t;

typedef struct {
    char value[UNCERT_BUFSIZE];   /* value, rounded to the uncertainty's place */
    char unc[UNCERT_BUFSIZE];     /* absolute uncertainty */
    char pct[UNCERT_BUFSIZE];     /* percentage uncertainty, or "n/a" */
    int decimals;                 /* decimals shown after the point */
    int unc_sig;                  /* significant figures used for unc */
    bool pct_valid;
} uncert_display_t;

/* Round and render a quantity:
 *   1. round the uncertainty half-up to 1 sig fig (2 when the mode says so and
 *      the first digit is 1),
 *   2. round the value half-up to the same decimal place,
 *   3. render both, plus the percentage uncertainty (2 significant figures).
 * Returns UNCERT_OK or an error status (the buffers stay untouched on error). */
uncert_status_t uncert_display(uncert_qty_t q, uncert_sig_mode_t mode,
                               uncert_display_t *out);

#endif /* BENCH_UNCERT_H */
