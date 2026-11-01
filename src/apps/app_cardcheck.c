/* app_cardcheck.c - what the calculator says about the folders Bench reads.
 *
 * Spec: the "it cannot see my files" reports.  A screen that says "no notes
 * yet" and a card that has the notes in it are two different facts, and from
 * the outside they look identical.  This screen prints the facts:
 *
 *   where    the four places Bench looks, with the path it actually opened
 *   counts   how many files each place holds, per folder
 *   listing  the raw names one folder really returns, straight from the OS
 *
 * Reached from Settings with F2 (the hardware report keeps F3).  Nothing is
 * written: it reads, it draws, it exits.
 *
 * Keys: UP/DOWN pick a folder, F1 re-scan, EXIT back.  F2 of the settings
 * screen gets you here; there is no tile, it is a diagnostic. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "platform/screen.h"
#include "platform/store.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

/* The four folders a person can put files in, in menu order. */
static char const *const FOLDERS[] = { "notes", "cards", "study", "formulas" };
#define FOLDER_COUNT 4

/* One folder's report. */
typedef struct {
    char folder[16];
    char ext[8];
    int counts[STORE_AT_COUNT];
    store_entry_t raw[8];
    int raw_count;
    store_location_t first_at;
} check_t;

static check_t checks[FOLDER_COUNT];
static int sel;

static void probe(void)
{
    static char const *const exts[FOLDER_COUNT] = { ".md", ".txt", ".txt", ".txt" };

    for (int i = 0; i < FOLDER_COUNT; i++)
    {
        check_t *c = &checks[i];

        strncpy(c->folder, FOLDERS[i], sizeof c->folder - 1);
        c->folder[sizeof c->folder - 1] = 0;
        strncpy(c->ext, exts[i], sizeof c->ext - 1);
        c->ext[sizeof c->ext - 1] = 0;
        store_probe(FOLDERS[i], exts[i], c->counts);

        /* The first place with anything, and its first eight names: the same
         * call the list screens make, so this shows what they see. */
        c->raw_count = store_list(FOLDERS[i], exts[i], c->raw, 8);
        if (c->raw_count < 0)
            c->raw_count = 0;
        c->first_at = store_list_location();
    }
}

void app_cardcheck_init(void)
{
    sel = 0;
    probe();
}

int app_cardcheck_timeout(void)
{
    return 0;
}

static void draw_summary(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int const w = SCR_W - 2 * TH_PAD;
    int const bottom = SCR_H - TH_SOFT_H - TH_LINE - 2;
    check_t const *c = &checks[sel];
    char num[8];

    scr_text(TH_PAD, y, "Folder", TH_INK);
    scr_text_ellipsis(TH_PAD + scr_text_w("Folder "), y, c->folder, TH_INK,
                      w - scr_text_w("Folder ") - scr_text_w(c->ext) - TH_PAD);
    scr_text_right(SCR_W - TH_PAD, y, c->ext, TH_INK);
    y += TH_LINE + 2;

    /* Each place, with the number of files it holds.  A zero everywhere is the
     * answer to "why is the list empty". */
    for (int at = 0; at < STORE_AT_COUNT && y + TH_LINE <= bottom; at++)
    {
        char line[64];
        char const *path = at == STORE_AT_BENCH  ? "Bench\\"
                           : at == STORE_AT_SAVEF ? "SAVE-F\\Bench\\"
                                                  : "root\\";
        int n = c->counts[at];
        int path_w;

        strcpy(line, path);
        /* The loose row has no folder of its own: those files sit in the drive
         * root next to the .g3a. */
        if (at != STORE_AT_LOOSE)
            nt_append(line, (int)sizeof line, c->folder);

        nt_u64((uint64_t)n, num, (int)sizeof num);
        nt_append(num, (int)sizeof num, n == 1 ? " file" : " files");
        path_w = w - scr_text_w(num) - TH_PAD;

        /* An empty place is drawn as an ink bar with paper text: the palette
         * has no grey, and a shade label on paper fails the contrast check. */
        if (n > 0)
        {
            scr_text_ellipsis(TH_PAD, y, line, TH_INK, path_w);
            scr_text_right(SCR_W - TH_PAD, y, num, TH_RED);
        }
        else
        {
            scr_fill(TH_PAD - 2, y - 1, w + 4, TH_LINE, TH_INK);
            scr_text_ellipsis(TH_PAD, y, line, TH_PAPER, path_w - 4);
            scr_text_right(SCR_W - TH_PAD, y, num, TH_PAPER);
        }
        y += TH_LINE;
    }

    y += 2;
    if (c->raw_count == 0)
    {
        if (y + TH_LINE <= bottom)
            scr_text_ellipsis(TH_PAD, y, "Nothing found in any place", TH_INK, w);
        if (y + 2 * TH_LINE <= bottom)
            scr_text_ellipsis(TH_PAD, y + TH_LINE, "Is the .g3a in the drive root?",
                              TH_INK, w);
        return;
    }

    {
        char head[64];
        int const rows = (bottom - y) / TH_LINE;

        strcpy(head, "List reads ");
        nt_u64((uint64_t)c->raw_count, num, (int)sizeof num);
        nt_append(head, (int)sizeof head, " from ");
        nt_append(head, (int)sizeof head, store_location_name(c->first_at));
        scr_text_ellipsis(TH_PAD, y, head, TH_INK, w);
        for (int i = 0; i < c->raw_count && i + 1 < rows; i++)
            scr_text_ellipsis(TH_PAD + 8, y + (i + 1) * TH_LINE, c->raw[i].name,
                              TH_INK, w - 8);
    }
}

void app_cardcheck_draw(void)
{
    scr_clear(TH_PAPER);
    w_title("Card check", "F1 rescan");
    draw_summary();
    w_strip("UP/DOWN folder", "EXIT back");
    scr_present();
}

app_result_t app_cardcheck_key(bench_event_t ev)
{
    if (ev.key == BK_EXIT)
        return APP_BACK;
    if (ev.key == BK_MENU)
        return APP_QUIT;
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("settings"));
        return APP_CONTINUE;
    }
    if (ev.key == BK_F1)
    {
        probe();
        return APP_CONTINUE;
    }
    if (ev.key == BK_UP && sel > 0)
        sel--;
    else if (ev.key == BK_DOWN && sel < FOLDER_COUNT - 1)
        sel++;
    return APP_CONTINUE;
}
