/* app_percent.c - percentage change, reverse percentage, growth.
 *
 * Spec: the request for "a bunch of more maths tools".  The four questions on
 * this screen are the ones a paper actually asks, and all four are one
 * multiplication once written down - the point of the screen is to write it
 * down with the numbers in it.
 *
 * Keys: UP/DOWN pick the question, LEFT/RIGHT move between the two fields,
 * digits and DEL fill the active field, EXE works it out, F1 clears, EXIT back.
 * The rules (and the four ways a question can have no answer) are
 * core/percent.c, tested there.
 *
 * Files: none. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/numfmt.h"
#include "core/numparse.h"
#include "core/percent.h"
#include "platform/input.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum {
    Q_CHANGE = 0,     /* from a to b, by what per cent */
    Q_APPLY,          /* v with pct added */
    Q_REVERSE,        /* now is v after pct, what was it */
    Q_COMPOUND,       /* principal at pct for years */
    Q_COUNT
} question_t;

static char const *const q_title[Q_COUNT] = {
    "Percentage change", "Add or take a percentage", "Reverse percentage",
    "Compound growth",
};
static char const *const q_first[Q_COUNT] = {
    "from", "value", "now", "start",
};
static char const *const q_second[Q_COUNT] = {
    "to", "per cent", "per cent", "per cent",
};

static question_t question;
static char a[16], b[16], years[4];
static int field;              /* 0 = first, 1 = second, 2 = years */
static char err[48];
static char answer[40];
static char working[56];
static int done;

void app_percent_init(void)
{
    question = Q_CHANGE;
    a[0] = 0;
    b[0] = 0;
    strcpy(years, "5");
    field = 0;
    err[0] = 0;
    answer[0] = 0;
    working[0] = 0;
    done = 0;
}

int app_percent_timeout(void)
{
    return 0;
}

static void reset_result(void)
{
    done = 0;
    err[0] = 0;
    answer[0] = 0;
    working[0] = 0;
}

static void work(void)
{
    double x = 0, y = 0, r = 0;
    pc_status_t st = PC_ERR_ARG;
    char num[24];

    reset_result();
    if (numparse_cstr(a, &x) != NUMPARSE_OK ||
        numparse_cstr(b, &y) != NUMPARSE_OK)
    {
        strcpy(err, "two numbers, please");
        return;
    }

    switch (question)
    {
    case Q_CHANGE:
        st = pc_change(x, y, &r);
        if (st == PC_OK)
        {
            nt_fixed(r, 2, num, (int)sizeof num);
            strcpy(answer, num);
            nt_append(answer, (int)sizeof answer, "%");
        }
        break;
    case Q_APPLY:
        st = pc_apply(x, y, &r);
        if (st == PC_OK)
        {
            nt_fixed(r, 2, num, (int)sizeof num);
            strcpy(answer, num);
            /* The working, so the answer is a calculation and not a result. */
            nt_fixed(x, 2, num, (int)sizeof num);
            strcpy(working, num);
            nt_append(working, (int)sizeof working, y < 0 ? " x (1 - " : " x (1 + ");
            nt_fixed(y < 0 ? -y : y, 2, num, (int)sizeof num);
            nt_append(working, (int)sizeof working, num);
            nt_append(working, (int)sizeof working, "/100)");
        }
        break;
    case Q_REVERSE:
        st = pc_reverse(x, y, &r);
        if (st == PC_OK)
        {
            nt_fixed(r, 2, num, (int)sizeof num);
            strcpy(answer, num);
            nt_fixed(x, 2, num, (int)sizeof num);
            strcpy(working, num);
            nt_append(working, (int)sizeof working, " / (1 ");
            nt_append(working, (int)sizeof working, y < 0 ? "- " : "+ ");
            nt_fixed(y < 0 ? -y : y, 2, num, (int)sizeof num);
            nt_append(working, (int)sizeof working, num);
            nt_append(working, (int)sizeof working, "/100)");
        }
        break;
    default:
    {
        int n = 0;

        if (numparse_cstr(years, &y) != NUMPARSE_OK)
        {
            strcpy(err, "years, please");
            return;
        }
        n = (int)y;
        st = pc_compound(x, y, n, &r);
        if (st == PC_OK)
        {
            nt_fixed(r, 2, num, (int)sizeof num);
            strcpy(answer, num);
            nt_fixed(x, 2, num, (int)sizeof num);
            strcpy(working, num);
            nt_append(working, (int)sizeof working, " x growth applied ");
            nt_u64((uint64_t)n, num, (int)sizeof num);
            nt_append(working, (int)sizeof working, num);
            nt_append(working, (int)sizeof working, " times");
        }
        break;
    }
    }

    if (st != PC_OK)
        strcpy(err, pc_error_text(st));
    else
        done = 1;
}

void app_percent_draw(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int const w = SCR_W - 2 * TH_PAD;
    int const half = (w - TH_GAP) / 2;

    scr_clear(TH_PAPER);
    w_title(q_title[question], "F1 clear");

    w_field(TH_PAD, y, half, w_field_h(), q_first[question], a, field == 0);
    w_field(TH_PAD + half + TH_GAP, y, half, w_field_h(), q_second[question], b,
            field == 1);
    y += 24;

    if (question == Q_COMPOUND)
    {
        w_field(TH_PAD, y, half, w_field_h(), "years", years, field == 2);
        y += 24;
    }

    if (err[0])
    {
        scr_fill(TH_PAD, y, w, TH_LINE + 6, TH_RED);
        scr_text_ellipsis(TH_PAD + 4, y + 3, err, TH_PAPER, w - 8);
        y += TH_LINE + 10;
    }
    else if (done)
    {
        scr_fill(TH_PAD, y, w, TH_LINE + 10, TH_PAPER);
        scr_frame(TH_PAD, y, w, TH_LINE + 10, TH_INK, 2);
        scr_text_ellipsis(TH_PAD + 4, y + 5, answer, TH_INK, w - 8);
        y += TH_LINE + 16;
        if (working[0])
        {
            scr_text_ellipsis(TH_PAD, y, working, TH_INK, w);
            y += TH_LINE;
        }
        if (question == Q_CHANGE)
        {
            /* Say which way it went, since a minus sign is easy to miss. */
            double x = 0, y2 = 0, r = 0;

            if (numparse_cstr(a, &x) == NUMPARSE_OK &&
                numparse_cstr(b, &y2) == NUMPARSE_OK &&
                pc_change(x, y2, &r) == PC_OK)
                scr_text(TH_PAD, y, r < 0 ? "a fall" : "a rise", TH_INK);
        }
    }
    else
    {
        scr_text(TH_PAD, y, "Type the numbers, EXE works it out.", TH_INK);
        y += TH_LINE;
        if (question == Q_REVERSE)
            scr_text(TH_PAD, y, "Undo a discount: 34 after 15% off was 40.",
                     TH_INK);
    }

    w_softkeys((char const *const[6]){ "Clear", "", "", "", "Help", "" }, -1);
    scr_present();
}

/* The field the digits go into. */
static char *active_field(void)
{
    if (field == 1)
        return b;
    if (field == 2)
        return years;
    return a;
}

static int field_cap(void)
{
    return field == 2 ? (int)sizeof years : (int)sizeof a;
}

app_result_t app_percent_key(bench_event_t ev)
{
    int const fields = question == Q_COMPOUND ? 3 : 2;

    switch (ev.key)
    {
    case BK_MENU:
        return APP_QUIT;
    case BK_EXIT:
        return APP_BACK;
    case BK_F5:
        app_help_open(tool_by_id("percent"));
        return APP_CONTINUE;
    case BK_UP:
        question = (question_t)((question + Q_COUNT - 1) % Q_COUNT);
        field = 0;
        reset_result();
        break;
    case BK_DOWN:
        question = (question_t)((question + 1) % Q_COUNT);
        field = 0;
        reset_result();
        break;
    case BK_LEFT:
        field = (field + fields - 1) % fields;
        break;
    case BK_RIGHT:
        field = (field + 1) % fields;
        break;
    case BK_F1:
        app_percent_init();
        break;
    case BK_EXE:
        work();
        break;
    case BK_DEL:
    {
        char *f = active_field();
        size_t const n = strlen(f);

        if (n > 0)
            f[n - 1] = 0;
        reset_result();
        break;
    }
    default:
        if (ev.key == BK_DIGIT || ev.key == BK_DOT || ev.key == BK_MINUS)
        {
            char *f = active_field();
            size_t const n = strlen(f);

            if ((int)n < field_cap() - 1)
            {
                f[n] = ev.ch;
                f[n + 1] = 0;
                reset_result();
            }
        }
        break;
    }
    return APP_CONTINUE;
}
