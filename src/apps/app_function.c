/* app_function.c - the Function table (V2 feature F1).
 *
 * Spec: the V2 master prompt, F1: expression entry on the on-screen keyboard,
 * a table of x and f(x) with start, step and count, a one-line value probe,
 * six named expressions f, g, h, p, q, r in RAM, saved to
 * Bench/state/exprs.txt only when the tool is left.
 *
 * Screens
 *   list    the six slots and their expressions
 *   table   x and f(x) for the current start, step and count
 *   probe   one value, large, for the row under the cursor
 *
 * Keys
 *   list    UP/DOWN pick a slot, EXE edit it, F1 table, F3 clear, F4 angle
 *   table   UP/DOWN pick a row, LEFT/RIGHT move the start, F1 step, F2 count,
 *           EXE probe, EXIT back to the list
 *   probe   EXE or EXIT back to the table
 *
 * The angle mode is shown in the title bar of every screen.
 *
 * Files: state/exprs.txt, written once when the tool is left, and only when a
 * slot changed. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/expr.h"
#include "core/exprs.h"
#include "platform/screen.h"
#include "platform/store.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum { SC_LIST = 0, SC_TABLE, SC_PROBE } screen_t;

#define STEPS 6
#define ROWS 11

static double const steps[STEPS] = { 0.1, 0.5, 1.0, 2.0, 5.0, 10.0 };

static screen_t screen;
static expr_slot_t slots[EXPR_SLOTS];
static expr_slot_t saved[EXPR_SLOTS];
static int sel;          /* slot on the list screen */
static int row;          /* row under the cursor on the table screen */
static int row_top;      /* first visible row */
static double start;
static int step_idx;
static int count;
static expr_angle_t angle;
static char err[96];

static char const *angle_name(void)
{
    return angle == EXPR_DEG ? "DEG" : "RAD";
}

/* ---- the file ------------------------------------------------------------ */

static void load(void)
{
    char buf[EXPR_SLOTS * (EXPR_TEXT_MAX + 2) + 2];
    int len = 0;

    int rc = store_read("state/exprs.txt", buf, (int)sizeof buf - 1, &len);

    if (rc != STORE_OK || len <= 0)
    {
        for (int i = 0; i < EXPR_SLOTS; i++)
            slots[i].text[0] = 0;
    }
    else
    {
        buf[len] = 0;
        exprs_parse(buf, slots);
    }
    for (int i = 0; i < EXPR_SLOTS; i++)
        slots[i].text[EXPR_TEXT_MAX - 1] = 0;
    memcpy(saved, slots, sizeof saved);
}

static void save_if_changed(void)
{
    char buf[EXPR_SLOTS * (EXPR_TEXT_MAX + 2) + 2];
    int len;

    if (exprs_same(slots, saved))
        return;
    len = exprs_format(slots, buf, (int)sizeof buf);
    if (len > 0 && store_write("state/exprs.txt", buf, len) == STORE_OK)
        memcpy(saved, slots, sizeof saved);
}

/* ---- evaluation ---------------------------------------------------------- */

/* f(start + i * step) for the selected slot, or the error line. */
static bool eval_row(int i, double *out)
{
    double const x = start + (double)i * steps[step_idx];
    expr_result_t r;

    if (slots[sel].text[0] == 0)
    {
        strcpy(err, "This slot is empty. Press EXE on the list to type one.");
        return false;
    }
    r = expr_eval(slots[sel].text, x, angle);
    if (r.status != EXPR_OK)
    {
        strcpy(err, expr_status_text(r.status));
        if (r.token[0])
        {
            nt_append(err, (int)sizeof err, ": ");
            nt_append(err, (int)sizeof err, r.token);
        }
        else if (r.column > 0)
        {
            char col[8];

            nt_append(err, (int)sizeof err, " at column ");
            nt_i64(r.column, col, (int)sizeof col);
            nt_append(err, (int)sizeof err, col);
        }
        return false;
    }
    *out = r.value;
    err[0] = 0;
    return true;
}

/* ---- drawing ------------------------------------------------------------- */

static void draw_list(void)
{
    int const y0 = TH_CONTENT_Y + TH_PAD;
    int const w = SCR_W - 2 * TH_PAD;

    for (int i = 0; i < EXPR_SLOTS; i++)
    {
        int y = y0 + i * (TH_LINE + 4);
        char line[EXPR_TEXT_MAX + 4];

        strcpy(line, exprs_name(i));
        nt_append(line, (int)sizeof line, "  ");
        if (slots[i].text[0])
            nt_append(line, (int)sizeof line, slots[i].text);
        else
            nt_append(line, (int)sizeof line, "empty");
        w_row(TH_PAD, y, w, line, "", i == sel);
    }

    /* The hint sits under the list, not at the bottom of an empty page. */
    scr_text_ellipsis(TH_PAD, y0 + EXPR_SLOTS * (TH_LINE + 4) + 6,
                      err[0] ? err : "EXE edits a slot; F1 tables it.",
                      TH_INK, w);
    w_softkeys((char const *const[6]){ "Table", "", "Clear", "Angle", "", "" },
               -1);
}

static void draw_table(void)
{
    int const y0 = TH_CONTENT_Y + TH_PAD + TH_LINE;
    int const rows = ROWS;
    int const w = SCR_W - 2 * TH_PAD;
    char head[48];
    char num[24];

    /* Column headings, then the rows. */
    scr_text(TH_PAD, TH_CONTENT_Y + TH_PAD, "x", TH_INK);
    scr_text_right(SCR_W - TH_PAD, TH_CONTENT_Y + TH_PAD, "f(x)", TH_INK);
    scr_hline(TH_PAD, y0 - 3, w, TH_SHADE);

    for (int i = 0; i < rows; i++)
    {
        int const idx = row_top + i;
        double fx = 0.0;
        int const ok = eval_row(idx, &fx);
        int const y = y0 + i * TH_LINE;

        nt_fit(start + (double)idx * steps[step_idx], 3, 12, num,
               (int)sizeof num);
        scr_text_ellipsis(TH_PAD, y, num, TH_INK, 90);
        if (ok)
        {
            nt_fit(fx, 4, 14, num, (int)sizeof num);
            scr_text_right(SCR_W - TH_PAD, y, num, TH_INK);
        }
        else
            scr_text_right(SCR_W - TH_PAD, y, err, TH_INK);
    }

    /* The cursor row: a 2 px ink bar in the margin, not a highlight over the
     * numbers, so no number is ever drawn on the wrong colour. */
    {
        int const y = y0 + (row - row_top) * TH_LINE;

        scr_fill(0, y - 1, 3, TH_LINE, TH_INK);
    }

    /* Start, step and count, in the strip. */
    nt_fit(start, 3, 8, num, (int)sizeof num);
    strcpy(head, "start ");
    nt_append(head, (int)sizeof head, num);
    nt_append(head, (int)sizeof head, "  step ");
    nt_fit(steps[step_idx], 2, 5, num, (int)sizeof num);
    nt_append(head, (int)sizeof head, num);
    nt_append(head, (int)sizeof head, "  n ");
    nt_i64(count, num, (int)sizeof num);
    nt_append(head, (int)sizeof head, num);
    w_strip(head, "EXE probe");
}

static void draw_probe(void)
{
    double fx = 0.0;
    int const ok = eval_row(row, &fx);
    char num[32];
    char line[64];
    int const y = TH_CONTENT_Y + TH_PAD + TH_LINE;

    nt_fit(start + (double)row * steps[step_idx], 4, 16, num, (int)sizeof num);
    strcpy(line, "x = ");
    nt_append(line, (int)sizeof line, num);
    scr_text(TH_PAD, y, line, TH_INK);

    if (ok)
    {
        nt_fit(fx, 6, 16, num, (int)sizeof num);
        scr_fill(TH_PAD, y + TH_LINE + 8, SCR_W - 2 * TH_PAD, TH_LINE + 14,
                 TH_PAPER);
        scr_frame(TH_PAD, y + TH_LINE + 8, SCR_W - 2 * TH_PAD, TH_LINE + 14,
                  TH_INK, 2);
        scr_text(TH_PAD + 6, y + TH_LINE + 15, num, TH_INK);
    }
    else
        scr_text_ellipsis(TH_PAD, y + TH_LINE + 8, err, TH_INK,
                          SCR_W - 2 * TH_PAD);

    w_strip(NULL, "EXE or EXIT back");
}

/* ---- app interface ------------------------------------------------------- */

void app_function_init(void)
{
    load();
    screen = SC_LIST;
    sel = 0;
    row = 0;
    row_top = 0;
    start = 0.0;
    step_idx = 2;   /* step 1 */
    count = ROWS;
    angle = EXPR_RAD;
    err[0] = 0;
}

int app_function_timeout(void)
{
    return 0;
}

void app_function_draw(void)
{
    scr_clear(TH_PAPER);

    if (screen == SC_LIST)
        w_title("Function table", angle_name());
    else
        w_title(slots[sel].text[0] ? slots[sel].text : "Function table",
                angle_name());

    switch (screen)
    {
    case SC_LIST: draw_list(); break;
    case SC_TABLE: draw_table(); break;
    default: draw_probe(); break;
    }
    scr_present();
}

app_result_t app_function_key(bench_event_t ev)
{
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("function"));
        return APP_CONTINUE;
    }
    if (ev.key == BK_MENU)
        return APP_QUIT;

    switch (screen)
    {
    case SC_LIST:
        switch (ev.key)
        {
        case BK_UP:
            if (sel > 0)
                sel--;
            break;
        case BK_DOWN:
            if (sel + 1 < EXPR_SLOTS)
                sel++;
            break;
        case BK_EXE:
        {
            char buf[EXPR_TEXT_MAX];

            strcpy(buf, slots[sel].text);
            if (w_keyboard(buf, (int)sizeof buf, "f, g, h, p, q, r"))
            {
                exprs_set(slots, sel, buf);
                err[0] = 0;
            }
            break;
        }
        case BK_F1:
            screen = SC_TABLE;
            row = 0;
            row_top = 0;
            break;
        case BK_F3:
            slots[sel].text[0] = 0;
            break;
        case BK_F4:
            angle = angle == EXPR_DEG ? EXPR_RAD : EXPR_DEG;
            break;
        case BK_EXIT:
            save_if_changed();
            return APP_BACK;
        default:
            break;
        }
        break;

    case SC_TABLE:
        switch (ev.key)
        {
        case BK_UP:
            if (row > 0)
                row--;
            break;
        case BK_DOWN:
            if (row + 1 < count)
                row++;
            break;
        case BK_LEFT:
            start -= steps[step_idx];
            break;
        case BK_RIGHT:
            start += steps[step_idx];
            break;
        case BK_F1:
            step_idx = (step_idx + 1) % STEPS;
            break;
        case BK_F2:
            count = count >= 200 ? 12 : count + 12;
            break;
        case BK_F4:
            angle = angle == EXPR_DEG ? EXPR_RAD : EXPR_DEG;
            break;
        case BK_EXE:
            screen = SC_PROBE;
            break;
        case BK_EXIT:
            screen = SC_LIST;
            break;
        default:
            break;
        }
        if (row >= count)
            row = count - 1;
        if (row < 0)
            row = 0;
        row_top = row >= ROWS ? row - ROWS + 1 : 0;
        break;

    default:
        switch (ev.key)
        {
        case BK_EXE:
        case BK_EXIT:
        case BK_F1:
            screen = SC_TABLE;
            break;
        default:
            break;
        }
        break;
    }
    return APP_CONTINUE;
}
