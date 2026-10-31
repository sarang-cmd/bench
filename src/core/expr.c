/* expr.c - see expr.h.  A shunting-yard parser and an RPN evaluator. */

#include "expr.h"

#include <stdbool.h>
#include <string.h>

#include "mathfn.h"

#define STACK_MAX 32
#define OUT_MAX 64

/* Functions live above the operator codes in the RPN queue, so the evaluator
 * can tell "add" from "sin" with one comparison. */
#define FN_BASE 100

/* Operator codes.  Values are ordered so that a plain comparison is not enough
 * of a shortcut: precedence is a separate table below. */
typedef enum {
    OP_ADD = 0,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_POW,
    OP_NEG,
} op_t;

/* Functions the engine knows, in the order they are matched. */
typedef enum {
    FN_SIN = 0,
    FN_COS,
    FN_TAN,
    FN_ASIN,
    FN_ACOS,
    FN_ATAN,
    FN_SINH,
    FN_COSH,
    FN_TANH,
    FN_EXP,
    FN_LN,
    FN_LOG,
    FN_SQRT,
    FN_ABS,
    FN_FLOOR,
    FN_CEIL,
    FN_NONE,
} fn_t;

static char const *const fn_names[] = {
    "sin",  "cos",  "tan",  "asin", "acos",  "atan", "sinh", "cosh",
    "tanh", "exp",  "ln",   "log",  "sqrt",  "abs",  "floor", "ceil",
};

/* One item of the operator stack: an operator or a function, both waiting for
 * their operands. */
typedef struct {
    bool is_fn;
    int code;       /* op_t or fn_t */
    int prec;       /* operators only */
    bool right;     /* operators only: right associative */
    int column;     /* where it started, for errors */
} pending_t;

/* One item of the output queue (RPN) or the value stack. */
typedef struct {
    bool is_op;
    int code;
    int column;
    double value;
} item_t;

/* ---- small helpers ------------------------------------------------------- */

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_space(char c)
{
    return c == ' ' || c == '\t';
}

static char lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

/* Case-insensitive comparison of a token against a lower-case name. */
static bool same_word(char const *a, int len, char const *b)
{
    int i;

    for (i = 0; i < len; i++)
        if (lower(a[i]) != b[i] || !b[i])
            return false;
    return b[len] == 0;
}

static int precedence(op_t op)
{
    switch (op)
    {
    case OP_ADD:
    case OP_SUB: return 1;
    case OP_MUL:
    case OP_DIV: return 2;
    case OP_NEG: return 3;
    case OP_POW: return 4;
    }
    return 0;
}

static bool right_assoc(op_t op)
{
    return op == OP_POW || op == OP_NEG;
}

static expr_result_t fail(expr_status_t status, int column)
{
    expr_result_t r;

    r.value = 0.0;
    r.status = status;
    r.column = column;
    r.token[0] = 0;
    return r;
}

static void remember_token(expr_result_t *r, char const *s, int len)
{
    int const n = len < (int)sizeof r->token - 1 ? len : (int)sizeof r->token - 1;

    if (len <= 0)
        return;
    memcpy(r->token, s, (size_t)n);
    r->token[n] = 0;
}

/* ---- the parser --------------------------------------------------------- */

typedef struct {
    pending_t ops[STACK_MAX];
    int ops_n;
    item_t out[OUT_MAX];
    int out_n;
} parser_t;

static bool push_op(parser_t *p, pending_t item)
{
    if (p->ops_n >= STACK_MAX)
        return false;
    p->ops[p->ops_n++] = item;
    return true;
}

static bool emit(parser_t *p, item_t item)
{
    if (p->out_n >= OUT_MAX)
        return false;
    p->out[p->out_n++] = item;
    return true;
}

static bool push_operator(parser_t *p, op_t op, int column)
{
    pending_t item;

    item.is_fn = false;
    item.code = (int)op;
    item.prec = precedence(op);
    item.right = right_assoc(op);
    item.column = column;

    /* A prefix minus has no left operand to bind: it is pushed, never popped
     * against.  Without this, 2^-1 emits the power before its right operand. */
    if (op == OP_NEG)
        return push_op(p, item);

    /* Pop everything that binds at least as tightly.  A right-associative
     * operator (^ and unary minus) stops at equal precedence. */
    while (p->ops_n > 0)
    {
        pending_t const *top = &p->ops[p->ops_n - 1];
        item_t o;

        if (top->is_fn)
            break;
        if (top->prec > item.prec || (top->prec == item.prec && !item.right))
        {
            o.is_op = true;
            o.code = top->code;
            o.column = top->column;
            o.value = 0.0;
            if (!emit(p, o))
                return false;
            p->ops_n--;
            continue;
        }
        break;
    }
    return push_op(p, item);
}

/* ---- token scanning ------------------------------------------------------ */

typedef enum {
    TOK_END = 0,
    TOK_NUMBER,
    TOK_NAME,
    TOK_OP,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_BAD,
} tok_kind_t;

typedef struct {
    tok_kind_t kind;
    double number;
    char const *text;   /* start of the token in the source */
    int len;
    int column;         /* 1-based */
} token_t;

static token_t next_token(char const *s, int *pos)
{
    token_t t;

    t.kind = TOK_END;
    t.number = 0.0;
    t.text = NULL;
    t.len = 0;
    t.column = *pos + 1;

    while (is_space(s[*pos]))
    {
        (*pos)++;
        t.column++;
    }
    t.text = s + *pos;
    if (!s[*pos])
        return t;

    if (is_digit(s[*pos]) || (s[*pos] == '.' && is_digit(s[*pos + 1])))
    {
        double value = 0.0;
        double scale = 0.1;
        bool fraction = false;

        t.kind = TOK_NUMBER;
        while (is_digit(s[*pos]))
            value = value * 10.0 + (double)(s[(*pos)++] - '0');
        if (s[*pos] == '.')
        {
            (*pos)++;
            fraction = true;
            while (is_digit(s[*pos]))
            {
                value += (double)(s[(*pos)++] - '0') * scale;
                scale *= 0.1;
            }
        }
        (void)fraction;
        /* Exponent: 1e-3, 2.5E6.  Only when a digit follows, so "2e" is a
         * number followed by the constant e. */
        if ((s[*pos] == 'e' || s[*pos] == 'E') &&
            (is_digit(s[*pos + 1]) ||
             ((s[*pos + 1] == '+' || s[*pos + 1] == '-') && is_digit(s[*pos + 2]))))
        {
            int sign = 1;
            int exp = 0;

            (*pos)++;
            if (s[*pos] == '+' || s[*pos] == '-')
                sign = (s[(*pos)++] == '-') ? -1 : 1;
            while (is_digit(s[*pos]))
                exp = exp * 10 + (s[(*pos)++] - '0');
            value *= bm_pow(10.0, (double)(sign * exp));
        }
        t.number = value;
        t.len = (int)(s + *pos - t.text);
        return t;
    }

    if (is_alpha(s[*pos]))
    {
        t.kind = TOK_NAME;
        while (is_alpha(s[*pos]) || is_digit(s[*pos]))
            (*pos)++;
        t.len = (int)(s + *pos - t.text);
        return t;
    }

    switch (s[*pos])
    {
    case '+': case '-': case '*': case '/': case '^':
        t.kind = TOK_OP;
        t.len = 1;
        (*pos)++;
        return t;
    case '(':
        t.kind = TOK_LPAREN;
        t.len = 1;
        (*pos)++;
        return t;
    case ')':
        t.kind = TOK_RPAREN;
        t.len = 1;
        (*pos)++;
        return t;
    default:
        t.kind = TOK_BAD;
        t.len = 1;
        (*pos)++;
        return t;
    }
}

/* The function named `text`, or FN_NONE. */
static fn_t find_fn(char const *text, int len)
{
    int i;

    for (i = 0; i < (int)FN_NONE; i++)
        if (same_word(text, len, fn_names[i]))
            return (fn_t)i;
    return FN_NONE;
}

/* ---- building the RPN queue --------------------------------------------- */

static expr_result_t build(char const *text, parser_t *p)
{
    int pos = 0;
    token_t t;
    bool expect_value = true;   /* at the start of the expression or after an
                                 * operator or '(' we expect a value */
    bool prev_value = false;    /* the previous token was a complete value */

    p->ops_n = 0;
    p->out_n = 0;

    for (;;)
    {
        t = next_token(text, &pos);

        if (t.kind == TOK_END)
            break;

        if (t.kind == TOK_BAD)
        {
            expr_result_t r = fail(EXPR_ERR_SYNTAX, t.column);

            remember_token(&r, t.text, t.len);
            return r;
        }

        /* Implicit multiplication: a value followed by something that starts
         * a value. */
        if (prev_value &&
            (t.kind == TOK_NUMBER || t.kind == TOK_LPAREN ||
             (t.kind == TOK_NAME && find_fn(t.text, t.len) == FN_NONE)))
        {
            if (!push_operator(p, OP_MUL, t.column))
                return fail(EXPR_ERR_DEPTH, t.column);
        }

        if (t.kind == TOK_NUMBER)
        {
            item_t v;

            v.is_op = false;
            v.code = 0;
            v.column = t.column;
            v.value = t.number;
            if (!emit(p, v))
                return fail(EXPR_ERR_DEPTH, t.column);
            expect_value = false;
            prev_value = true;
            continue;
        }

        if (t.kind == TOK_NAME)
        {
            fn_t const fn = find_fn(t.text, t.len);
            int const after = pos;

            if (fn != FN_NONE)
            {
                int look = after;

                while (is_space(text[look]))
                    look++;
                if (text[look] != '(')
                {
                    expr_result_t r = fail(EXPR_ERR_SYNTAX, t.column);

                    remember_token(&r, t.text, t.len);
                    return r;
                }
                {
                    pending_t item;

                    item.is_fn = true;
                    item.code = (int)fn;
                    item.prec = 0;
                    item.right = false;
                    item.column = t.column;
                    if (!push_op(p, item))
                        return fail(EXPR_ERR_DEPTH, t.column);
                }
                /* Consume the '(' here and let the group handler below push
                 * its marker, so the parenthesis is seen exactly once. */
                pos = look + 1;
                t.kind = TOK_LPAREN;
                t.text = text + look;
                t.len = 1;
                t.column = look + 1;
                /* fall through to the '(' case */
            }
            else if (same_word(t.text, t.len, "pi"))
            {
                item_t v;

                v.is_op = false;
                v.code = 0;
                v.column = t.column;
                v.value = BM_PI;
                if (!emit(p, v))
                    return fail(EXPR_ERR_DEPTH, t.column);
                expect_value = false;
                prev_value = true;
                continue;
            }
            else if (same_word(t.text, t.len, "e"))
            {
                item_t v;

                v.is_op = false;
                v.code = 0;
                v.column = t.column;
                v.value = BM_E;
                if (!emit(p, v))
                    return fail(EXPR_ERR_DEPTH, t.column);
                expect_value = false;
                prev_value = true;
                continue;
            }
            else if (same_word(t.text, t.len, "x"))
            {
                item_t v;

                v.is_op = false;
                v.code = 1;     /* 1 marks "x": the evaluator substitutes */
                v.column = t.column;
                v.value = 0.0;
                if (!emit(p, v))
                    return fail(EXPR_ERR_DEPTH, t.column);
                expect_value = false;
                prev_value = true;
                continue;
            }
            else
            {
                expr_result_t r = fail(EXPR_ERR_NAME, t.column);

                remember_token(&r, t.text, t.len);
                return r;
            }
        }

        if (t.kind == TOK_LPAREN)
        {
            pending_t item;

            item.is_fn = false;
            item.code = -1;     /* '(' marker */
            item.prec = 0;
            item.right = false;
            item.column = t.column;
            if (!push_op(p, item))
                return fail(EXPR_ERR_DEPTH, t.column);
            expect_value = true;
            prev_value = false;
            continue;
        }

        if (t.kind == TOK_RPAREN)
        {
            bool found = false;

            while (p->ops_n > 0)
            {
                pending_t *top = &p->ops[p->ops_n - 1];

                if (!top->is_fn && top->code == -1)
                {
                    found = true;
                    p->ops_n--;
                    break;
                }
                {
                    item_t o;

                    o.is_op = true;
                    o.code = top->is_fn ? FN_BASE + top->code : top->code;
                    o.column = top->column;
                    o.value = 0.0;
                    if (!emit(p, o))
                        return fail(EXPR_ERR_DEPTH, t.column);
                }
                p->ops_n--;
            }
            if (!found)
                return fail(EXPR_ERR_SYNTAX, t.column);
            /* A function immediately before the group takes it now. */
            if (p->ops_n > 0 && p->ops[p->ops_n - 1].is_fn)
            {
                item_t o;

                o.is_op = true;
                o.code = FN_BASE + p->ops[p->ops_n - 1].code;
                o.column = p->ops[p->ops_n - 1].column;
                o.value = 0.0;
                if (!emit(p, o))
                    return fail(EXPR_ERR_DEPTH, t.column);
                p->ops_n--;
            }
            expect_value = false;
            prev_value = true;
            continue;
        }

        /* TOK_OP */
        if (t.text[0] == '-' && expect_value)
        {
            if (!push_operator(p, OP_NEG, t.column))
                return fail(EXPR_ERR_DEPTH, t.column);
            expect_value = true;
            prev_value = false;
            continue;
        }
        if (t.text[0] == '+' && expect_value)
        {
            /* Unary plus is a no-op, but it is still a valid prefix. */
            expect_value = true;
            prev_value = false;
            continue;
        }
        if (expect_value)
        {
            expr_result_t r = fail(EXPR_ERR_SYNTAX, t.column);

            remember_token(&r, t.text, t.len);
            return r;
        }
        {
            op_t op;

            switch (t.text[0])
            {
            case '+': op = OP_ADD; break;
            case '-': op = OP_SUB; break;
            case '*': op = OP_MUL; break;
            case '/': op = OP_DIV; break;
            default: op = OP_POW; break;
            }
            if (!push_operator(p, op, t.column))
                return fail(EXPR_ERR_DEPTH, t.column);
        }
        expect_value = true;
        prev_value = false;
    }

    if (p->out_n == 0 && p->ops_n == 0)
        return fail(EXPR_ERR_EMPTY, 1);
    if (expect_value)
        return fail(EXPR_ERR_SYNTAX, pos + 1);

    /* Drain: everything left must be a complete operator. */
    while (p->ops_n > 0)
    {
        pending_t const *top = &p->ops[p->ops_n - 1];
        item_t o;

        if (!top->is_fn && top->code == -1)
            return fail(EXPR_ERR_SYNTAX, top->column);
        o.is_op = true;
        o.code = top->is_fn ? FN_BASE + top->code : top->code;
        o.column = top->column;
        o.value = 0.0;
        if (!emit(p, o))
            return fail(EXPR_ERR_DEPTH, top->column);
        p->ops_n--;
    }
    return fail(EXPR_OK, 0);
}

/* ---- evaluating the RPN queue ------------------------------------------- */

static double apply_fn(fn_t fn, double v, expr_angle_t angle)
{
    switch (fn)
    {
    case FN_SIN: return bm_sin(angle == EXPR_DEG ? bm_deg_to_rad(v) : v);
    case FN_COS: return bm_cos(angle == EXPR_DEG ? bm_deg_to_rad(v) : v);
    case FN_TAN: return bm_tan(angle == EXPR_DEG ? bm_deg_to_rad(v) : v);
    case FN_ASIN:
    {
        double const r = bm_asin(v);

        return angle == EXPR_DEG ? bm_rad_to_deg(r) : r;
    }
    case FN_ACOS:
    {
        double const r = bm_acos(v);

        return angle == EXPR_DEG ? bm_rad_to_deg(r) : r;
    }
    case FN_ATAN:
    {
        double const r = bm_atan(v);

        return angle == EXPR_DEG ? bm_rad_to_deg(r) : r;
    }
    case FN_SINH: return bm_sinh(v);
    case FN_COSH: return bm_cosh(v);
    case FN_TANH: return bm_tanh(v);
    case FN_EXP: return bm_exp(v);
    case FN_LN: return bm_ln(v);
    case FN_LOG: return bm_log10(v);
    case FN_SQRT: return bm_sqrt(v);
    case FN_ABS: return v < 0.0 ? -v : v;
    case FN_FLOOR: return bm_floor(v);
    case FN_CEIL: return bm_ceil(v);
    case FN_NONE: break;
    }
    return bm_nan();
}

static bool fn_is_domain_sensitive(fn_t fn)
{
    return fn == FN_SQRT || fn == FN_LN || fn == FN_LOG;
}

expr_result_t expr_eval(char const *text, double x, expr_angle_t angle)
{
    static parser_t p;      /* static: 1 KiB off the calculator's stack */
    expr_result_t r = build(text, &p);
    item_t stack[STACK_MAX];
    int n = 0;

    if (r.status != EXPR_OK)
        return r;

    for (int i = 0; i < p.out_n; i++)
    {
        item_t const *it = &p.out[i];

        if (!it->is_op)
        {
            if (n >= STACK_MAX)
                return fail(EXPR_ERR_DEPTH, it->column);
            stack[n] = *it;
            if (it->code == 1)      /* the variable x */
                stack[n].value = x;
            n++;
            continue;
        }

        if (it->code < FN_BASE && it->code != OP_NEG)
        {
            /* Binary operator. */
            double lhs, rhs;

            if (n < 2)
                return fail(EXPR_ERR_SYNTAX, it->column);
            rhs = stack[n - 1].value;
            lhs = stack[n - 2].value;
            n -= 2;
            switch ((op_t)it->code)
            {
            case OP_ADD: stack[n].value = lhs + rhs; break;
            case OP_SUB: stack[n].value = lhs - rhs; break;
            case OP_MUL: stack[n].value = lhs * rhs; break;
            case OP_DIV:
                if (rhs == 0.0)
                    return fail(EXPR_ERR_DIV_ZERO, it->column);
                stack[n].value = lhs / rhs;
                break;
            default:
                stack[n].value = bm_pow(lhs, rhs);
                break;
            }
            stack[n].is_op = false;
            stack[n].code = 0;
            stack[n].column = it->column;
            n++;
            continue;
        }

        if (it->code < FN_BASE && (op_t)it->code == OP_NEG)
        {
            if (n < 1)
                return fail(EXPR_ERR_SYNTAX, it->column);
            stack[n - 1].value = -stack[n - 1].value;
            continue;
        }

        /* Function. */
        if (n < 1)
            return fail(EXPR_ERR_SYNTAX, it->column);
        {
            fn_t const fn = (fn_t)(it->code - FN_BASE);
            double const arg = stack[n - 1].value;
            double value;

            if (fn == FN_SQRT && arg < 0.0)
                return fail(EXPR_ERR_DOMAIN, it->column);
            if ((fn == FN_LN || fn == FN_LOG) && arg <= 0.0)
                return fail(EXPR_ERR_DOMAIN, it->column);
            value = apply_fn(fn, arg, angle);
            if (bm_isnan(value))
            {
                if (fn_is_domain_sensitive(fn))
                    return fail(EXPR_ERR_DOMAIN, it->column);
                return fail(EXPR_ERR_DOMAIN, it->column);
            }
            stack[n - 1].value = value;
        }
    }

    if (n != 1)
        return fail(EXPR_ERR_SYNTAX, 0);
    if (!bm_isfinite(stack[0].value))
        return fail(EXPR_ERR_RANGE, stack[0].column);
    r.value = stack[0].value;
    r.status = EXPR_OK;
    r.column = 0;
    r.token[0] = 0;
    return r;
}

char const *expr_status_text(expr_status_t status)
{
    switch (status)
    {
    case EXPR_OK: return "ok";
    case EXPR_ERR_EMPTY: return "empty expression";
    case EXPR_ERR_SYNTAX: return "syntax error";
    case EXPR_ERR_NAME: return "unknown name";
    case EXPR_ERR_DIV_ZERO: return "divide by zero";
    case EXPR_ERR_DOMAIN: return "not a real number";
    case EXPR_ERR_RANGE: return "too large";
    case EXPR_ERR_DEPTH: return "too deeply nested";
    }
    return "error";
}
