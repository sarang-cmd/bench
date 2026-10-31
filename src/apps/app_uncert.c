/* app_uncert.c - the Uncertainty tool.
 *
 * Spec: docs/plan.md section 6.2, TASK-M4.
 *
 * Screens
 *   a      value and uncertainty of A (two fields)
 *   op     pick one of six operations
 *   b      value and uncertainty of B, or the exponent, or the constant
 *   result value, absolute uncertainty, percentage; F1 chains the result into A
 *
 * Keys
 *   a/b    UP/DOWN pick a field, digits and . type, '-' toggles the sign,
 *          DEL removes, EXE validates
 *   op     UP/DOWN pick, EXE apply, EXIT back
 *   result F1 chain, F2 edit A again, EXIT back to the operation picker
 *
 * Files: none. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/numparse.h"
#include "core/uncert.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum { SC_A, SC_OP, SC_B, SC_RESULT } screen_t;

static screen_t screen;
static uncert_qty_t a;
static uncert_qty_t b;
static uncert_op_t op;
static uncert_result_t result;
static uncert_display_t shown;
static bool chain;
static int field;
static int op_sel;
static char err[64];
static char a_val[16] = "10.0";
static char a_unc[16] = "0.1";
static char b_val[16] = "5.0";
static char b_unc[16] = "0.2";

static char const *const op_names[6] = {
    "A + B", "A - B", "A * B", "A / B", "A ^ n", "k * A",
};
static char const *const op_hints[6] = {
    "uncertainties add", "uncertainties add", "relative errors add",
    "relative errors add", "n is a whole number", "k is exact",
};

void app_uncert_init(void)
{
    screen = SC_A;
    field = 0;
    op_sel = 0;
    op = UNCERT_OP_ADD;
    chain = false;
    err[0] = 0;
    a.value = 10.0;
    a.unc = 0.1;
    b.value = 5.0;
    b.unc = 0.2;
    result.status = UNCERT_OK;
}

int app_uncert_timeout(void)
{
    return 0;
}

/* ---- helpers ------------------------------------------------------------ */

static char *cur_field(void)
{
    if (screen == SC_A)
        return field == 0 ? a_val : a_unc;
    return field == 0 ? b_val : b_unc;
}

static int cur_size(void)
{
    if (screen == SC_A)
        return field == 0 ? (int)sizeof a_val : (int)sizeof a_unc;
    return field == 0 ? (int)sizeof b_val : (int)sizeof b_unc;
}

static char const *op_label(int i)
{
    switch (i)
    {
    case 2:
    case 3: return "B";
    case 4: return "n";
    case 5: return "k";
    default: return "B";
    }
}

/* Parse both fields of the current screen into q. */
static bool read_pair(uncert_qty_t *q)
{
    char *v = screen == SC_A ? a_val : b_val;
    char *u = screen == SC_A ? a_unc : b_unc;

    if (numparse_cstr(v, &q->value) != NUMPARSE_OK)
    {
        strcpy(err, "The value is not a number");
        return false;
    }
    if (numparse_cstr(u, &q->unc) != NUMPARSE_OK)
    {
        strcpy(err, "The uncertainty is not a number");
        return false;
    }
    if (q->unc < 0.0)
    {
        strcpy(err, "An uncertainty cannot be negative");
        return false;
    }
    err[0] = 0;
    return true;
}

/* ---- screens ------------------------------------------------------------ */

static void draw_pair(char const *title, char const *sub)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int w = SCR_W - 2 * TH_PAD;
    int fh = TH_LINE + 8;

    scr_text(TH_PAD, y, sub, TH_INK);
    y += TH_LINE + 2;
    w_field(TH_PAD, y, w, fh, "Value  ",
            screen == SC_A ? a_val : b_val, field == 0);
    y += fh + TH_GAP;
    w_field(TH_PAD, y, w, fh, "+/-    ",
            screen == SC_A ? a_unc : b_unc, field == 1);
    y += fh + TH_GAP + 2;

    if (err[0])
        scr_text_ellipsis(TH_PAD, y, err, TH_INK, w);
    else
        scr_text(TH_PAD, y, "Same units for value and uncertainty.", TH_INK);

    w_strip(NULL, NULL);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "type digits   UP/DOWN field   EXE next", TH_INK);
    (void)title;
}

static void draw_op(void)
{
    int y = TH_CONTENT_Y + TH_PAD + 2;
    char num[24];

    nt_fit(a.value, 3, 10, num, (int)sizeof num);
    scr_text(TH_PAD, y, "A =", TH_INK);
    scr_text(TH_PAD + 26, y, num, TH_INK);
    nt_fit(a.unc, 3, 10, num, (int)sizeof num);
    scr_text(TH_PAD + 26 + 62, y, "+/-", TH_INK);
    scr_text(TH_PAD + 26 + 62 + 24, y, num, TH_INK);
    y += TH_LINE + 4;

    for (int i = 0; i < 6; i++)
        scr_text(TH_PAD + 6, y + i * TH_LINE,
                 op_names[i], i == op_sel ? TH_RED : TH_INK);

    /* Selection marker, drawn as a block so it carries across the row. */
    scr_fill(TH_PAD, y + op_sel * TH_LINE - 1, 3, TH_LINE, TH_RED);

    scr_text(SCR_W / 2 + 10, y + op_sel * TH_LINE, op_hints[op_sel], TH_INK);

    w_softkeys((char const *const[6]){ "Pick", "", "", "", "", "" }, -1);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "UP/DOWN choose   EXE apply   EXIT back", TH_INK);
}

static void draw_b(void)
{
    char const *lbl = op_label(op_sel);
    char sub[64];

    strcpy(sub, "This operand is ");
    nt_append(sub, (int)sizeof sub, lbl);
    nt_append(sub, (int)sizeof sub, ".");

    if (op_sel == 4)
    {
        /* Exponent: only the first field is meaningful. */
        int y = TH_CONTENT_Y + TH_PAD;
        int w = SCR_W - 2 * TH_PAD;
        int fh = TH_LINE + 8;

        scr_text(TH_PAD, y, "Exponent n (whole number or 0.5)", TH_INK);
        y += TH_LINE + 2;
        w_field(TH_PAD, y, w, fh, "n       ", b_val, true);
        y += fh + TH_GAP + 2;
        if (err[0])
            scr_text_ellipsis(TH_PAD, y, err, TH_INK, w);
        else
            scr_text(TH_PAD, y, "unc = |a^n| * |n| * da / |a|", TH_INK);
        w_strip(NULL, NULL);
        scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                        "type n   EXE apply   EXIT back", TH_INK);
        return;
    }
    draw_pair("B", sub);
}

static void draw_result(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int w = SCR_W - 2 * TH_PAD;
    char line[64];

    scr_text(TH_PAD, y, "Result", TH_INK);
    y += TH_LINE + 2;

    if (result.status != UNCERT_OK)
    {
        scr_fill(TH_PAD, y, w, TH_LINE + 6, TH_RED);
        scr_text(TH_PAD + 4, y + 3, uncert_error_text(result.status), TH_PAPER);
        y += TH_LINE + 12;
    }
    else
    {
        scr_fill(TH_PAD, y, w, TH_LINE * 2 + 10, TH_PAPER);
        scr_frame(TH_PAD, y, w, TH_LINE * 2 + 10, TH_INK, 2);
        strcpy(line, shown.value);
        nt_append(line, (int)sizeof line, "  +/-  ");
        nt_append(line, (int)sizeof line, shown.unc);
        scr_text_center(TH_PAD, w, y + 4, line, TH_INK);
        strcpy(line, "(");
        nt_append(line, (int)sizeof line, shown.pct_valid ? shown.pct : "n/a");
        nt_append(line, (int)sizeof line, " %)");
        scr_text_center(TH_PAD, w, y + 4 + TH_LINE, line, TH_INK);
        y += TH_LINE * 2 + 16;
    }

    if (chain)
        scr_text(TH_PAD, y, "Chained: this result is A", TH_INK);
    if (a.unc != 0.0 || a.value != 0.0)
    {
        /* nothing */
    }

    w_softkeys((char const *const[6]){ "Chain", "Edit A", "", "", "", "" }, -1);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "F1 chain   F2 edit   EXIT back", TH_INK);
}

/* ---- entry points ------------------------------------------------------- */

void app_uncert_draw(void)
{
    scr_clear(TH_PAPER);
    w_title("Uncertainty", "");
    switch (screen)
    {
    case SC_A: draw_pair("A", "First quantity (A)"); break;
    case SC_OP: draw_op(); break;
    case SC_B: draw_b(); break;
    default: draw_result(); break;
    }
    scr_present();
}

static void apply_op(void)
{
    char const *n = op_names[op_sel];

    op = (uncert_op_t)op_sel;
    (void)n;
    if (op == UNCERT_OP_POW)
    {
        double e = 0.0;

        if (numparse_cstr(b_val, &e) != NUMPARSE_OK)
        {
            strcpy(err, "The exponent is not a number");
            return;
        }
        err[0] = 0;
        result = uncert_pow(a, e);
    }
    else if (op == UNCERT_OP_SCALE)
    {
        double k = 0.0;

        if (numparse_cstr(b_val, &k) != NUMPARSE_OK)
        {
            strcpy(err, "The constant is not a number");
            return;
        }
        err[0] = 0;
        result = uncert_scale(a, k);
    }
    else
    {
        if (!read_pair(&b))
            return;
        result = uncert_apply(op, a, b);
    }

    if (result.status == UNCERT_OK)
    {
        uncert_sig_mode_t mode = app_cfg()->uncert_sig == 2
                                     ? UNCERT_SIG_ONE_OR_TWO
                                     : UNCERT_SIG_ONE;

        if (uncert_display(result.q, mode, &shown) != UNCERT_OK)
            result.status = UNCERT_ERR_ARG;
    }
    screen = SC_RESULT;
}

app_result_t app_uncert_key(bench_event_t ev)
{
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("uncert"));
        return APP_CONTINUE;
    }
    if (ev.key == BK_MENU)
        return APP_QUIT;
    if (ev.key == BK_EXIT)
    {
        switch (screen)
        {
        case SC_A: return APP_BACK;
        case SC_OP: screen = SC_A; break;
        case SC_B: screen = SC_OP; break;
        default: screen = SC_OP; break;
        }
        return APP_CONTINUE;
    }

    switch (screen)
    {
    case SC_A:
    case SC_B:
        if (ev.key == BK_UP || ev.key == BK_DOWN)
        {
            if (screen == SC_B && op_sel == 4)
                break;
            field = field == 0 ? 1 : 0;
            err[0] = 0;
        }
        else if (ev.key == BK_DIGIT || ev.key == BK_DOT)
        {
            char *buf = cur_field();
            int n = (int)strlen(buf);

            if (n < cur_size() - 1)
            {
                buf[n] = ev.key == BK_DIGIT ? ev.ch : '.';
                buf[n + 1] = 0;
            }
        }
        else if (ev.key == BK_MINUS)
        {
            char *buf = cur_field();

            if (buf[0] == '-')
            {
                memmove(buf, buf + 1, strlen(buf));
            }
            else if ((int)strlen(buf) < cur_size() - 1)
            {
                memmove(buf + 1, buf, strlen(buf) + 1);
                buf[0] = '-';
            }
        }
        else if (ev.key == BK_DEL)
        {
            char *buf = cur_field();
            int n = (int)strlen(buf);

            if (n > 0)
                buf[n - 1] = 0;
        }
        else if (ev.key == BK_EXE)
        {
            if (screen == SC_A)
            {
                if (read_pair(&a))
                    screen = SC_OP;
            }
            else
            {
                if (op_sel == 4)
                {
                    double e = 0.0;

                    if (numparse_cstr(b_val, &e) != NUMPARSE_OK)
                    {
                        strcpy(err, "The exponent is not a number");
                        break;
                    }
                }
                apply_op();
            }
        }
        break;

    case SC_OP:
        if (ev.key == BK_DOWN)
            op_sel = (op_sel + 1) % 6;
        else if (ev.key == BK_UP)
            op_sel = (op_sel + 5) % 6;
        else if (ev.key == BK_EXE)
        {
            err[0] = 0;
            if (op_sel == 0 || op_sel == 1 || op_sel == 2 || op_sel == 3)
                screen = SC_B;
            else if (op_sel == 4)
                screen = SC_B;
            else
                screen = SC_B;
            field = 0;
        }
        break;

    default:
        if (ev.key == BK_F1)
        {
            if (result.status == UNCERT_OK)
            {
                a = result.q;
                nt_fit(a.value, 6, 14, a_val, (int)sizeof a_val);
                nt_fit(a.unc, 6, 14, a_unc, (int)sizeof a_unc);
                chain = true;
                screen = SC_OP;
                op_sel = 0;
            }
        }
        else if (ev.key == BK_F2)
        {
            chain = false;
            screen = SC_A;
            field = 0;
        }
        else if (ev.key == BK_EXE)
        {
            chain = false;
            screen = SC_OP;
        }
        break;
    }
    return APP_CONTINUE;
}
