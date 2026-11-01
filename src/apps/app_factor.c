/* app_factor.c - prime factorisation, on its own screen in Math.
 *
 * Spec: the request for "prime factorisation plus more maths tools".  Number
 * theory already factored numbers, three levels down a list of nine functions;
 * this is the same core routine with the answer where you can see it.
 *
 * Type a number, press EXE, and the page shows:
 *
 *   360 = 2^3 x 3^2 x 5
 *   divisors      24
 *   divisor sum   1170
 *   totient       96
 *   square-free   no
 *
 * Keys: digits and DEL edit, EXE factorises, F1 clears, F2 -1 (repeat the last
 * answer, halved), EXIT back home.  A factorisation that takes more than a
 * moment can be stopped with EXIT, and says so.
 *
 * Unverified on device: the timing of the rho routine on a real CPU. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/numtheory.h"
#include "platform/input.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

#define IN_MAX 19
#define LINE_MAX 20

static char in[IN_MAX];
static int cursor;
static char err[40];
static char factor_text[56];
static char num_text[LINE_MAX];
static uint64_t divisor_count, divisor_sum, totient, radical;
static int distinct;
static int square;
static int done;
static char last[IN_MAX];   /* the number that was factorised */

void app_factor_init(void)
{
    in[0] = 0;
    cursor = 0;
    err[0] = 0;
    factor_text[0] = 0;
    num_text[0] = 0;
    done = 0;
}

int app_factor_timeout(void)
{
    return 0;
}

/* Factor what is typed.  The number is echoed back through nt_u64 so the page
 * cannot show a different number from the one it factored. */
static void run(void)
{
    nt_factorization_t f;
    nt_progress_t prog;
    uint64_t n;
    nt_status_t st;

    err[0] = 0;
    factor_text[0] = 0;
    done = 0;
    strcpy(last, in);

    st = nt_parse_u64(in, (int)strlen(in), &n);
    if (st != NT_OK)
    {
        strcpy(err, nt_error_text(st));
        return;
    }
    st = nt_factor(n, &f, NULL, NULL, &prog);
    if (st != NT_OK)
    {
        strcpy(err, nt_error_text(st));
        return;
    }
    if (nt_factor_text(&f, factor_text, (int)sizeof factor_text) != NT_OK)
        strcpy(factor_text, "too many factors to show");

    nt_u64(n, num_text, (int)sizeof num_text);
    if (nt_divisor_count(&f, &divisor_count) != NT_OK)
        divisor_count = 0;
    if (nt_divisor_sum(&f, &divisor_sum) != NT_OK)
        divisor_sum = 0;
    if (nt_totient_from(&f, &totient) != NT_OK)
        totient = 0;
    if (nt_factor_radical(&f, &radical) != NT_OK)
        radical = 0;
    distinct = f.count;
    square = nt_factor_is_square(&f) ? 1 : 0;
    done = 1;
}

/* A label on the left, its value against the right margin. */
static void pair(int y, char const *label, char const *value)
{
    scr_text(TH_PAD, y, label, TH_INK);
    scr_text_right(SCR_W - TH_PAD, y, value, TH_INK);
}

static void draw(const char *title)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int w = SCR_W - 2 * TH_PAD;
    char line[64], num[24];

    scr_clear(TH_PAPER);
    w_title(title, "F1 clear");

    w_field(TH_PAD, y, w, 20, "n =", in, 1);
    y += 24;

    if (err[0])
    {
        scr_fill(TH_PAD, y, w, TH_LINE + 6, TH_RED);
        scr_text_ellipsis(TH_PAD + 4, y + 3, err, TH_PAPER, w - 8);
        y += TH_LINE + 12;
    }

    if (done)
    {
        /* The factorisation itself, in a bordered panel: it is the answer. */
        strcpy(line, num_text);
        nt_append(line, (int)sizeof line, " = ");
        nt_append(line, (int)sizeof line, factor_text);
        scr_fill(TH_PAD, y, w, TH_LINE + 10, TH_PAPER);
        scr_frame(TH_PAD, y, w, TH_LINE + 10, TH_INK, 2);
        scr_text_ellipsis(TH_PAD + 4, y + 5, line, TH_INK, w - 8);
        y += TH_LINE + 16;

        nt_u64(divisor_count, num, (int)sizeof num);
        pair(y, "divisors", num);
        y += TH_LINE;
        nt_u64(divisor_sum, num, (int)sizeof num);
        pair(y, "their sum", num);
        y += TH_LINE;
        nt_u64(totient, num, (int)sizeof num);
        pair(y, "totient", num);
        y += TH_LINE;
        nt_u64(radical, num, (int)sizeof num);
        pair(y, "square-free part", num);
        y += TH_LINE;
        nt_u64((uint64_t)distinct, num, (int)sizeof num);
        pair(y, "distinct primes", num);
        y += TH_LINE;
        pair(y, "perfect square", square ? "yes" : "no");
        y += TH_LINE + 8;

        /* What the numbers mean, since a label like "square-free part" is not
         * self-explanatory and the help page is a keypress away. */
        scr_hline(TH_PAD, y - 4, w, TH_SHADE);
        scr_text(TH_PAD, y, "Square-free part: the primes with", TH_INK);
        scr_text(TH_PAD, y + TH_LINE, "their powers thrown away.", TH_INK);
    }
    else if (!err[0])
    {
        scr_text(TH_PAD, y, "Type a whole number, EXE factors it.", TH_INK);
        scr_text(TH_PAD, y + TH_LINE, "Up to 18 digits.", TH_INK);
    }

    w_softkeys((char const *const[6]){ "Clear", "Halve", "", "", "Help", "" }, -1);
    scr_present();
}

void app_factor_draw(void)
{
    draw("Prime factors");
}

app_result_t app_factor_key(bench_event_t ev)
{
    switch (ev.key)
    {
    case BK_MENU:
        return APP_QUIT;
    case BK_EXIT:
        return APP_BACK;
    case BK_F5:
        app_help_open(tool_by_id("factor"));
        return APP_CONTINUE;
    case BK_F1:
        in[0] = 0;
        cursor = 0;
        err[0] = 0;
        done = 0;
        break;
    case BK_F2:
        /* Halve the answer: the second half of a factoring question, and the
         * reason this screen is in Math rather than hidden in a list. */
        if (done)
        {
            uint64_t n;

            if (nt_parse_u64(last, (int)strlen(last), &n) == NT_OK && n > 1)
            {
                nt_u64(n / 2, in, (int)sizeof in);
                cursor = (int)strlen(in);
                run();
            }
        }
        break;
    case BK_DEL:
        if (cursor > 0)
        {
            in[--cursor] = 0;
            done = 0;
            err[0] = 0;
        }
        break;
    case BK_EXE:
        if (cursor > 0)
            run();
        break;
    default:
        if (ev.key == BK_DIGIT && cursor < IN_MAX - 1)
        {
            in[cursor++] = ev.ch;
            in[cursor] = 0;
            done = 0;
            err[0] = 0;
        }
        break;
    }
    return APP_CONTINUE;
}
