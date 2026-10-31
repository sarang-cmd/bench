/* app_numtheory.c - the Number theory tool.
 *
 * Spec: docs/plan.md section 6.5, TASK-M8.
 *
 * Screens
 *   list    the nine functions
 *   form    one to four number fields, validated as 18-digit integers
 *   busy    progress bar while a factorisation runs; EXIT cancels
 *   result  the answer, plus steps for gcd and the extended gcd
 *
 * Keys
 *   list    UP/DOWN pick, EXE open, EXIT back
 *   form    UP/DOWN field, digits, DEL removes, EXE run, EXIT back
 *   busy    EXIT cancels (the running job polls the key itself)
 *   result  EXE or EXIT back to the list
 *
 * Files: none. */

#include "app.h"

#include <string.h>

#include "core/numtheory.h"
#include "platform/screen.h"
#include "platform/input.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum {
    FN_GCD, FN_LCM, FN_EGCD, FN_INV, FN_POW, FN_PRIME, FN_FACTOR, FN_TOTIENT,
    FN_CRT, FN_COUNT
} fn_t;

typedef enum { SC_LIST, SC_FORM, SC_BUSY, SC_RESULT } screen_t;

static char const *const fn_name[FN_COUNT] = {
    "gcd", "lcm", "ext gcd", "inverse", "modpow", "is prime", "factor",
    "totient", "CRT",
};
static char const *const fn_hint[FN_COUNT] = {
    "two numbers", "least common multiple", "Bezout x and y",
    "a^-1 mod m", "base^exp mod m", "Miller-Rabin", "EXIT cancels",
    "Euler phi", "x = r1 mod m1, r2 mod m2",
};
static uint8_t const fn_args[FN_COUNT] = { 2, 2, 2, 2, 3, 1, 1, 1, 4 };

static screen_t screen;
static int list_sel;
static int list_top;
static int field;
static char in[4][20];
static char err[48];
static char result[160];
static char steps[6][48];
static int step_count;
static int busy_permille;
static uint64_t busy_n;

void app_numtheory_init(void)
{
    screen = SC_LIST;
    list_sel = 0;
    list_top = 0;
    field = 0;
    err[0] = 0;
    result[0] = 0;
    step_count = 0;
    busy_permille = 0;
    for (int i = 0; i < 4; i++)
        in[i][0] = 0;
}

int app_numtheory_timeout(void)
{
    return 0;
}

/* ---- argument labels ---------------------------------------------------- */

static char const *arg_label(fn_t fn, int idx)
{
    switch (fn)
    {
    case FN_POW:
        return idx == 0 ? "base" : idx == 1 ? "exp" : "mod";
    case FN_INV:
        return idx == 0 ? "a" : "mod";
    case FN_CRT:
        return idx == 0 ? "r1" : idx == 1 ? "m1" : idx == 2 ? "r2" : "m2";
    default:
        return idx == 0 ? "a" : "b";
    }
}

/* ---- running ------------------------------------------------------------ */

static void busy_draw(uint32_t steps_done, uint32_t limit);

static nt_progress_t *live_prog;

static bool cancel_poll(void *ctx)
{
    (void)ctx;
    /* Redraw from here: the core calls this hook between trial divisions, so
     * the bar moves without a second thread or a timer. */
    if (live_prog)
        busy_draw(live_prog->steps, live_prog->trial_limit);
    return input_exit_down();
}

/* Progress callback draw: the bar and a hint line. */
static void busy_draw(uint32_t steps_done, uint32_t limit)
{
    char line[64];
    char num[24];

    scr_clear(TH_PAPER);
    w_title("factor", "EXIT cancels");
    nt_u64(busy_n, num, (int)sizeof num);
    strcpy(line, "Factoring ");
    nt_append(line, (int)sizeof line, num);
    scr_text(TH_PAD, TH_CONTENT_Y + TH_PAD, line, TH_INK);

    busy_permille = limit ? (int)((uint64_t)steps_done * 1000u / limit) : 0;
    if (busy_permille > 1000)
        busy_permille = 1000;
    w_progress(TH_PAD, TH_CONTENT_Y + 3 * TH_LINE, SCR_W - 2 * TH_PAD, 10,
               "trial division", busy_permille);

    nt_u64((uint64_t)steps_done, num, (int)sizeof num);
    scr_text(TH_PAD, TH_CONTENT_Y + 5 * TH_LINE, num, TH_INK);
    scr_text(TH_PAD, TH_CONTENT_Y + 7 * TH_LINE, "EXIT stops the search.",
             TH_RED);
    scr_present();
}

static void clear_result(void)
{
    result[0] = 0;
    step_count = 0;
}

static void add_step(char const *text)
{
    if (step_count < 6)
    {
        strncpy(steps[step_count], text, (int)sizeof steps[0] - 1);
        steps[step_count][sizeof steps[0] - 1] = 0;
        step_count++;
    }
}

static void run_gcd(uint64_t a, uint64_t b)
{
    char num[24];
    uint64_t g = nt_gcd(a, b);

    nt_u64(g, result, (int)sizeof result);
    /* Euclid steps, at most six. */
    while (b != 0 && step_count < 6)
    {
        char line[48];
        uint64_t r = a % b;

        nt_u64(a, num, (int)sizeof num);
        strcpy(line, num);
        nt_append(line, (int)sizeof line, " mod ");
        nt_u64(b, num, (int)sizeof num);
        nt_append(line, (int)sizeof line, num);
        nt_append(line, (int)sizeof line, " = ");
        nt_u64(r, num, (int)sizeof num);
        nt_append(line, (int)sizeof line, num);
        add_step(line);
        a = b;
        b = r;
    }
}

static void run_ext_gcd(uint64_t a, uint64_t b)
{
    int64_t x = 0;
    int64_t y = 0;
    uint64_t g = nt_egcd(a, b, &x, &y);
    char num[24];
    char line[48];

    nt_u64(g, result, (int)sizeof result);
    strcpy(line, "x = ");
    nt_i64(x, num, (int)sizeof num);
    nt_append(line, (int)sizeof line, num);
    add_step(line);
    strcpy(line, "y = ");
    nt_i64(y, num, (int)sizeof num);
    nt_append(line, (int)sizeof line, num);
    add_step(line);
    strcpy(line, "a*x + b*y = g");
    add_step(line);
}

static void run_one(fn_t fn)
{
    uint64_t v[4] = { 0, 0, 0, 0 };
    char num[24];
    nt_status_t st = NT_OK;

    for (int i = 0; i < fn_args[fn]; i++)
    {
        st = nt_parse_u64(in[i], (int)strlen(in[i]), &v[i]);
        if (st != NT_OK)
        {
            strcpy(err, nt_error_text(st));
            return;
        }
    }
    err[0] = 0;
    clear_result();

    switch (fn)
    {
    case FN_GCD:
        run_gcd(v[0], v[1]);
        return;
    case FN_EGCD:
        run_ext_gcd(v[0], v[1]);
        return;
    case FN_LCM:
    {
        uint64_t out = 0;

        st = nt_lcm(v[0], v[1], &out);
        if (st != NT_OK)
            strcpy(err, nt_error_text(st));
        else
            nt_u64(out, result, (int)sizeof result);
        return;
    }
    case FN_INV:
    {
        uint64_t out = 0;

        st = nt_modinv(v[0], v[1], &out);
        if (st != NT_OK)
            strcpy(err, nt_error_text(st));
        else
            nt_u64(out, result, (int)sizeof result);
        return;
    }
    case FN_POW:
        if (v[2] == 0)
        {
            strcpy(err, "The modulus cannot be 0");
            return;
        }
        nt_u64(nt_modpow(v[0], v[1], v[2]), result, (int)sizeof result);
        return;
    case FN_PRIME:
        strcpy(result, nt_is_prime(v[0]) ? "prime" : "not prime");
        return;
    case FN_FACTOR:
    {
        nt_factorization_t f;
        nt_progress_t prog = { 0, 0 };

        busy_n = v[0];
        memset(&f, 0, sizeof f);
        live_prog = &prog;
        screen = SC_BUSY;
        busy_permille = 0;
        busy_draw(0, 1);
        st = nt_factor(v[0], &f, cancel_poll, NULL, &prog);
        live_prog = NULL;
        screen = SC_RESULT;
        if (st == NT_ERR_CANCELLED)
        {
            strcpy(err, "Cancelled");
            return;
        }
        if (st != NT_OK)
        {
            strcpy(err, nt_error_text(st));
            return;
        }
        if (f.count == 0)
            strcpy(result, "1");
        else
            nt_factors(f.factor, f.exponent, f.count, result,
                       (int)sizeof result);
        for (int i = 0; i < f.count && step_count < 5; i++)
        {
            char line[48];
            char ex[8];

            nt_u64(f.factor[i], num, (int)sizeof num);
            strcpy(line, num);
            if (f.exponent[i] > 1)
            {
                nt_append(line, (int)sizeof line, "^");
                nt_u64((uint64_t)f.exponent[i], ex, (int)sizeof ex);
                nt_append(line, (int)sizeof line, ex);
            }
            add_step(line);
        }
        return;
    }
    case FN_TOTIENT:
    {
        uint64_t out = 0;

        busy_n = v[0];
        screen = SC_BUSY;
        busy_draw(0, 1);
        st = nt_totient(v[0], &out, cancel_poll, NULL);
        screen = SC_RESULT;
        if (st == NT_ERR_CANCELLED)
            strcpy(err, "Cancelled");
        else if (st != NT_OK)
            strcpy(err, nt_error_text(st));
        else
            nt_u64(out, result, (int)sizeof result);
        return;
    }
    default:
    {
        uint64_t r = 0;
        uint64_t m = 0;

        st = nt_crt2(v[0], v[1], v[2], v[3], &r, &m);
        if (st != NT_OK)
        {
            strcpy(err, nt_error_text(st));
            return;
        }
        nt_u64(r, result, (int)sizeof result);
        strcpy(steps[0], "mod ");
        nt_u64(m, num, (int)sizeof num);
        nt_append(steps[0], (int)sizeof steps[0], num);
        step_count = 1;
        return;
    }
    }
}

/* ---- drawing ------------------------------------------------------------ */

static void draw_list(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int rows = w_rows_in(TH_CONTENT_H - 4 * TH_LINE);

    list_top = w_scroll_follow(list_sel, list_top, rows);
    for (int i = 0; i < rows && list_top + i < FN_COUNT; i++)
    {
        int idx = list_top + i;

        w_row(TH_PAD, y + i * TH_LINE, SCR_W - 2 * TH_PAD, fn_name[idx], "",
              idx == list_sel);
    }
    w_scrollbar(SCR_W - TH_PAD - 2, y, rows * TH_LINE, FN_COUNT, rows,
                list_top);
    scr_text_ellipsis(TH_PAD, SCR_H - TH_SOFT_H - TH_LINE - 2,
                      fn_hint[list_sel], TH_INK, SCR_W - 2 * TH_PAD);

    w_softkeys((char const *const[6]){ "Open", "", "", "", "", "" }, -1);
}

static void draw_form(fn_t fn)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int w = SCR_W - 2 * TH_PAD;
    int fh = TH_LINE + 7;

    scr_text(TH_PAD, y, fn_name[fn], TH_INK);
    y += TH_LINE + 2;
    for (int i = 0; i < fn_args[fn]; i++)
    {
        w_field(TH_PAD, y, w, fh, arg_label(fn, i), in[i], field == i);
        y += fh + 2;
    }
    y += 2;
    if (err[0])
        scr_text_ellipsis(TH_PAD, y, err, TH_INK, w);
    else
        scr_text(TH_PAD, y, "Up to 18 digits, no sign.", TH_INK);

    w_softkeys((char const *const[6]){ "Run", "", "", "", "", "" }, -1);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "EXE run   UP/DOWN field   EXIT back", TH_INK);
}

static void draw_result(fn_t fn)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int w = SCR_W - 2 * TH_PAD;
    char line[64];

    strcpy(line, fn_name[fn]);
    nt_append(line, (int)sizeof line, " result");
    scr_text(TH_PAD, y, line, TH_INK);
    y += TH_LINE + 2;

    if (err[0])
    {
        scr_fill(TH_PAD, y, w, TH_LINE + 6, TH_RED);
        scr_text(TH_PAD + 4, y + 3, err, TH_PAPER);
        y += TH_LINE + 12;
    }
    else
    {
        scr_fill(TH_PAD, y, w, TH_LINE + 10, TH_PAPER);
        scr_frame(TH_PAD, y, w, TH_LINE + 10, TH_INK, 2);
        scr_text_ellipsis(TH_PAD + 4, y + 5, result, TH_INK, w - 8);
        y += TH_LINE + 16;
    }

    for (int i = 0; i < step_count && i < 6; i++)
    {
        scr_text_ellipsis(TH_PAD + 4, y + i * TH_LINE, steps[i], TH_INK, w - 4);
    }

    w_softkeys((char const *const[6]){ "", "", "", "", "", "" }, -1);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "EXE or EXIT back", TH_INK);
}

void app_numtheory_draw(void)
{
    scr_clear(TH_PAPER);
    switch (screen)
    {
    case SC_BUSY:
        return;                 /* drawn by the progress callback */
    case SC_LIST:
        w_title("Number theory", "");
        draw_list();
        break;
    case SC_FORM:
        w_title("Number theory", "F1 run");
        draw_form((fn_t)list_sel);
        break;
    default:
        w_title("Number theory", "");
        draw_result((fn_t)list_sel);
        break;
    }
    scr_present();
}

app_result_t app_numtheory_key(bench_event_t ev)
{
    if (ev.key == BK_MENU)
        return APP_QUIT;

    switch (screen)
    {
    case SC_LIST:
        if (ev.key == BK_EXIT)
            return APP_BACK;
        if (ev.key == BK_UP && list_sel > 0)
            list_sel--;
        else if (ev.key == BK_DOWN && list_sel + 1 < FN_COUNT)
            list_sel++;
        else if (ev.key == BK_EXE)
        {
            screen = SC_FORM;
            field = 0;
            err[0] = 0;
            for (int i = 0; i < 4; i++)
                in[i][0] = 0;
        }
        break;

    case SC_FORM:
        if (ev.key == BK_EXIT)
            screen = SC_LIST;
        else if (ev.key == BK_UP && field > 0)
            field--;
        else if (ev.key == BK_DOWN && field + 1 < fn_args[list_sel])
            field++;
        else if (ev.key == BK_DIGIT)
        {
            int n = (int)strlen(in[field]);

            if (n < (int)sizeof in[0] - 1)
            {
                in[field][n] = ev.ch;
                in[field][n + 1] = 0;
            }
        }
        else if (ev.key == BK_DEL)
        {
            int n = (int)strlen(in[field]);

            if (n > 0)
                in[field][n - 1] = 0;
        }
        else if (ev.key == BK_EXE || ev.key == BK_F1)
        {
            run_one((fn_t)list_sel);
            if (screen != SC_RESULT)
                screen = SC_RESULT;
        }
        break;

    case SC_BUSY:
        /* The running job owns the screen; nothing to do here. */
        break;

    default:
        if (ev.key == BK_EXE || ev.key == BK_EXIT)
        {
            clear_result();
            err[0] = 0;
            screen = SC_LIST;
        }
        break;
    }
    return APP_CONTINUE;
}
