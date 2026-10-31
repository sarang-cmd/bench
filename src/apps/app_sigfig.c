/* app_sigfig.c - Significant figures and standard form (V2 feature F9).
 *
 * Spec: the V2 master prompt, F9.  One screen, because the answer is the
 * screen: type a number, pick what to do with it, and the result updates as
 * you move.
 *
 * Modes: count significant figures, round to n significant figures, round to
 * n decimals, standard form, SI prefix, and a practice mode that asks a
 * question and marks the typed answer.
 *
 * Keys: UP/DOWN pick the mode, LEFT/RIGHT change n, EXE types a number (or the
 * practice answer), F1 clears, EXIT back.
 *
 * Files: none. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/numfmt.h"
#include "core/numparse.h"
#include "core/sigfig.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum {
    M_COUNT = 0,
    M_SIG,
    M_PLACE,
    M_STANDARD,
    M_SI,
    M_PRACTICE,
    M_COUNT_
} mode_t;

static char const *const mode_name[M_COUNT_] = {
    "Count the figures", "Round to n sf", "Round to n decimals",
    "Standard form", "SI prefix", "Practice",
};

/* The practice questions: values whose figure count is not obvious. */
static char const *const practice[] = {
    "0.004560", "1200", "1200.", "10.0", "0.50", "2.500", "100", "3.010",
};
#define PRACTICE_COUNT ((int)(sizeof practice / sizeof practice[0]))

static mode_t mode;
static char value[24];
static int figures;          /* n for the rounding modes, 1..10 */
static double parsed;
static bool parsed_ok;
static int asked;            /* index into practice[] */
static char answer[24];
static int score;
static int tries;
static bool checked;
static bool right;

static void reparse(void)
{
    parsed_ok = numparse_cstr(value, &parsed) == NUMPARSE_OK;
}

/* Type into a number field the way every other screen does: the keypad's own
 * digits, DEL removes.  (The on-screen keyboard is for text, not numbers.) */
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

static void type_back(void)
{
    int const n = (int)strlen(value);

    if (n > 0)
        value[n - 1] = 0;
    reparse();
}

void app_sigfig_init(void)
{
    mode = M_SIG;
    strcpy(value, "0.00456");
    figures = 3;
    asked = 0;
    answer[0] = 0;
    score = 0;
    tries = 0;
    checked = false;
    right = false;
    reparse();
}

int app_sigfig_timeout(void)
{
    return 0;
}

/* The line the tool is answering, and the answer. */
static bool result_line(char *out, int size)
{
    char num[32];

    switch (mode)
    {
    case M_COUNT:
    {
        int const n = sf_count_text(value);
        char tail[24];

        if (n < 0)
            return false;
        nt_i64(n, tail, (int)sizeof tail);
        strcpy(out, tail);
        strcat(out, n == 1 ? " significant figure" : " significant figures");
        return true;
    }
    case M_SIG:
        if (!parsed_ok || numfmt_format_sig(parsed, figures, num, (int)sizeof num) < 0)
            return false;
        strcpy(out, num);
        strcat(out, "   =   ");
        if (sf_standard(parsed, figures, num, (int)sizeof num) < 0)
            return false;
        strcat(out, num);
        return true;
    case M_PLACE:
        if (!parsed_ok ||
            numfmt_format_fixed(parsed, figures, num, (int)sizeof num) < 0)
            return false;
        strcpy(out, num);
        return true;
    case M_STANDARD:
        return parsed_ok &&
               sf_standard(parsed, figures, out, size) >= 0;
    default:
        return parsed_ok && sf_si(parsed, figures, out, size) >= 0;
    }
}

static void draw_practice(int y)
{
    char line[64];

    strcpy(line, "How many figures in ");
    nt_append(line, (int)sizeof line, practice[asked]);
    nt_append(line, (int)sizeof line, "?");
    scr_text_ellipsis(TH_PAD, y, line, TH_INK, SCR_W - 2 * TH_PAD);

    y += TH_LINE + 4;
    w_field(TH_PAD, y, SCR_W - 2 * TH_PAD, TH_LINE + 10, "Your answer",
            answer[0] ? answer : "type it", !checked);

    y += TH_LINE + 18;
    if (checked)
    {
        strcpy(line, right ? "Right. " : "Not quite: ");
        nt_append(line, (int)sizeof line, right ? "" : "it is ");
        if (!right)
        {
            nt_i64(sf_count_text(practice[asked]), line + strlen(line),
                   (int)sizeof line - (int)strlen(line));
            nt_append(line, (int)sizeof line, ".");
        }
        scr_text_ellipsis(TH_PAD, y, line, TH_INK, SCR_W - 2 * TH_PAD);
    }
    strcpy(line, "Score ");
    nt_i64(score, line + strlen(line), (int)sizeof line - (int)strlen(line));
    nt_append(line, (int)sizeof line, "/");
    nt_i64(tries, line + strlen(line), (int)sizeof line - (int)strlen(line));
    scr_text_right(SCR_W - TH_PAD, y, line, TH_INK);
}

void app_sigfig_draw(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    char line[80];
    int const w = SCR_W - 2 * TH_PAD;

    scr_clear(TH_PAPER);
    w_title("Sig figs", mode_name[mode]);

    if (mode == M_PRACTICE)
    {
        draw_practice(y);
        w_softkeys((char const *const[6]){ "", "Next", "", "", "", "" }, -1);
        scr_present();
        return;
    }

    /* The number, in a field, so it reads as something you can edit. */
    w_field(TH_PAD, y, w, TH_LINE + 10, "Number", value, true);
    y += TH_LINE + 18;

    /* The line under the field is the knob, not the mode name: the title bar
     * already says which mode this is. */
    if (mode == M_COUNT)
        strcpy(line, "UP/DOWN picks one of six modes");
    else
    {
        strcpy(line, mode == M_PLACE ? "decimal places dp = " : "figures n = ");
        nt_i64(figures, line + strlen(line), (int)sizeof line - (int)strlen(line));
        if (mode != M_SIG && mode != M_STANDARD && mode != M_SI &&
            mode != M_PLACE)
            line[0] = 0;
    }
    scr_text_ellipsis(TH_PAD, y, line, TH_INK, w);
    y += TH_LINE + 6;

    if (result_line(line, (int)sizeof line))
    {
        scr_fill(TH_PAD, y, w, TH_LINE + 12, TH_PAPER);
        scr_frame(TH_PAD, y, w, TH_LINE + 12, TH_INK, 2);
        scr_text_ellipsis(TH_PAD + 5, y + 6, line, TH_INK, w - 10);
    }
    else
        scr_text_ellipsis(TH_PAD, y + 4, "Type a number first (EXE).", TH_INK,
                          w);

    /* One line of why, under the answer. */
    y += TH_LINE + 20;
    switch (mode)
    {
    case M_COUNT:
        scr_text(TH_PAD, y, "Trailing zeros count only after a point.", TH_INK);
        break;
    case M_SIG:
        scr_text(TH_PAD, y, "Half up on the digit after the nth.", TH_INK);
        break;
    case M_PLACE:
        scr_text(TH_PAD, y, "Half up on the next decimal.", TH_INK);
        break;
    case M_STANDARD:
        scr_text(TH_PAD, y, "One digit, then the power of ten.", TH_INK);
        break;
    default:
        scr_text(TH_PAD, y, "The power is always a multiple of three.", TH_INK);
        break;
    }

    w_softkeys((char const *const[6]){ "Clear", "Practice", "", "", "", "" },
               -1);
    scr_present();
}

app_result_t app_sigfig_key(bench_event_t ev)
{
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("sigfig"));
        return APP_CONTINUE;
    }
    if (ev.key == BK_MENU)
        return APP_QUIT;

    if (mode == M_PRACTICE)
    {
        switch (ev.key)
        {
        case BK_DIGIT:
        case BK_DOT:
            if (!checked)
                type_digit(answer, (int)sizeof answer, ev);
            break;
        case BK_DEL:
            if (!checked && answer[0])
                answer[strlen(answer) - 1] = 0;
            break;
        case BK_EXE:
            if (!checked && answer[0])
            {
                int const want = sf_count_text(practice[asked]);
                double typed = 0.0;
                int const got = numparse_cstr(answer, &typed) == NUMPARSE_OK
                                    ? (int)(typed + 0.5)
                                    : -1;

                right = got == want;
                checked = true;
                tries++;
                if (right)
                    score++;
            }
            else if (checked)
            {
                asked = (asked + 1) % PRACTICE_COUNT;
                answer[0] = 0;
                checked = false;
            }
            break;
        case BK_F2:
            asked = (asked + 1) % PRACTICE_COUNT;
            answer[0] = 0;
            checked = false;
            break;
        case BK_EXIT:
            mode = M_COUNT;
            break;
        default:
            break;
        }
        return APP_CONTINUE;
    }

    switch (ev.key)
    {
    case BK_UP:
        mode = (mode_t)((mode + M_COUNT_ - 1) % M_COUNT_);
        break;
    case BK_DOWN:
        mode = (mode_t)((mode + 1) % M_COUNT_);
        break;
    case BK_LEFT:
        if (figures > 1)
            figures--;
        break;
    case BK_RIGHT:
        if (figures < 10)
            figures++;
        break;
    case BK_DIGIT:
    case BK_DOT:
        type_digit(value, (int)sizeof value, ev);
        reparse();
        break;
    case BK_DEL:
        type_back();
        break;
    case BK_F1:
        value[0] = 0;
        reparse();
        break;
    case BK_F2:
        mode = M_PRACTICE;
        answer[0] = 0;
        checked = false;
        break;
    case BK_EXIT:
        return APP_BACK;
    default:
        break;
    }
    return APP_CONTINUE;
}
