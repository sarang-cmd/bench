/* app_cplx.c - complex numbers (V2 feature F6).
 *
 * Spec: the V2 master prompt, F6.  Two complex numbers a and b, one operation
 * at a time, and the result written three ways: rectangular, modulus and
 * argument, polar.
 *
 *      a = [ re ] + [ im ] i
 *      b = [ re ] + [ im ] i
 *      = -1 + 2i
 *      |z| 2.24    arg 116.57 deg
 *
 * Keys
 *   arrows     move between the four boxes
 *   digits . - type, DEL removes
 *   F1         clear all four
 *   F2         the operation: a+b, a-b, a*b, a/b, a^i
 *   F3 / F4    argument in degrees / radians, shown in the title bar
 *   EXIT       home
 *
 * Nothing is written to the card. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/cplx.h"
#include "core/numparse.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum { OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_POW, OP_COUNT } op_t;

static char const *const op_name[OP_COUNT] = { "a + b", "a - b", "a x b",
                                               "a / b", "a ^ n" };

#define FLEN 16

static char field[4][FLEN];
static int sel;
static op_t op;
static bool use_deg = true;

void app_cplx_init(void)
{
    strcpy(field[0], "3");
    strcpy(field[1], "4");
    strcpy(field[2], "1");
    strcpy(field[3], "-2");
    sel = 0;
    op = OP_MUL;
}

int app_cplx_timeout(void)
{
    return 0;
}

static double value(int i)
{
    double v = 0.0;

    if (!field[i][0] || numparse_cstr(field[i], &v) != NUMPARSE_OK)
        return 0.0;
    return v;
}

static bool valid(int i)
{
    double v;

    return field[i][0] && numparse_cstr(field[i], &v) == NUMPARSE_OK;
}

static cplx_t a_value(void)
{
    return cx(value(0), value(1));
}

static cplx_t b_value(void)
{
    return cx(value(2), value(3));
}

static cplx_t result(void)
{
    switch (op)
    {
    case OP_ADD: return cplx_add(a_value(), b_value());
    case OP_SUB: return cplx_sub(a_value(), b_value());
    case OP_MUL: return cplx_mul(a_value(), b_value());
    case OP_DIV: return cplx_div(a_value(), b_value());
    default: return cplx_powi(a_value(), (int)(value(2) < 0 ? value(2) - 0.5 : value(2) + 0.5));
    }
}

/* ---- drawing ------------------------------------------------------------ */

static void draw_box(int i, int x, int y, int w, char const *label)
{
    /* An empty box stays empty: writing 0 there would claim a value the
     * user never gave. */
    w_field(x, y, w, w_field_h(), label, field[i], i == sel);
}

void app_cplx_draw(void)
{
    int const w = (SCR_W - 2 * TH_PAD - TH_GAP) / 2;
    int const x2 = TH_PAD + w + TH_GAP;
    char line[64];
    char num[32];
    cplx_t z;
    bool ok = valid(0) && valid(1) && valid(2) && valid(3);
    int y = 26;

    scr_clear(TH_PAPER);
    w_title("Complex", op_name[op]);

    draw_box(0, TH_PAD, y, w, "a re");
    draw_box(1, x2, y, w, "a im");
    y += 24;
    draw_box(2, TH_PAD, y, w, "b re");
    draw_box(3, x2, y, w, "b im");
    y += 28;

    if (!ok)
    {
        scr_text(TH_PAD, y, "Fill all four boxes.", TH_INK);
        w_softkeys((char const *const[6]){ "Clear", "Op", "Deg", "Rad", "", "" },
                   -1);
        scr_present();
        return;
    }

    z = result();

    if (cplx_format(z, 6, line, (int)sizeof line) < 0)
        strcpy(line, "not a number");
    scr_text(TH_PAD, y, "= ", TH_INK);
    scr_text(TH_PAD + 13, y, line, TH_INK);
    y += TH_LINE + 4;

    if (!cplx_is_nan(z))
    {
        double const arg = cplx_arg(z);
        double const shown = use_deg ? arg * 57.29577951308232 : arg;

        strcpy(line, "|z| ");
        nt_fixed(cplx_abs(z), 3, num, (int)sizeof num);
        strcat(line, num);
        strcat(line, "   arg ");
        nt_fixed(shown, 2, num, (int)sizeof num);
        strcat(line, num);
        if (use_deg)
            strcat(line, " deg");
        scr_text(TH_PAD, y, line, TH_INK);
        y += TH_LINE;

        /* Polar form, with the same numbers: 2.24 (116.57 deg). */
        strcpy(line, "polar ");
        nt_fixed(cplx_abs(z), 3, num, (int)sizeof num);
        strcat(line, num);
        strcat(line, "  angle ");
        nt_fixed(shown, 2, num, (int)sizeof num);
        strcat(line, num);
        if (use_deg)
            strcat(line, " deg");
        scr_text(TH_PAD, y, line, TH_INK);
    }

    w_softkeys((char const *const[6]){ "Clear", "Op", "Deg", "Rad", "", "" }, -1);
    scr_present();
}

/* ---- keys --------------------------------------------------------------- */

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

app_result_t app_cplx_key(bench_event_t ev)
{
    if (ev.key == BK_MENU)
        return APP_QUIT;
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("cplx"));
        return APP_CONTINUE;
    }

    switch (ev.key)
    {
    case BK_LEFT:
        if (sel % 2 == 1)
            sel--;
        break;
    case BK_RIGHT:
        if (sel % 2 == 0)
            sel++;
        break;
    case BK_UP:
        if (sel >= 2)
            sel -= 2;
        break;
    case BK_DOWN:
        if (sel < 2)
            sel += 2;
        break;
    case BK_DIGIT:
    case BK_DOT:
    case BK_MINUS:
    case BK_DEL:
        type_into(field[sel], FLEN, ev);
        break;
    case BK_F1:
        for (int i = 0; i < 4; i++)
            field[i][0] = 0;
        break;
    case BK_F2:
        op = (op_t)((op + 1) % OP_COUNT);
        break;
    case BK_F3:
        use_deg = true;
        break;
    case BK_F4:
        use_deg = false;
        break;
    case BK_EXIT:
        return APP_BACK;
    default:
        break;
    }
    return APP_CONTINUE;
}
