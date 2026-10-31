/* app_about.c - version screen (TASK-M9: "Version screen with the Bench
 * version, build date, and OS version").
 *
 * Keys: any of EXE or EXIT goes back.
 * Files: none. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "platform/mem.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"
#include "version.h"

void app_about_init(void)
{
}

int app_about_timeout(void)
{
    return 0;
}

void app_about_draw(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int w = SCR_W - 2 * TH_PAD;
    char num[24];
    char line[64];

    scr_clear(TH_PAPER);
    w_title("About Bench", "");
    y = TH_CONTENT_Y + TH_PAD;
    scr_text(TH_PAD, y, BENCH_NAME, TH_INK);
    scr_text_right(SCR_W - TH_PAD, y, "v" BENCH_VERSION, TH_INK);
    y += TH_LINE + 2;
    scr_text(TH_PAD, y, "Pacer, Uncertainty, Cards, Notes, Number theory",
             TH_INK);
    y += TH_LINE + 6;

    scr_hline(TH_PAD, y - 3, w, TH_SHADE);

    strcpy(line, "Built ");
    nt_append(line, (int)sizeof line, __DATE__);
    scr_text(TH_PAD, y, line, TH_INK);
    y += TH_LINE;

    scr_text(TH_PAD, y, "Target Casio fx-CG50, OS 3.81", TH_INK);
    y += TH_LINE;

    scr_text(TH_PAD, y, "Framebuffer 396x224, window 384x216", TH_INK);
    y += TH_LINE;

    {
        size_t largest = mem_largest();

        nt_u64((uint64_t)(largest / 1024u), num, (int)sizeof num);
        strcpy(line, "Largest free block ");
        nt_append(line, (int)sizeof line, num);
        nt_append(line, (int)sizeof line, " KiB");
        scr_text(TH_PAD, y, line, TH_INK);
        y += TH_LINE;
    }

    scr_text(TH_PAD, y, "Notes cap 32 KiB, decks 500 cards", TH_INK);
    y += TH_LINE + 4;

    /* A red mark, ink text: red on paper is 4.46:1, short of AA for a
     * sentence (docs/design.md section 4). */
    scr_fill(TH_PAD, y + 1, 4, 4, TH_RED);
    scr_text(TH_PAD + 8, y, "Exam Mode blocks add-ins. Bench never touches it.",
             TH_INK);

    w_strip("F5 help", "EXE or EXIT back");
    scr_present();
}

app_result_t app_about_key(bench_event_t ev)
{
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("about"));
        return APP_CONTINUE;
    }
    if (ev.key == BK_MENU)
        return APP_QUIT;
    if (ev.key == BK_EXIT || ev.key == BK_EXE || ev.key == BK_F6)
        return APP_BACK;
    return APP_CONTINUE;
}
