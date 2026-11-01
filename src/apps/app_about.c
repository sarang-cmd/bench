/* app_about.c - what Bench is, who wrote it and how it was made.
 *
 * Spec: TASK-M9 ("Version screen with the Bench version, build date, and OS
 * version"), and the report that it said nothing about the person or the work.
 *
 * The page is longer than the screen, so it scrolls: the lines are plain text
 * and the scrolling rules are core/help.c's, tested there.  The last lines are
 * the ones nobody should have to hunt for - who made it, and the promise that
 * Bench never touches Exam Mode.
 *
 * Keys: UP/DOWN scroll, F5 help, EXE or EXIT back.
 * Files: none. */

#include "app.h"
#include "app_help.h"

#include <stdio.h>
#include <string.h>

#include "core/help.h"
#include "platform/mem.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"
#include "version.h"

#define ABOUT_TEXT_MAX 700

static char about_text[ABOUT_TEXT_MAX];
static int top;

/* Built once per entry: the free-block figure is read at that moment, not on
 * every key. */
static void build_text(void)
{
    static char buf[ABOUT_TEXT_MAX];
    char num[24];
    int n = 0;

    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "A study add-in for the Casio\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "fx-CG50: Pacer, Uncertainty,\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "Cards, Notes, number theory and\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n, "the rest of it.\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n, "\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n, "Made by Sarang\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n, "github.com/sarang-cmd\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n, "\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n, "HOW IT WAS MADE\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "The maths lives in plain C with no\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "calculator headers, so it can be\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "tested on a PC. The screens and\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "keys are a thin layer on top of\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "gint. Every picture in the docs is\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "a real frame from the simulator in\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "tools/sim, not a mock-up. Builds\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "come from GitHub Actions.\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n, "\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "MIT licence, no account, nothing\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n, "to sign up to.\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n, "\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n, "THE BUILD\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n, "v%s, built %s\n", BENCH_VERSION,
                  __DATE__);
    n += snprintf(buf + n, sizeof buf - (size_t)n, "Casio fx-CG50, OS 3.81\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "Screen 384x216, 396x224 buffer\n");
    nt_u64((uint64_t)(mem_largest() / 1024u), num, (int)sizeof num);
    n += snprintf(buf + n, sizeof buf - (size_t)n, "Largest free block %s KiB\n",
                  num);
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "Notes cap 32 KiB, decks 500 cards\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n, "\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "Exam Mode blocks add-ins. Bench\n");
    n += snprintf(buf + n, sizeof buf - (size_t)n,
                  "never reads it and never changes it.");
    if (n >= (int)sizeof buf)
        n = (int)sizeof buf - 1;
    buf[n] = 0;
    memcpy(about_text, buf, (size_t)n + 1);
}

static int visible_lines(void)
{
    /* One line of headroom above the softkey strip. */
    return (SCR_H - TH_SOFT_H - TH_CONTENT_Y - TH_PAD - 4) / TH_LINE;
}

void app_about_init(void)
{
    top = 0;
    build_text();
}

int app_about_timeout(void)
{
    return 0;
}

static int columns(void)
{
    int cols = scr_text_cols(SCR_W - 2 * TH_PAD - 4);

    return cols < 8 ? 8 : cols;
}

void app_about_draw(void)
{
    int const cols = columns();
    int const fit = visible_lines();
    int const lines = help_wrap_count(about_text, cols);
    int const y0 = TH_CONTENT_Y + TH_PAD;
    int y = y0;
    char buf[64];

    scr_clear(TH_PAPER);
    w_title("About Bench", "v" BENCH_VERSION);

    for (int i = top; i < top + fit && i < lines; i++)
    {
        if (help_wrap_line(about_text, cols, i, buf, (int)sizeof buf) > 0)
            scr_text(TH_PAD, y, buf, TH_INK);
        y += TH_LINE;
    }

    w_scrollbar(SCR_W - TH_PAD + 2, y0, fit * TH_LINE - 4, lines, fit, top);

    w_strip(help_wrap_needs_scroll(about_text, cols, fit) ? "UP/DOWN scroll"
                                                          : "F5 help",
            "EXE back");
    scr_present();
}

app_result_t app_about_key(bench_event_t ev)
{
    int const fit = visible_lines();

    switch (ev.key)
    {
    case BK_F5:
        app_help_open(tool_by_id("about"));
        return APP_CONTINUE;
    case BK_MENU:
        return APP_QUIT;
    case BK_UP:
        top = help_wrap_scroll(about_text, columns(), top, fit, -1);
        break;
    case BK_DOWN:
        top = help_wrap_scroll(about_text, columns(), top, fit, 1);
        break;
    case BK_EXIT:
    case BK_EXE:
    case BK_F6:
        return APP_BACK;
    default:
        break;
    }
    return APP_CONTINUE;
}
