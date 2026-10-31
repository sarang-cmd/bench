/* app_roots.c - the root finder (V2 feature F4).
 *
 * Spec: the V2 master prompt, F4.  One equation, three methods, and the table
 * of iterates so convergence is visible instead of asserted.
 *
 *      f = x^2-2                     +
 *      a 1      b 2                 |     *    <- f(x), with the iterates
 *      converged: 1.414214          |   *      marked
 *      n    x          f(x)         |
 *      1    1.500000   0.250000     +------------
 *      2    1.416667   0.006944
 *
 * Keys
 *   EXE on f   edit the expression on the keyboard
 *   UP/DOWN    pick the field, or scroll the table when it has focus
 *   digits . - type into a field, DEL removes
 *   F1 / F2    clear the fields / next method (bisection, Newton, secant)
 *   F3         solve
 *   F5 / EXIT  help / home
 *
 * Radians always: the readout says RAD no matter what the angle setting is. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/expr.h"
#include "core/numfmt.h"
#include "core/numparse.h"
#include "core/roots.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum { M_BISECT, M_NEWTON, M_SECANT, M_COUNT } method_t;

static char const *const method_name[M_COUNT] = { "Bisection", "Newton",
                                                  "Secant" };

#define FLEN 24

static char fn[48];
static char pa[FLEN];
static char pb[FLEN];
static method_t method;
static int sel;        /* 0 = f, 1 = first number, 2 = second number */
static int top;        /* first table row on screen */
static root_result_t res;
static bool solved;
static char note[48];

void app_roots_init(void)
{
    strcpy(fn, "x^2-2");
    strcpy(pa, "1");
    strcpy(pb, "2");
    method = M_BISECT;
    sel = 0;
    top = 0;
    solved = false;
    note[0] = 0;
    memset(&res, 0, sizeof res);
}

int app_roots_timeout(void)
{
    return 0;
}

/* ---- small helpers ------------------------------------------------------ */

static double num_of(char const *s, double fallback)
{
    double v = fallback;

    if (s[0])
        numparse_cstr(s, &v);
    return v;
}

static bool second_field(void)
{
    return method != M_NEWTON;
}

static char const *first_label(void)
{
    return method == M_BISECT ? "a" : "x0";
}

static char const *second_label(void)
{
    return method == M_BISECT ? "b" : "x1";
}

static void type_into(char *buf, int size, bench_event_t ev)
{
    int const n = (int)strlen(buf);

    if (ev.key == BK_DIGIT || ev.key == BK_DOT)
    {
        if (n < size - 1)
        {
            buf[n] = ev.key == BK_DIGIT ? ev.ch : '.';
            buf[n + 1] = 0;
        }
    }
    else if (ev.key == BK_MINUS)
    {
        if (buf[0] == '-')
            memmove(buf, buf + 1, (size_t)n);
        else if (n < size - 2)
        {
            memmove(buf + 1, buf, (size_t)n + 1);
            buf[0] = '-';
        }
    }
    else if (ev.key == BK_DEL && n > 0)
        buf[n - 1] = 0;
}

static char *field_at(int i)
{
    if (i == 0)
        return fn;
    return i == 1 ? pa : pb;
}

static void solve(void)
{
    double a = num_of(pa, 0.0), b = num_of(pb, 0.0);

    top = 0;
    solved = false;
    note[0] = 0;

    switch (method)
    {
    case M_BISECT: res.status = root_bisect(fn, a, b, ROOT_TOL, &res); break;
    case M_NEWTON: res.status = root_newton(fn, a, ROOT_TOL, &res); break;
    default: res.status = root_secant(fn, a, b, ROOT_TOL, &res); break;
    }
    solved = res.status == ROOT_OK;

    if (res.status == ROOT_ERR_EXPR)
        strcpy(note, "check the expression");
}

/* ---- the plot ----------------------------------------------------------- */

static double plotted_at(double x)
{
    expr_result_t r = expr_eval(fn, x, EXPR_RAD);

    if (r.status != EXPR_OK)
        return 0.0;
    return r.value;
}

static void draw_plot(int x0, int y0, int w, int h)
{
    double ymin, ymax, xmin, xmax;
    int i;

    scr_frame(x0, y0, w, h, TH_INK, 1);
    if (!solved && res.count == 0)
        return;

    /* The window is the range the method actually visited, widened by 25% so
     * the first iterate is not glued to the frame. */
    xmin = xmax = num_of(pa, 0.0);
    if (second_field())
    {
        double const b = num_of(pb, 1.0);

        if (b < xmin)
            xmin = b;
        if (b > xmax)
            xmax = b;
    }
    for (i = 0; i < res.count; i++)
    {
        if (res.step[i].x < xmin)
            xmin = res.step[i].x;
        if (res.step[i].x > xmax)
            xmax = res.step[i].x;
    }
    if (xmax - xmin < 1e-9)
    {
        xmin -= 1.0;
        xmax += 1.0;
    }
    xmin -= (xmax - xmin) * 0.25;
    xmax += (xmax - xmin) * 0.25;

    ymin = ymax = 0.0;
    for (i = 0; i <= 60; i++)
    {
        double const x = xmin + (xmax - xmin) * i / 60.0;
        double const y = plotted_at(x);

        if (y != y)
            continue;
        if (y < ymin)
            ymin = y;
        if (y > ymax)
            ymax = y;
    }
    if (ymax - ymin < 1e-9)
    {
        ymin -= 1.0;
        ymax += 1.0;
    }

    /* The axes, drawn first so the curve sits on top of them. */
    if (ymin < 0.0 && ymax > 0.0)
    {
        int const ay = y0 + h - 1 - (int)((0.0 - ymin) / (ymax - ymin) * (h - 2));

        for (i = 0; i < w - 2; i++)
            scr_fill(x0 + 1 + i, ay, 1, 1, TH_SHADE);
    }
    if (xmin < 0.0 && xmax > 0.0)
    {
        int const ax = x0 + 1 + (int)((0.0 - xmin) / (xmax - xmin) * (w - 2));

        for (i = 0; i < h - 2; i++)
            scr_fill(ax, y0 + 1 + i, 1, 1, TH_SHADE);
    }

    for (i = 0; i <= 80; i++)
    {
        double const x = xmin + (xmax - xmin) * i / 80.0;
        double const y = plotted_at(x);
        int px, py;

        if (y != y || y < ymin - (ymax - ymin) || y > ymax + (ymax - ymin))
            continue;
        px = x0 + 1 + (int)((x - xmin) / (xmax - xmin) * (w - 3));
        py = y0 + h - 1 - (int)((y - ymin) / (ymax - ymin) * (h - 2));
        scr_fill(px, py, 1, 1, TH_BLUE);
    }

    /* The iterates, and the root they are closing in on. */
    for (i = 0; i < res.count; i++)
    {
        double const x = res.step[i].x, y = res.step[i].fx;
        int px, py;

        if (y != y)
            continue;
        px = x0 + 1 + (int)((x - xmin) / (xmax - xmin) * (w - 3));
        py = y0 + h - 1 - (int)((y - ymin) / (ymax - ymin) * (h - 2));
        if (px > x0 + 1 && px < x0 + w - 3 && py > y0 + 1 && py < y0 + h - 3)
            scr_fill(px, py, 2, 2, TH_RED);
    }
}

/* ---- drawing ------------------------------------------------------------ */

#define TABLE_ROWS 6

/* f(x) in a column 11 characters wide: small numbers keep their digits
 * instead of rounding to 0, which is exactly the column a student reads to see
 * the method working.  Anything that will not fit as decimals goes scientific
 * rather than growing across the plot. */
static void fmt_fx(double v, char *buf, int size)
{
    numfmt_dec_t d;
    double const a = v < 0.0 ? -v : v;

    if (v == 0.0)
    {
        strcpy(buf, "0");
        return;
    }
    if ((a < 1e-3 || a >= 1e6) &&
        numfmt_dec_from_double(v, NUMFMT_DEFAULT_SIG, &d) == NUMFMT_OK &&
        numfmt_dec_render_sci(&d, 3, buf, size) > 0)
        return;
    nt_fit(v, 5, 11, buf, size);
    if (buf[0] == 0)
        strcpy(buf, "?");
}

static void draw_table(int x, int y, int w)
{
    char line[48], num[24];
    int i;

    scr_text(x, y, "n", TH_INK);
    scr_text(x + 18, y, "x", TH_INK);
    scr_text(x + 105, y, "f(x)", TH_INK);
    scr_fill(x, y + TH_LINE - 3, w, 1, TH_INK);
    y += TH_LINE;

    for (i = top; i < res.count && i < top + TABLE_ROWS; i++)
    {
        root_step_t const *s = &res.step[i];

        nt_i64(s->n, num, (int)sizeof num);
        strcpy(line, num);
        scr_text(x, y, line, TH_INK);

        nt_fixed(s->x, 6, num, (int)sizeof num);
        scr_text(x + 18, y, num, TH_INK);
        fmt_fx(s->fx, num, (int)sizeof num);
        scr_text(x + 105, y, num, TH_INK);
        y += TH_LINE;
    }


}

void app_roots_draw(void)
{
    int const w = (SCR_W - 2 * TH_PAD - TH_GAP) / 2;
    char line[64], num[24];
    int y = 24;

    scr_clear(TH_PAPER);
    w_title("Root finder", method_name[method]);
    w_field(TH_PAD, y, SCR_W - 2 * TH_PAD, 20, "f =", fn, sel == 0);
    y += 24;

    w_field(TH_PAD, y, w, 20, first_label(), pa, sel == 1);
    if (second_field())
        w_field(TH_PAD + w + TH_GAP, y, w, 20, second_label(), pb, sel == 2);
    else
        scr_text(TH_PAD + w + TH_GAP, y + 6, "one guess is enough", TH_INK);
    y += 24;

    /* The answer line: the number, or why there is not one. */
    if (res.count == 0 && res.status == ROOT_OK && !note[0])
        scr_text(TH_PAD, y, "F3 solves; EXE on f edits it", TH_INK);
    else if (res.status != ROOT_OK)
        scr_text(TH_PAD, y, root_status_text(res.status), TH_RED);
    else if (res.status == ROOT_OK)
    {
        strcpy(line, "root ");
        nt_fixed(res.root, 6, num, (int)sizeof num);
        strcat(line, num);
        strcat(line, "  (");
        nt_i64(res.count, num, (int)sizeof num);
        strcat(line, num);
        strcat(line, " steps)");
        scr_text(TH_PAD, y, line, TH_INK);
    }
    else
    {
        strcpy(line, root_status_text(res.status));
        scr_text(TH_PAD, y, line, TH_RED);
    }
    y += TH_LINE + 2;

    draw_table(TH_PAD, y, 200);
    draw_plot(TH_PAD + 200 + TH_GAP, y, SCR_W - 2 * TH_PAD - 200 - TH_GAP,
              SCR_H - TH_SOFT_H - y - 4);

    w_softkeys((char const *const[6]){ "Clear", "Method", "Solve", "", "", "" },
               -1);
    scr_present();
}

/* ---- keys --------------------------------------------------------------- */

app_result_t app_roots_key(bench_event_t ev)
{
    if (ev.key == BK_MENU)
        return APP_QUIT;
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("roots"));
        return APP_CONTINUE;
    }

    switch (ev.key)
    {
    case BK_UP:
        if (sel > 0)
            sel--;
        else if (top > 0)
            top--;
        break;
    case BK_DOWN:
        if (sel < (second_field() ? 2 : 1))
            sel++;
        else if (top + TABLE_ROWS < res.count)
            top++;
        break;
    case BK_LEFT:
        if (sel > 0)
            sel--;
        break;
    case BK_RIGHT:
        if (sel < 2 && second_field())
            sel++;
        break;
    case BK_EXE:
        if (sel == 0)
        {
            char buf[sizeof fn];

            strcpy(buf, fn);
            if (w_keyboard(buf, (int)sizeof buf, "f(x)"))
            {
                strcpy(fn, buf);
                res.count = 0;
                note[0] = 0;
            }
        }
        else
            solve();
        break;
    case BK_DIGIT:
    case BK_DOT:
    case BK_MINUS:
    case BK_DEL:
        if (sel > 0)
        {
            type_into(field_at(sel), FLEN, ev);
            res.count = 0;
            note[0] = 0;
        }
        break;
    case BK_F1:
        fn[0] = pa[0] = pb[0] = 0;
        res.count = 0;
        note[0] = 0;
        break;
    case BK_F2:
        method = (method_t)((method + 1) % M_COUNT);
        if (sel > (second_field() ? 0 : 1))
            sel = 0;
        res.count = 0;
        note[0] = 0;
        break;
    case BK_F3:
        solve();
        break;
    case BK_EXIT:
        return APP_BACK;
    default:
        break;
    }
    return APP_CONTINUE;
}
