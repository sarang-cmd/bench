/* app_seq.c - sequences, series and binomials (V2 feature F5).
 *
 * Spec: the V2 master prompt, F5.  Three screens:
 *
 *   arithmetic  a, d, n -> the nth term, the sum of n, and the terms
 *   geometric   a, r, n -> the nth term, the sum of n, the sum to infinity
 *                          when |r| < 1
 *   binomial    n, k    -> n choose k, and row n of Pascal's triangle
 *
 * Keys
 *   arrows      between the fields
 *   digits . -  type, DEL removes
 *   F1          clear
 *   F3 / F4     previous / next screen
 *   EXIT        back
 *
 * Nothing is written to the card. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/numparse.h"
#include "core/seq.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum { M_ARITH = 0, M_GEOM, M_BINOM, M_COUNT_ } app_mode_t;

static char const *const mode_name[M_COUNT_] = {
    "Arithmetic", "Geometric", "Binomial",
};

#define NFIELDS 3
#define FLEN 14

static app_mode_t mode;
static int sel;
static char field[NFIELDS][FLEN];

void app_seq_init(void)
{
    mode = M_ARITH;
    sel = 0;
    strcpy(field[0], "3");
    strcpy(field[1], "4");
    strcpy(field[2], "10");
}

int app_seq_timeout(void)
{
    return 0;
}

static bool value_of(int i, double *out)
{
    if (!field[i][0])
        return false;
    return numparse_cstr(field[i], out) == NUMPARSE_OK;
}

static int int_of(int i, int fallback)
{
    double v = 0.0;

    if (!value_of(i, &v))
        return fallback;
    return (int)(v < 0 ? v - 0.5 : v + 0.5);
}

/* ---- drawing ------------------------------------------------------------ */

static char const *label_of(int i)
{
    if (mode == M_BINOM)
        return i == 0 ? "n" : (i == 1 ? "k" : "");
    return i == 0 ? "a" : (i == 1 ? (mode == M_ARITH ? "d" : "r") : "n");
}

static void draw_fields(void)
{
    int const w = (SCR_W - 2 * TH_PAD - 2 * TH_GAP) / 3;

    for (int i = 0; i < (mode == M_BINOM ? 2 : 3); i++)
        w_field(TH_PAD + i * (w + TH_GAP), TH_CONTENT_Y + TH_PAD, w,
                TH_LINE + 8, label_of(i), field[i][0] ? field[i] : "-", i == sel);
}

/* Up to this many terms are listed under the sums. */
#define SHOW_TERMS 6

static void draw_arith(int y)
{
    double a = 0.0, d = 0.0;
    int const n = int_of(2, 0);
    char line[80];
    double terms[SHOW_TERMS];

    if (!value_of(0, &a) || !value_of(1, &d) || n < 1 || n > 1000)
    {
        scr_text_ellipsis(TH_PAD, y, "a, d and n (1 to 1000).", TH_INK,
                          SCR_W - 2 * TH_PAD);
        return;
    }

    strcpy(line, "term ");
    nt_i64(n, line + strlen(line), (int)sizeof line - (int)strlen(line));
    nt_append(line, (int)sizeof line, " = ");
    nt_fit(seq_arith_nth(a, d, n), -1, 12, line + strlen(line),
           (int)sizeof line - (int)strlen(line));
    scr_text_ellipsis(TH_PAD, y, line, TH_INK, SCR_W - 2 * TH_PAD);
    y += TH_LINE;

    strcpy(line, "sum = ");
    nt_fit(seq_arith_sum(a, d, n), -1, 12, line + strlen(line),
           (int)sizeof line - (int)strlen(line));
    scr_text_ellipsis(TH_PAD, y, line, TH_INK, SCR_W - 2 * TH_PAD);
    y += TH_LINE + 4;

    seq_arith_terms(a, d, n < SHOW_TERMS ? n : SHOW_TERMS, terms);
    {
        int const count = n < SHOW_TERMS ? n : SHOW_TERMS;

        line[0] = 0;
        for (int i = 0; i < count; i++)
        {
            if (i)
                nt_append(line, (int)sizeof line, ", ");
            nt_fit(terms[i], -1, 10, line + strlen(line),
                   (int)sizeof line - (int)strlen(line));
        }
        if (n > SHOW_TERMS)
            nt_append(line, (int)sizeof line, ", ...");
        scr_text_ellipsis(TH_PAD, y, line, TH_INK, SCR_W - 2 * TH_PAD);
    }
}

static void draw_geom(int y)
{
    double a = 0.0, r = 0.0, inf = 0.0;
    int const n = int_of(2, 0);
    char line[80];

    if (!value_of(0, &a) || !value_of(1, &r) || n < 1 || n > 1000)
    {
        scr_text_ellipsis(TH_PAD, y, "a, r and n (1 to 1000).", TH_INK,
                          SCR_W - 2 * TH_PAD);
        return;
    }

    strcpy(line, "term ");
    nt_i64(n, line + strlen(line), (int)sizeof line - (int)strlen(line));
    nt_append(line, (int)sizeof line, " = ");
    nt_fit(seq_geom_nth(a, r, n), -1, 12, line + strlen(line),
           (int)sizeof line - (int)strlen(line));
    scr_text_ellipsis(TH_PAD, y, line, TH_INK, SCR_W - 2 * TH_PAD);
    y += TH_LINE;

    strcpy(line, "sum = ");
    nt_fit(seq_geom_sum(a, r, n), -1, 12, line + strlen(line),
           (int)sizeof line - (int)strlen(line));
    scr_text_ellipsis(TH_PAD, y, line, TH_INK, SCR_W - 2 * TH_PAD);
    y += TH_LINE;

    if (seq_geom_infinite(a, r, &inf))
    {
        strcpy(line, "to infinity = ");
        nt_fit(inf, -1, 12, line + strlen(line),
               (int)sizeof line - (int)strlen(line));
    }
    else
        strcpy(line, "no sum to infinity: |r| >= 1");
    scr_text_ellipsis(TH_PAD, y, line, TH_INK, SCR_W - 2 * TH_PAD);
}

static void draw_binom(int y)
{
    int64_t v = 0;
    int const n = int_of(0, -1);
    int const k = int_of(1, -1);
    char line[80];

    if (n < 0 || k < 0 || n > SEQ_PASCAL_MAX)
    {
        scr_text_ellipsis(TH_PAD, y, "n and k, n up to 62.", TH_INK,
                          SCR_W - 2 * TH_PAD);
        return;
    }

    if (seq_binom(n, k, &v))
    {
        strcpy(line, "C(");
        nt_i64(n, line + strlen(line), (int)sizeof line - (int)strlen(line));
        nt_append(line, (int)sizeof line, ",");
        nt_i64(k, line + strlen(line), (int)sizeof line - (int)strlen(line));
        nt_append(line, (int)sizeof line, ") = ");
        nt_i64(v, line + strlen(line), (int)sizeof line - (int)strlen(line));
    }
    else
        strcpy(line, "k must be 0 to n, and C fits 64 bits.");
    scr_text_ellipsis(TH_PAD, y, line, TH_INK, SCR_W - 2 * TH_PAD);
    y += TH_LINE + 4;

    /* Row n, wrapped over as many lines as it takes. */
    {
        int64_t row[SEQ_PASCAL_MAX + 1];

        if (!seq_pascal_row(n, row, SEQ_PASCAL_MAX + 1))
        {
            scr_text_ellipsis(TH_PAD, y, "that row is too big to list", TH_INK,
                              SCR_W - 2 * TH_PAD);
            return;
        }
        line[0] = 0;
        for (int i = 0; i <= n; i++)
        {
            char nbuf[24];

            nt_i64(row[i], nbuf, (int)sizeof nbuf);
            if (line[0] && strlen(line) + strlen(nbuf) + 3 > 46)
            {
                scr_text(TH_PAD, y, line, TH_INK);
                y += TH_LINE;
                line[0] = 0;
            }
            if (line[0])
                nt_append(line, (int)sizeof line, " ");
            nt_append(line, (int)sizeof line, nbuf);
        }
        if (line[0])
            scr_text(TH_PAD, y, line, TH_INK);
    }
}

void app_seq_draw(void)
{
    int y = TH_CONTENT_Y + TH_PAD + TH_LINE + 8 + 8;

    scr_clear(TH_PAPER);
    w_title("Sequences", mode_name[mode]);
    draw_fields();

    switch (mode)
    {
    case M_ARITH: draw_arith(y); break;
    case M_GEOM: draw_geom(y); break;
    default: draw_binom(y); break;
    }

    w_softkeys((char const *const[6]){ "Clear", "", "Back", "Next", "", "" },
               -1);
    scr_present();
}

/* ---- keys --------------------------------------------------------------- */

static void type_digit(bench_event_t ev)
{
    char *f = field[sel];
    int const n = (int)strlen(f);
    char ch = 0;

    if (ev.key == BK_DIGIT)
        ch = ev.ch;
    else if (ev.key == BK_DOT)
        ch = '.';
    if (ch && n < FLEN - 1)
    {
        f[n] = ch;
        f[n + 1] = 0;
    }
}

app_result_t app_seq_key(bench_event_t ev)
{
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("seq"));
        return APP_CONTINUE;
    }
    if (ev.key == BK_MENU)
        return APP_QUIT;

    switch (ev.key)
    {
    case BK_LEFT:
        if (sel > 0)
            sel--;
        break;
    case BK_RIGHT:
        if (sel < (mode == M_BINOM ? 1 : 2))
            sel++;
        break;
    case BK_UP:
        sel = 0;
        break;
    case BK_DOWN:
        sel = mode == M_BINOM ? 1 : 2;
        break;
    case BK_DIGIT:
    case BK_DOT:
        type_digit(ev);
        break;
    case BK_MINUS:
    {
        char *f = field[sel];

        if (f[0] == '-')
            memmove(f, f + 1, strlen(f));
        else if ((int)strlen(f) < FLEN - 1)
        {
            memmove(f + 1, f, strlen(f) + 1);
            f[0] = '-';
        }
        break;
    }
    case BK_DEL:
    {
        char *f = field[sel];
        int const n = (int)strlen(f);

        if (n > 0)
            f[n - 1] = 0;
        break;
    }
    case BK_F1:
        for (int i = 0; i < NFIELDS; i++)
            field[i][0] = 0;
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
