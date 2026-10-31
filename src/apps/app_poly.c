/* app_poly.c - Quadratics and polynomials (V2 feature F3).
 *
 * Spec: the V2 master prompt, F3.  Three screens over src/core/poly.c:
 *
 *   quadratic  a, b, c -> discriminant, both roots (real or complex), the
 *              vertex, and the factored form when the roots are rational.
 *   expand     (ax + b)^n up to degree 8, term by term.
 *   roots      a polynomial typed as coefficients c0..cn: the rational roots
 *              found by the rational root theorem, and what is left over.
 *
 * Polynomials are typed as coefficients, not as text: the calculator's keypad
 * has no letters to type "x^2 - 5x + 6" with, and guessing at the ALPHA
 * mapping is one of the things only the device can settle (AGENTS.md).  Every
 * coefficient is a number field driven by the digits.
 *
 * Keys
 *   UP/DOWN/LEFT/RIGHT  move between fields (LEFT/RIGHT on the degree line
 *                       change the degree in the roots screen)
 *   digits . -          type into the field under the cursor, DEL removes
 *   F1                  clear every field
 *   F2                  one more coefficient (roots screen)
 *   F3 / F4             previous / next screen
 *   EXIT                back to the home screen
 *
 * Nothing here writes a file. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/numfmt.h"
#include "core/numparse.h"
#include "core/poly.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum { M_QUAD = 0, M_EXPAND, M_ROOTS, M_COUNT_ } app_mode_t;

static char const *const mode_name[M_COUNT_] = {
    "Quadratic", "Expand", "Roots",
};

#define MAX_COEF 9          /* c0..c8 */
#define COEF_LEN 14

static app_mode_t mode;
static int sel;             /* the field under the cursor */

/* Quadratic and expand share three fields: a, b, c and a, b, n. */
static char abc[3][COEF_LEN];
static int degree;          /* roots mode: highest power, 1..8 */
static char coef[MAX_COEF][COEF_LEN];

static void reset_fields(void)
{
    strcpy(abc[0], "1");
    strcpy(abc[1], "-5");
    strcpy(abc[2], "6");
    degree = 3;
    strcpy(coef[0], "-6");
    strcpy(coef[1], "11");
    strcpy(coef[2], "-6");
    strcpy(coef[3], "1");
    for (int i = 4; i < MAX_COEF; i++)
        coef[i][0] = 0;
}

void app_poly_init(void)
{
    mode = M_QUAD;
    sel = 0;
    reset_fields();
}

int app_poly_timeout(void)
{
    return 0;
}

/* ---- fields ------------------------------------------------------------- */

static int field_count(void)
{
    return mode == M_ROOTS ? degree + 1 : 3;
}

static char *field_at(int i)
{
    if (mode == M_ROOTS)
        return i <= degree ? coef[i] : 0;
    return i >= 0 && i < 3 ? abc[i] : 0;
}

static char *cur_field(void)
{
    char *f = field_at(sel);

    if (!f)
    {
        sel = 0;
        f = field_at(0);
    }
    return f;
}

static double field_value(int i)
{
    double v = 0.0;

    if (numparse_cstr(field_at(i), &v) != NUMPARSE_OK)
        return 0.0;
    return v;
}

static bool field_ok(int i)
{
    double v;

    return numparse_cstr(field_at(i), &v) == NUMPARSE_OK;
}

/* ---- results ------------------------------------------------------------ */

/* One term of a polynomial, for the factored and remainder lines. */
static void poly_text(poly_t const *p, char *buf, int size)
{
    if (poly_format(p, buf, size) < 0)
        strcpy(buf, "too long");
}

/* The result block: up to four lines, and how many there are. */
static int quad_lines(char lines[4][56])
{
    double a = field_value(0), b = field_value(1), c = field_value(2);
    double disc = 0.0, r1re = 0.0, r1im = 0.0, r2re = 0.0, r2im = 0.0;
    char n1[24], n2[24];
    int count = 0;

    for (int i = 0; i < 4; i++)
        lines[i][0] = 0;

    if (!field_ok(0) || !field_ok(1) || !field_ok(2) || a == 0.0)
    {
        strcpy(lines[count++], "a, b and c must be numbers, a not 0.");
        return count;
    }

    poly_quadratic(a, b, c, &disc, &r1re, &r1im, &r2re, &r2im);

    strcpy(lines[count], "disc = ");
    nt_fit(disc, -1, 12, n1, (int)sizeof n1);
    nt_append(lines[count], (int)sizeof lines[count], n1);
    count++;

    if (disc >= 0.0)
    {
        strcpy(lines[count], "roots ");
        nt_fit(r1re, -1, 10, n1, (int)sizeof n1);
        nt_fit(r2re, -1, 10, n2, (int)sizeof n2);
        nt_append(lines[count], (int)sizeof lines[count], n1);
        nt_append(lines[count], (int)sizeof lines[count], ", ");
        nt_append(lines[count], (int)sizeof lines[count], n2);
    }
    else
    {
        strcpy(lines[count], "roots ");
        nt_fit(r1re, -1, 8, n1, (int)sizeof n1);
        nt_append(lines[count], (int)sizeof lines[count], n1);
        nt_append(lines[count], (int)sizeof lines[count], r1im >= 0 ? " + " : " - ");
        nt_fit(r1im >= 0 ? r1im : -r1im, -1, 8, n1, (int)sizeof n1);
        nt_append(lines[count], (int)sizeof lines[count], n1);
        nt_append(lines[count], (int)sizeof lines[count], "i");
    }
    count++;

    /* Vertex: x = -b/2a, y = the value there. */
    {
        double const vx = -b / (2.0 * a);
        double const vy = a * vx * vx + b * vx + c;

        strcpy(lines[count], "vertex (");
        nt_fit(vx, -1, 8, n1, (int)sizeof n1);
        nt_append(lines[count], (int)sizeof lines[count], n1);
        nt_append(lines[count], (int)sizeof lines[count], ", ");
        nt_fit(vy, -1, 8, n1, (int)sizeof n1);
        nt_append(lines[count], (int)sizeof lines[count], n1);
        nt_append(lines[count], (int)sizeof lines[count], ")");
    }
    count++;

    strcpy(lines[count], disc > 0.0 ? "two real roots"
                     : disc == 0.0 ? "one repeated root"
                                   : "a complex pair");
    count++;
    return count;
}

/* The factored form, when both roots are whole numbers. */
static void quad_factored(char *out, int size)
{
    double a = field_value(0);
    double r1re = 0.0, r1im = 0.0, r2re = 0.0, r2im = 0.0, disc = 0.0;
    char n[24];

    out[0] = 0;
    poly_quadratic(a, field_value(1), field_value(2), &disc, &r1re, &r1im,
                   &r2re, &r2im);
    if (r1im != 0.0 || r1re != (double)(long)r1re ||
        r2re != (double)(long)r2re)
        return;

    if (a != 1.0)
    {
        nt_fit(a, -1, 8, n, (int)sizeof n);
        nt_append(out, size, n);
    }
    nt_append(out, size, "(");
    nt_append(out, size, "x ");
    nt_append(out, size, r1re >= 0 ? "- " : "+ ");
    nt_fit(r1re >= 0 ? r1re : -r1re, -1, 8, n, (int)sizeof n);
    nt_append(out, size, n);
    if (r1re != r2re)
    {
        nt_append(out, size, ")(x ");
        nt_append(out, size, r2re >= 0 ? "- " : "+ ");
        nt_fit(r2re >= 0 ? r2re : -r2re, -1, 8, n, (int)sizeof n);
        nt_append(out, size, n);
    }
    nt_append(out, size, ")");
    if (r1re == r2re)
        nt_append(out, size, "^2");
}

/* ---- drawing ------------------------------------------------------------ */

static void draw_field_box(int i, int x, int y, int w)
{
    char value[COEF_LEN];
    char label[16];

    if (!field_ok(i) && field_at(i)[0])
        strcpy(value, "?");
    else
        strcpy(value, field_at(i));

    label[0] = 0;
    if (mode == M_QUAD)
        strcpy(label, i == 0 ? "a" : (i == 1 ? "b" : "c"));
    else if (mode == M_EXPAND)
        strcpy(label, i == 0 ? "a" : (i == 1 ? "b" : "n"));
    else
    {
        strcpy(label, "c");
        nt_i64(i, label + strlen(label), (int)sizeof label - (int)strlen(label));
    }
    w_field(x, y, w, TH_LINE + 8, label, value, i == sel);
}

static void draw_quad(void)
{
    int const x = TH_PAD;
    int const w = SCR_W - 2 * TH_PAD;
    int const lw = (w - TH_GAP) / 3;
    char lines[4][56];
    char fact[64];
    int y = TH_CONTENT_Y + TH_PAD;
    int const count = quad_lines(lines);

    for (int i = 0; i < 3; i++)
        draw_field_box(i, x + i * (lw + TH_GAP), y, lw);
    y += TH_LINE + 8 + 8;

    for (int i = 0; i < count; i++)
    {
        scr_text_ellipsis(x, y, lines[i], TH_INK, w);
        y += TH_LINE;
    }

    quad_factored(fact, (int)sizeof fact);
    if (fact[0])
    {
        y += 4;
        scr_fill(x, y, w, TH_LINE + 10, TH_PAPER);
        scr_frame(x, y, w, TH_LINE + 10, TH_INK, 2);
        scr_text_ellipsis(x + 5, y + 5, fact, TH_INK, w - 10);
    }
}

static void draw_expand(void)
{
    int const x = TH_PAD;
    int const w = SCR_W - 2 * TH_PAD;
    int const lw = (w - 2 * TH_GAP) / 3;
    int y = TH_CONTENT_Y + TH_PAD;
    poly_t p;
    char out[80];

    for (int i = 0; i < 3; i++)
        draw_field_box(i, x + i * (lw + TH_GAP), y, lw);
    y += TH_LINE + 8 + 8;

    {
        double const a = field_value(0);
        double const b = field_value(1);
        int const n = field_ok(2) ? (int)field_value(2) : -1;

        if (!field_ok(0) || !field_ok(1) || n < 0 || n > POLY_MAX_DEG ||
            poly_binomial(a, b, n, &p) != POLY_OK)
        {
            scr_text_ellipsis(x, y, "n must be a whole number, 0 to 8.", TH_INK,
                              w);
            return;
        }
    }

    scr_text(x, y, "(ax + b)^n =", TH_INK);
    y += TH_LINE + 4;
    poly_text(&p, out, (int)sizeof out);
    scr_fill(x, y, w, TH_LINE + 10, TH_PAPER);
    scr_frame(x, y, w, TH_LINE + 10, TH_INK, 2);
    scr_text_ellipsis(x + 5, y + 5, out, TH_INK, w - 10);
}

static void draw_roots(void)
{
    int const x = TH_PAD;
    int const w = SCR_W - 2 * TH_PAD;
    int const cw = (w - TH_GAP) / 2;
    int y = TH_CONTENT_Y + TH_PAD;
    char line[64];

    for (int i = 0; i <= degree; i++)
    {
        int const col = i % 2;
        int const row = i / 2;

        draw_field_box(i, x + col * (cw + TH_GAP), y + row * (TH_LINE + 10), cw);
    }
    y += ((degree + 1 + 1) / 2) * (TH_LINE + 10) + 6;

    /* The polynomial as it stands, then its rational roots. */
    {
        poly_t p, rest;
        double roots[POLY_MAX_ROOTS];
        int count = 0;

        for (int i = 0; i <= degree; i++)
            p.c[i] = field_value(i);
        p.deg = degree;
        poly_text(&p, line, (int)sizeof line);
        scr_text_ellipsis(x, y, line, TH_INK, w);
        y += TH_LINE;

        poly_factor(&p, roots, &count, &rest);
        if (count == 0)
            strcpy(line, "no rational roots");
        else
        {
            char n[24];

            strcpy(line, "roots ");
            for (int i = 0; i < count; i++)
            {
                if (i)
                    nt_append(line, (int)sizeof line, ", ");
                nt_fit(roots[i], 2, 8, n, (int)sizeof n);
                nt_append(line, (int)sizeof line, n);
            }
        }
        scr_text_ellipsis(x, y, line, TH_INK, w);
        y += TH_LINE;

        if (rest.deg < 1)
            strcpy(line, "divides completely");
        else
        {
            poly_text(&rest, line, (int)sizeof line);
            memmove(line + 8, line, strlen(line) + 1);
            memcpy(line, "left: ", 6);
        }
        scr_text_ellipsis(x, y, line, TH_INK, w);
    }
}

void app_poly_draw(void)
{
    scr_clear(TH_PAPER);
    w_title("Polynomials", mode_name[mode]);

    switch (mode)
    {
    case M_QUAD: draw_quad(); break;
    case M_EXPAND: draw_expand(); break;
    default: draw_roots(); break;
    }

    if (mode == M_ROOTS)
        w_softkeys((char const *const[6]){ "Clear", "Add c", "Back", "Next", "",
                                          "" }, -1);
    else
        w_softkeys((char const *const[6]){ "Clear", "", "Back", "Next", "", "" },
                   -1);
    scr_present();
}

/* ---- keys --------------------------------------------------------------- */

static void type_digit(char *buf, int size, bench_event_t ev)
{
    int const n = (int)strlen(buf);
    char ch = 0;

    if (ev.key == BK_DIGIT)
        ch = ev.ch;
    else if (ev.key == BK_DOT)
        ch = '.';
    if (ch && n < size - 1)
    {
        buf[n] = ch;
        buf[n + 1] = 0;
    }
}

app_result_t app_poly_key(bench_event_t ev)
{
    int const count = field_count();
    char *f = cur_field();

    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("poly"));
        return APP_CONTINUE;
    }
    if (ev.key == BK_MENU)
        return APP_QUIT;

    switch (ev.key)
    {
    case BK_UP:
        if (mode == M_ROOTS && sel >= 2)
            sel -= 2;
        else if (mode != M_ROOTS && sel > 0)
            sel--;
        break;
    case BK_DOWN:
        if (mode == M_ROOTS && sel + 2 < count)
            sel += 2;
        else if (mode != M_ROOTS && sel + 1 < count)
            sel++;
        break;
    case BK_LEFT:
        if (mode == M_ROOTS && sel >= 1)
            sel--;
        break;
    case BK_RIGHT:
        if (mode == M_ROOTS && sel + 1 < count)
            sel++;
        break;
    case BK_DIGIT:
    case BK_DOT:
        type_digit(f, COEF_LEN, ev);
        break;
    case BK_MINUS:
        if (f[0] == '-')
            memmove(f, f + 1, strlen(f));
        else if ((int)strlen(f) < COEF_LEN - 1)
        {
            memmove(f + 1, f, strlen(f) + 1);
            f[0] = '-';
        }
        break;
    case BK_DEL:
    {
        int const n = (int)strlen(f);

        if (n > 0)
            f[n - 1] = 0;
        break;
    }
    case BK_F1:
        if (mode == M_ROOTS)
            for (int i = 0; i <= degree; i++)
                coef[i][0] = 0;
        else
            for (int i = 0; i < 3; i++)
                abc[i][0] = 0;
        break;
    case BK_F2:
        /* Another coefficient: the degree goes 1..8 and then back to 1. */
        if (mode == M_ROOTS)
        {
            degree = degree >= POLY_MAX_DEG ? 1 : degree + 1;
            if (sel > degree)
                sel = degree;
        }
        break;
    case BK_F3:
        mode = (app_mode_t)((mode + M_COUNT_ - 1) % M_COUNT_);
        sel = 0;
        break;
    case BK_F4:
        mode = (app_mode_t)((mode + 1) % M_COUNT_);
        sel = 0;
        break;
    case BK_EXIT:
        return APP_BACK;
    default:
        break;
    }
    return APP_CONTINUE;
}
