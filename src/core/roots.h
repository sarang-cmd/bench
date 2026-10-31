/* roots.h - the root finder (V2 feature F4).
 *
 * Spec: the V2 master prompt, F4.  One equation, three methods, and the table
 * of iterates that makes convergence visible: a student should be able to see
 * the error fall, not just read the answer.
 *
 *   bisection  needs a sign change, halves the bracket, always converges
 *   Newton     needs a starting guess, uses a numeric derivative, is fastest
 *   secant     needs two guesses, no derivative at all
 *
 * The expression comes from the shared engine (core/expr.h), evaluated with x
 * bound and trigonometry in radians: a root finder works in radians whatever
 * the angle setting says, and the screen says so.
 *
 * No gint and no libm here.
 */

#ifndef BENCH_ROOTS_H
#define BENCH_ROOTS_H

/* 50 iterations is the prompt's cap, plus the starting guess. */
#define ROOT_MAX_STEPS 51

typedef enum {
    ROOT_OK = 0,
    ROOT_ERR_EXPR,        /* the expression did not parse; see `column` */
    ROOT_ERR_BRACKET,     /* f(a) and f(b) have the same sign */
    ROOT_ERR_FLAT,        /* the derivative is zero: the method cannot move */
    ROOT_ERR_NO_CONVERGE, /* 50 iterations and still short of the tolerance */
    ROOT_ERR_BAD_INTERVAL /* a == b, or a tolerance that is not positive */
} root_status_t;

typedef struct {
    int n;        /* iteration number, 1-based */
    double x;     /* the iterate */
    double fx;    /* f(x) */
    double delta; /* how far the estimate moved: |x_n - x_(n-1)|, or the half
                   * width of the bracket for bisection */
} root_step_t;

typedef struct {
    double root;      /* the best estimate, meaningful when status != ERR_EXPR */
    int count;        /* iterates recorded, 0..ROOT_MAX_STEPS */
    root_status_t status;
    int column;       /* 1-based column of a parse error, else 0 */
    root_step_t step[ROOT_MAX_STEPS];
} root_result_t;

/* default tolerance and iteration cap, so every screen agrees */
#define ROOT_TOL 1e-6
#define ROOT_MAX_ITER 50

/* h for the numeric derivative in Newton's method. */
#define ROOT_H 1e-6

/* Each returns the status and fills `out`; `out` may be NULL when only the
 * verdict matters.  `out->count` says how many rows the table has. */
root_status_t root_bisect(char const *f, double a, double b, double tol,
                          root_result_t *out);
root_status_t root_newton(char const *f, double x0, double tol,
                          root_result_t *out);
root_status_t root_secant(char const *f, double x0, double x1, double tol,
                          root_result_t *out);

/* "no sign change in the bracket", ...  Never NULL. */
char const *root_status_text(root_status_t status);

/* The same three methods but stopping at `max_iter` steps, for the tests. */
root_status_t root_bisect_max(char const *f, double a, double b, double tol,
                              int max_iter, root_result_t *out);
root_status_t root_newton_max(char const *f, double x0, double tol,
                              int max_iter, root_result_t *out);

#endif /* BENCH_ROOTS_H */
