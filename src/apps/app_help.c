/* app_help.c - the shared help screen, rendered from a registry row.
 *
 * Spec: docs/plan.md (help) and F in the V2 plan (per-tool help, under 20
 * lines of 40 columns).  The text lives in src/apps/registry.c next to the
 * tool; this file only lays it out, so there is one help page per tool and no
 * second copy to drift.  Paging rules are in core/help.c, where they are
 * tested without a display.
 *
 * Keys: UP/DOWN scroll, EXE or EXIT back, F1 also goes back, F5 stays here.
 * Files: none. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/help.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/widgets.h"

static tool_t const *subject;
static int first;   /* index of the first line on screen */

void app_help_open(tool_t const *tool)
{
    subject = tool;
    first = 0;
    app_open(APP_SCREEN_HELP);
}

/* The tool this page is about; the menu always passes one. */
static tool_t const *resolved(void)
{
    return subject ? subject : tool_by_id("pacer");
}

/* The help text without the title line the title bar already shows. */
static char const *body(void)
{
    tool_t const *tool = resolved();
    char const *text = (tool && tool->help) ? tool->help
                                            : "No help for this screen yet.";

    if (tool && help_skip_title(text, tool->title))
        text += strlen(tool->title) + 1;
    return text;
}

static int visible_lines(void)
{
    int const top = TH_CONTENT_Y + TH_PAD;
    int const bottom = SCR_H - TH_SOFT_H - TH_PAD;

    return (bottom - top) / TH_LINE;
}

void app_help_init(void)
{
}

int app_help_timeout(void)
{
    return 0;
}

void app_help_draw(void)
{
    char const *text = body();
    int const lines = help_line_count(text);
    int const fit = visible_lines();
    int const last = first + fit;
    int y = TH_CONTENT_Y + TH_PAD;
    char buf[48];

    scr_clear(TH_PAPER);
    w_title(resolved()->title, "Help");

    for (int i = first; i < last && i < lines; i++)
    {
        /* An empty line is spacing, and a label of nothing is not readable
         * text: skip it rather than asking the checker to read it. */
        if (help_line(text, i, buf, (int)sizeof buf) > 0)
            scr_text(TH_PAD, y, buf, TH_INK);
        y += TH_LINE;
    }

    w_strip(help_needs_scroll(text, fit) ? "UP/DOWN scroll" : NULL,
            "EXE back");
    scr_present();
}

app_result_t app_help_key(bench_event_t ev)
{
    char const *text = body();
    int const fit = visible_lines();

    switch (ev.key)
    {
    case BK_MENU:
        return APP_QUIT;
    case BK_UP:
        first = help_scroll_pos(text, first, fit, -1);
        break;
    case BK_DOWN:
        first = help_scroll_pos(text, first, fit, 1);
        break;
    case BK_EXIT:
    case BK_EXE:
    case BK_F1:
        return APP_UP;
    default:
        break;
    }
    return APP_CONTINUE;
}
