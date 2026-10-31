/* expr.h - the expression engine (V2 feature F1).
 *
 * Spec: docs/plan.md and the V2 master prompt section 4, F1.  One engine is
 * shared: the Function table ships it first, and the root finder, the triangle
 * solver, the SUVAT solver and the graph tool all reuse it, so it lives in
 * core and knows nothing about screens.
 *
 * Grammar, in the order the parser applies it:
 *
 *   expression := term (('+' | '-') term)*
 *   term       := unary (('*' | '/' | implicit) unary)*
 *   unary      := '-' unary | power
 *   power      := primary ('^' unary)?
 *   primary    := number | name | name '(' expression ')' | '(' expression ')'
 *
 * Implicit multiplication is written where a value is followed by a value:
 * 3x, 2(x+1), (x+1)(x-1), 2pi.
 *
 * A shunting-yard parse, not a recursive one: the operator stack is a fixed
 * 32 entries, so a hostile expression cannot overflow the calculator's stack.
 * Errors carry the 1-based column of the token that failed, which is what the
 * entry screen points at.
 *
 * No libm and no gint: the maths is core/mathfn.c.
 */

#ifndef BENCH_EXPR_H
#define BENCH_EXPR_H

typedef enum {
    EXPR_OK = 0,
    EXPR_ERR_EMPTY,
    EXPR_ERR_SYNTAX,
    EXPR_ERR_NAME,
    EXPR_ERR_DIV_ZERO,
    EXPR_ERR_DOMAIN,
    EXPR_ERR_RANGE,
    EXPR_ERR_DEPTH,
} expr_status_t;

typedef enum {
    EXPR_DEG = 0,
    EXPR_RAD = 1,
} expr_angle_t;

typedef struct {
    double value;       /* meaningful only when status == EXPR_OK */
    expr_status_t status;
    int column;         /* 1-based column of the failure; 0 when OK */
    char token[16];     /* the offending name or operator, for the error line */
} expr_result_t;

/* Evaluate `text` with `x` bound (the Function table's column).  Trigonometry
 * and its inverses respect `angle`; everything else is unaffected. */
expr_result_t expr_eval(char const *text, double x, expr_angle_t angle);

/* "divide by zero", "unknown name", ...  Never NULL. */
char const *expr_status_text(expr_status_t status);

#endif /* BENCH_EXPR_H */
