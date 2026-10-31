/* app_triangle.c - the triangle solver (V2 feature F2).
 *
 * Spec: the V2 master prompt, F2.  Six fields, three across and two down:
 *
 *      a  b  c
 *      A  B  C
 *
 * Type what you know and leave the rest blank, then EXE.  With at least one
 * side and three values in all, the rest fills in.  The answer is shown in the
 * fields themselves, with the area and the perimeter underneath.
 *
 * The SSA case can have two triangles; when it does, the title bar says so and
 * F2 swaps between them.  Nothing is written to the card.
 *
 * Keys
 *   arrows     move between the six fields
 *   digits . - type into the field under the cursor, DEL removes
 *   EXE        solve
 *   F1         clear every field
 *   F2         the other triangle, when there are two
 *   F3 / F4    degrees / radians, shown in the title bar
 *   EXIT       back */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/numparse.h"
#include "core/triangle.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

#define FIELDS 6
#define FLEN 12

static char field[FIELDS][FLEN];
static int sel;
static bool solved;
static bool use_deg = true;
static int solutions;        /* 0 before solving, then 1 or 2 */
static int which;
static tri_t sol[2];
static char err[64];

static char const *angle_name(void)
{
    return use_deg ? "DEG" : "RAD";
}

void app_triangle_init(void)
{
    for (int i = 0; i < FIELDS; i++)
        field[i][0] = 0;
    /* The tool opens in whatever unit Settings names; F3/F4 still switch
     * for this session only and never write the settings file. */
    use_deg = app_cfg()->angle_unit != SETTINGS_ANGLE_RAD;
    sel = 0;
    solved = false;
    solutions = 0;
    which = 0;
    err[0] = 0;
}

int app_triangle_timeout(void)
{
    return 0;
}

/* ---- solving ------------------------------------------------------------ */

static bool read_number(char const *text, double *out)
{
    return text[0] && numparse_cstr(text, out) == NUMPARSE_OK;
}

static void solve(void)
{
    tri_t g;
    tri_t a, b;

    tri_clear(&g);
    err[0] = 0;
    for (int i = 0; i < FIELDS; i++)
    {
        double v = 0.0;

        if (!read_number(field[i], &v))
        {
            if (field[i][0])
            {
                strcpy(err, "Every value must be a number.");
                return;
            }
            continue;
        }
        if (i < 3)
        {
            g.has_side[i] = true;
            g.side[i] = v;
        }
        else
        {
            g.has_angle[i - 3] = true;
            g.angle[i - 3] = use_deg ? tri_rad(v) : v;
        }
    }

    tri_clear(&a);
    tri_clear(&b);
    solutions = tri_solve(&g, &a, &b);
    if (solutions < 0)
    {
        switch (solutions)
        {
        case TRI_ERR_SHAPE:
            strcpy(err, solutions == TRI_ERR_SHAPE
                            ? "Those values cannot make a triangle."
                            : "");
            break;
        case TRI_ERR_SINE:
            strcpy(err, "No triangle: the sine would pass 1.");
            break;
        case TRI_ERR_ARG:
            strcpy(err, "Sides and angles must be positive.");
            break;
        default:
            strcpy(err, "Give three values, at least one a side.");
            break;
        }
        solutions = 0;
        return;
    }
    sol[0] = a;
    sol[1] = b;
    which = 0;
    solved = true;
    err[0] = 0;
}

/* Fill the six boxes from the solution on show. */
static void write_back(void)
{
    tri_t const *t = &sol[which];

    for (int i = 0; i < 3; i++)
    {
        if (t->has_side[i])
            nt_fixed(t->side[i], 3, field[i], FLEN);
        if (t->has_angle[i])
            nt_fixed(use_deg ? tri_deg(t->angle[i]) : t->angle[i], 3,
                     field[3 + i], FLEN);
    }
}

/* ---- drawing ------------------------------------------------------------ */

static void draw_field(int i, int x, int y, int w)
{
    char const *label;
    char value[FLEN];

    if (i == 0)
        label = "a";
    else if (i == 1)
        label = "b";
    else if (i == 2)
        label = "c";
    else if (i == 3)
        label = "A";
    else if (i == 4)
        label = "B";
    else
        label = "C";

    /* An empty box says what it wants: a side or an angle, in the unit on
     * show. */
    if (field[i][0])
        strcpy(value, field[i]);
    else
        strcpy(value, i < 3 ? "side" : (use_deg ? "deg" : "rad"));

    w_field(x, y, w, TH_LINE + 8, label, value, i == sel);
}

void app_triangle_draw(void)
{
    int const x = TH_PAD;
    int const w = SCR_W - 2 * TH_PAD;
    int const fw = (w - 2 * TH_GAP) / 3;
    int y = TH_CONTENT_Y + TH_PAD;
    char line[64];

    scr_clear(TH_PAPER);

    if (solutions == 2)
        w_title("Triangle", "Two! F2 swaps");
    else
        w_title("Triangle", angle_name());

    for (int i = 0; i < 3; i++)
        draw_field(i, x + i * (fw + TH_GAP), y, fw);
    y += TH_LINE + 8 + TH_GAP;
    for (int i = 0; i < 3; i++)
        draw_field(3 + i, x + i * (fw + TH_GAP), y, fw);
    y += TH_LINE + 8 + 8;

    if (err[0])
    {
        scr_text_ellipsis(x, y, err, TH_INK, w);
        y += TH_LINE;
    }
    else if (!solved)
    {
        scr_text_ellipsis(x, y, "Fill in three, then EXE.", TH_INK, w);
        y += TH_LINE;
    }
    else
    {
        tri_t const *t = &sol[which];

        strcpy(line, "area ");
        {
            char n[24];

            nt_fixed(tri_area(t), 3, n, (int)sizeof n);
            nt_append(line, (int)sizeof line, n);
        }
        nt_append(line, (int)sizeof line, "   perimeter ");
        {
            char n[24];

            nt_fixed(tri_perimeter(t), 3, n, (int)sizeof n);
            nt_append(line, (int)sizeof line, n);
        }
        scr_text_ellipsis(x, y, line, TH_INK, w);
        y += TH_LINE;

        if (solutions == 2)
            scr_text_ellipsis(x, y, which == 0 ? "first triangle shown"
                                              : "second triangle shown",
                              TH_INK, w);
        y += TH_LINE;
    }

    w_softkeys((char const *const[6]){ "Clear", solutions == 2 ? "Other" : "",
                                       "DEG", "RAD", "", "" },
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
    if (solved)
        solved = false;
}

app_result_t app_triangle_key(bench_event_t ev)
{
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("triangle"));
        return APP_CONTINUE;
    }
    if (ev.key == BK_MENU)
        return APP_QUIT;

    switch (ev.key)
    {
    case BK_LEFT:
        if (sel > 0 && sel % 3 > 0)
            sel--;
        break;
    case BK_RIGHT:
        if (sel % 3 < 2)
            sel++;
        break;
    case BK_UP:
        if (sel >= 3)
            sel -= 3;
        break;
    case BK_DOWN:
        if (sel < 3)
            sel += 3;
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
        solved = false;
        break;
    }
    case BK_DEL:
    {
        char *f = field[sel];
        int const n = (int)strlen(f);

        if (n > 0)
            f[n - 1] = 0;
        solved = false;
        break;
    }
    case BK_EXE:
        solve();
        if (solved)
            write_back();
        break;
    case BK_F1:
        for (int i = 0; i < FIELDS; i++)
            field[i][0] = 0;
        solved = false;
        solutions = 0;
        err[0] = 0;
        break;
    case BK_F2:
        if (solutions == 2)
        {
            which = which == 0 ? 1 : 0;
            for (int i = 0; i < FIELDS; i++)
                field[i][0] = 0;
            write_back();
        }
        break;
    case BK_F3:
    case BK_F4:
        /* Switching units re-solves what is on screen, so the fields always
         * show the unit named in the title bar. */
        if (use_deg != (ev.key == BK_F3))
        {
            use_deg = ev.key == BK_F3;
            if (solved)
            {
                for (int i = 3; i < FIELDS; i++)
                    field[i][0] = 0;
                write_back();
            }
        }
        break;
    case BK_EXIT:
        return APP_BACK;
    default:
        break;
    }
    return APP_CONTINUE;
}
