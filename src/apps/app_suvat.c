/* app_suvat.c - constant acceleration and projectiles (V2 feature F10).
 *
 * Spec: the V2 master prompt, F10.  Type three of s, u, v, a, t, and the screen
 * fills in the other two and shows the equations it used.  F2 switches to the
 * projectile view, which is the same maths drawn: the path, the range, the
 * peak and the time in the air.
 *
 *      SUVAT                        m  s  m/s
 *      s = 45  u = 0  v = 30
 *      a = 10  t = 3
 *      v = u + a t
 *      s = u t + a t^2 / 2
 *
 * Keys
 *   ( ) select a quantity, then digits . - to type it and EXE to keep it
 *   F1          start again
 *   F2          the projectile view
 *   F3          the sign of g: 9.81 or 10
 *   F5 / EXIT   help / home
 *
 * Nothing is written to the card. */

#include "app.h"
#include "app_help.h"

#include <stdio.h>
#include <string.h>

#include "core/mathfn.h"
#include "core/numparse.h"
#include "core/suvat.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum { SV_INPUT = 0, SV_PROJECTILE, SV_COUNT } stage_t;

static suvat_t state;
static su_var_t field = SU_S;
static char entry[16];
static stage_t stage = SV_INPUT;
static double speed = 20, angle = 30, height = 0, g = 9.81;
static int proj_field;      /* 0 speed, 1 angle, 2 height */

void app_suvat_init(void)
{
    memset(&state, 0, sizeof state);
    /* The prompt's worked example, ready to read. */
    state.v[SU_S] = 45;
    state.known[SU_S] = true;
    state.v[SU_U] = 0;
    state.known[SU_U] = true;
    state.v[SU_A] = 10;
    state.known[SU_A] = true;
    field = SU_S;
    entry[0] = 0;
    stage = SV_INPUT;
    speed = 20;
    angle = 30;
    height = 0;
    proj_field = 0;
}

int app_suvat_timeout(void)
{
    return 0;
}

/* ---- drawing ------------------------------------------------------------ */

static void fmt_num(double v, char *buf, int size)
{
    nt_fit(v, 3, 10, buf, size);
}

/* One "s = 45" cell: the name, then the value, the typed text or a question
 * mark.  A value the add-in worked out carries a small teal square, so the
 * student can tell what they gave from what they got without colour on text. */
/* Given, worked out, or still a question mark. */
static bool shown(su_var_t var)
{
    if (state.known[var]) return true;
    for (int i = 0; i < state.solved_count; i++)
        if (state.solved[i] == var) return true;
    return false;
}

static void cell(int x, int y, su_var_t var, bool derived)
{
    char line[32], b[16];

    snprintf(line, sizeof line, "%s = ", suvat_name(var));
    if (var == field && entry[0])
        strcat(line, entry);
    else if (var == field && !shown(var))
        strcat(line, "_");
    else if (!shown(var))
        strcat(line, "?");
    else
    {
        fmt_num(state.v[var], b, (int)sizeof b);
        strcat(line, b);
        strcat(line, " ");
        strcat(line, suvat_unit(var));
    }
    scr_text(x, y, line, TH_INK);

    if (derived && shown(var))
        scr_fill(x - 5, y + 4, 3, 3, TH_TEAL);
    if (var == field)
        scr_fill(x, y + TH_LINE - 2, scr_text_w(line), 2, TH_RED);
}

static void draw_input(void)
{
    char line[64];
    char m[80];
    int y = 22;

    /* Work the two unknowns out before anything is drawn: with three of the
     * five known this is what every key press recomputes anyway. */
    if (state.solved_count == 0)
        suvat_solve(&state, m, sizeof m);

    /* Fixed places even when a value is missing: s u v on one row, a t on the
     * next, in the order the prompt lists them. */
    for (int i = 0; i < SU_COUNT; i++)
    {
        bool derived = false;

        for (int k = 0; k < state.solved_count; k++)
            if (state.solved[k] == (su_var_t)i) derived = true;
        cell(TH_PAD + 6 + (i % 3) * 124, y + (i / 3) * (TH_LINE + 4),
             (su_var_t)i, derived);
    }
    y += 2 * (TH_LINE + 4) + 2;

    if (state.solved_count == 0)
    {
        scr_text(TH_PAD, y, m, TH_INK);
        snprintf(line, sizeof line, "type %s, then EXE", suvat_name(field));
        scr_text(TH_PAD, y + TH_LINE, line, TH_INK);
        return;
    }

    scr_text(TH_PAD, y, state.solved_count
                 ? "worked out, marked with a dot"
                 : "", TH_INK);
    y += TH_LINE + 4;
    for (int i = 0; i < state.eq_count && i < 3; i++)
        scr_text(TH_PAD, y + i * TH_LINE, state.eq[i], TH_INK);

    y += state.eq_count * TH_LINE + 2;
    if (state.two_times)
    {
        snprintf(line, sizeof line, "t = %.4g or %.4g s: first kept",
                 state.v[SU_T], state.other_time);
        scr_text(TH_PAD, y, line, TH_RED);
        y += TH_LINE;
    }

    snprintf(line, sizeof line, "g = %.2f m/s^2 on this screen; F2 draws the path",
             g);
    scr_text(TH_PAD, SCR_H - TH_SOFT_H - 2 * TH_LINE - 6, line, TH_INK);
    scr_text(TH_PAD, SCR_H - TH_SOFT_H - TH_LINE - 4,
             "arrows pick, digits type, EXE keeps", TH_INK);
}

/* The path: axes, the ground, the parabola, and the landing point. */
static void draw_arc(projectile_t const *p, int x, int y, int w, int h)
{
    double const vmax = (p->peak > 1 ? p->peak : 1) * 1.15;
    double const xmax = p->range > 0 ? p->range * 1.10 : 1;
    int const gy = y + h;   /* the ground line */
    int const gx = x + 2;
    int const ww = w - 6;
    int const hh = h - 6;
    int px = gx, py = gy;

    scr_fill(gx, gy - 1, ww, 1, TH_INK);         /* ground */
    scr_fill(gx, y + 2, 1, gy - y - 2, TH_SHADE); /* height axis */

    for (int i = 1; i <= 60; i++)
    {
        double const t = p->flight * i / 60.0;
        double const xx = p->vx * t;
        double const yy = height + p->vy * t - 0.5 * g * t * t;
        int const nx = gx + (int)(xx / xmax * ww);
        int const ny = gy - (int)(yy / vmax * hh);

        if (i > 1)
        {
            scr_line(px, py, nx, ny, TH_BLUE);
            scr_line(px, py + 1, nx, ny + 1, TH_BLUE);
        }
        px = nx;
        py = ny;
    }

    scr_fill(px - 2, py - 2, 5, 5, TH_RED);
    scr_frame(px - 2, py - 2, 5, 5, TH_INK, 1);

    /* The peak, marked, because it is the number students are asked for. */
    if (p->vy > 0)
    {
        int const tx = gx + (int)((p->vx * p->vy / g) / xmax * ww);
        int const ty = gy - (int)(p->peak / vmax * hh);

        scr_fill(tx - 1, ty - 1, 3, 3, TH_INK);
    }
}

static void draw_projectile(void)
{
    projectile_t p;
    char line[64], b[24];
    int y = 22;

    /* Three fields, one per line, so the active one can be underlined. */
    {
        char const *names[3] = { "speed", "angle", "height" };
        double const vals[3] = { speed, angle, height };
        char const *units[3] = { "m/s", "deg", "m" };

        for (int i = 0; i < 3; i++)
        {
            char row[48];

            if (proj_field == i && entry[0])
                snprintf(row, sizeof row, "%s %s %s", names[i], entry, units[i]);
            else
            {
                fmt_num(vals[i], b, (int)sizeof b);
                snprintf(row, sizeof row, "%s %s %s", names[i], b, units[i]);
            }
            scr_text(TH_PAD, y, row, TH_INK);
            if (proj_field == i)
                scr_fill(TH_PAD, y + TH_LINE - 2, scr_text_w(row), 2, TH_RED);
            y += TH_LINE;
        }
    }

    if (!projectile(speed, angle, height, g, &p))
    {
        scr_text(TH_PAD, y + 2, "These numbers have no flight path.", TH_RED);
        return;
    }

    y += 2;
    strcpy(line, "range ");
    fmt_num(p.range, b, (int)sizeof b);
    strcat(line, b);
    strcat(line, " m   peak ");
    fmt_num(p.peak, b, (int)sizeof b);
    strcat(line, b);
    strcat(line, " m");
    scr_text(TH_PAD, y, line, TH_INK);

    snprintf(line, sizeof line, "flight %.3f s   g %.2f", p.flight, g);
    scr_text(TH_PAD, y + TH_LINE, line, TH_INK);

    draw_arc(&p, TH_PAD, y + 2 * TH_LINE + 2, SCR_W - 2 * TH_PAD,
             SCR_H - (y + 2 * TH_LINE) - TH_SOFT_H - 8);
}

void app_suvat_draw(void)
{
    scr_clear(TH_PAPER);

    if (stage == SV_INPUT)
    {
        w_title("SUVAT", "three of five");
        draw_input();
        w_softkeys((char const *const[6]){ "Again", "Path", "g", "", "", "" }, -1);
    }
    else
    {
        {
            static char hint[16];

            snprintf(hint, sizeof hint, "g %.2f", g);
            w_title("Projectile", hint);
        }
        draw_projectile();
        w_softkeys((char const *const[6]){ "Again", "SUVAT", "g", "", "", "" }, -1);
    }
    scr_present();
}

/* ---- keys --------------------------------------------------------------- */

static void keep_entry(void)
{
    double v;

    if (!entry[0] || numparse_cstr(entry, &v) != NUMPARSE_OK)
        return;

    if (stage == SV_INPUT)
    {
        state.v[field] = v;
        state.known[field] = true;
        /* Re-solving while the student types is the whole point: the two
         * remaining values appear as soon as the third is known. */
        {
            char msg[48];

            suvat_solve(&state, msg, sizeof msg);
        }
        /* Move on to the next thing that is still unknown, so three values in
         * a row are three EXEs and no arrow keys. */
        for (int i = 1; i <= SU_COUNT; i++)
        {
            su_var_t const next = (su_var_t)((field + i) % SU_COUNT);

            if (!state.known[next])
            {
                field = next;
                break;
            }
        }
    }
    else if (proj_field == 0)
        speed = v;
    else if (proj_field == 1)
        angle = v;
    else
        height = v;

    entry[0] = 0;
}

static void clear_all(void)
{
    memset(&state, 0, sizeof state);
    entry[0] = 0;
    field = SU_S;
    stage = SV_INPUT;
}

app_result_t app_suvat_key(bench_event_t ev)
{
    if (ev.key == BK_MENU)
        return APP_QUIT;
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("suvat"));
        return APP_CONTINUE;
    }

    switch (ev.key)
    {
    case BK_EXE:
        keep_entry();
        break;
    case BK_DEL:
        if (entry[0])
            entry[strlen(entry) - 1] = 0;
        else if (stage == SV_INPUT)
        {
            state.known[field] = false;
            state.solved_count = 0;
            state.eq_count = 0;
        }
        break;
    case BK_LEFT:
    case BK_RIGHT:
    {
        int step = ev.key == BK_RIGHT ? 1 : -1;
        if (stage == SV_INPUT)
            field = (su_var_t)((field + step + SU_COUNT) % SU_COUNT);
        else
            proj_field = (proj_field + step + 3) % 3;
        entry[0] = 0;
        break;
    }
    case BK_DOWN:
    case BK_UP:
        if (stage == SV_INPUT)
            field = (su_var_t)((field + (ev.key == BK_DOWN ? 1 : -1) + SU_COUNT)
                               % SU_COUNT);
        else if (stage == SV_PROJECTILE)
            proj_field = (proj_field + (ev.key == BK_DOWN ? 1 : 2)) % 3;
        entry[0] = 0;
        break;
    case BK_MINUS:
        if (entry[0] == '-')
            strcpy(entry, entry + 1);
        else
        {
            memmove(entry + 1, entry, strlen(entry) + 1);
            entry[0] = '-';
        }
        break;
    case BK_DOT:
    case BK_DIGIT:
        if (strlen(entry) < sizeof entry - 2)
        {
            size_t const n = strlen(entry);

            entry[n] = ev.key == BK_DIGIT ? ev.ch : '.';
            entry[n + 1] = 0;
        }
        break;
    case BK_F1:
        clear_all();
        break;
    case BK_F2:
        stage = stage == SV_PROJECTILE ? SV_INPUT : SV_PROJECTILE;
        entry[0] = 0;
        break;
    case BK_F3:
        g = g > 9.9 ? 10.0 : 9.81;
        break;
    case BK_EXIT:
        return APP_BACK;
    default:
        break;
    }
    return APP_CONTINUE;
}
