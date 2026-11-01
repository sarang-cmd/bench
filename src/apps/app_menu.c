/* app_menu.c - the home screen: categories, pages of tiles, one dashboard.
 *
 * Spec: docs/plan.md section 5 and docs/design.md section 3.
 *
 * Layout: a title bar, a content area, a softkey strip.  The content area is
 * divided into a category dashboard (Study only) and a 3 x 2 grid of tiles;
 * a category with more than six tools gets pages, shown as dots.
 *
 * Colour rules, and the bug they exist for: an icon drawn in the same colour
 * as its tile disappears.  So an unselected tile is paper with ink text and an
 * icon whose colour is measured against paper (core/color.h); selecting a tile
 * fills it with ink and turns the text paper, which is 12.27:1 no matter what
 * the tool's accent is.  tools/sim checks the pixels afterwards.
 *
 * Keys: F1..F4 category, LEFT/RIGHT/UP/DOWN move, EXE opens, F5 help for
 * the selected tool, F6 Settings, MENU quits. */

#include "app.h"
#include "app_help.h"

#include <stdio.h>
#include <string.h>

#include "core/color.h"
#include "platform/screen.h"
#include "platform/store.h"
#include "registry.h"
#include "theme.h"
#include "ui/shapes.h"
#include "ui/widgets.h"

/* ---- state -------------------------------------------------------------- */

static tool_category_t category;
static int sel;          /* index inside the category */
static int cards_files;  /* dashboard: decks on the card, counted once */
static int notes_files;

static int const category_keys[4] = { BK_F1, BK_F2, BK_F3, BK_F4 };

/* The icon colour on an unselected tile: the accent when it can be told apart
 * from paper, ink when it cannot (shade is 1.31:1, the trap this bug is about). */
static int icon_on_paper(tool_t const *tool)
{
    return col_contrast(tool->accent, TH_PAPER) >= COL_ICON_MIN_CONTRAST
               ? tool->accent
               : TH_INK;
}

/* ---- dashboard ---------------------------------------------------------- */

static void dashboard_count(void)
{
    store_entry_t list[STORE_LIST_MAX];

    /* Counted once, in init: the menu must not touch the card on every key. */
    cards_files = store_list("cards", ".txt", list, STORE_LIST_MAX);
    notes_files = store_list("notes", NULL, list, STORE_LIST_MAX);
    if (cards_files < 0)
        cards_files = 0;
    if (notes_files < 0)
        notes_files = 0;
}

static void draw_dashboard(int x, int y, int w, int h)
{
    char line[64];

    scr_fill(x, y, w, h, TH_SHADE);
    scr_frame(x, y, w, h, TH_INK, 1);
    scr_text(x + 5, y + (h - scr_text_h()) / 2, "ON THE CARD", TH_INK);

    /* Two counts and nothing invented: cards due and streaks arrive with the
     * Study log (F13) and Cards v2 (F15). */
    if (notes_files > 0 || cards_files > 0)
    {
        int n = 0;

        n += snprintf(line + n, sizeof line - (size_t)n, "%d deck%s",
                      cards_files, cards_files == 1 ? "" : "s");
        snprintf(line + n, sizeof line - (size_t)n, "  %d note%s", notes_files,
                 notes_files == 1 ? "" : "s");
    }
    else
    {
        snprintf(line, sizeof line, "empty: copy files into Bench/");
    }
    scr_text_right(x + w - 5, y + (h - scr_text_h()) / 2, line, TH_INK);
}

/* ---- tiles -------------------------------------------------------------- */

static void draw_tile(tool_t const *tool, int x, int y, int w, int h, bool on)
{
    /* Selection inverts the tile: ink body, paper text.  12.27:1 whatever the
     * accent is, so no plate, no special case, no way for a label to vanish. */
    int const fill = on ? TH_INK : TH_PAPER;
    int const title_c = on ? TH_PAPER : TH_INK;
    int const sub_c = on ? TH_SHADE : TH_INK;
    int const icon_fill = on ? tool->accent : icon_on_paper(tool);
    int const icon_edge = on ? TH_PAPER : TH_INK;
    int const tx = x + 28;
    int const text_w = w - 28 - 5;
    int const block_h = 2 * TH_LINE;
    int const ty = y + (h - block_h) / 2;

    /* Shadow first, so the tile sits on it: 3 px, right and bottom. */
    if (on)
    {
        scr_fill(x + TH_SHADOW, y + h, w, TH_SHADOW, TH_INK);
        scr_fill(x + w, y + TH_SHADOW, TH_SHADOW, h, TH_INK);
    }

    scr_fill(x, y, w, h, fill);
    scr_frame(x, y, w, h, TH_INK, on ? TH_BORDER_SEL : TH_BORDER);

    /* The icon keeps the tool's colour, outlined so it can never merge into
     * the tile.  Recorded for the harness in tools/sim. */
    tool->icon(x + 4, ty + (block_h - 20) / 2, icon_fill, icon_edge);
    SCR_TRACE(SCR_TR_ICON, x + 4, ty + (block_h - 20) / 2, 22, 22, icon_fill,
              fill, tool->title);

    scr_text_ellipsis_as(SCR_TR_TILE_TITLE, tx, ty, tool->title, title_c,
                         text_w);
    scr_text_ellipsis(tx, ty + TH_LINE, tool->subtitle, sub_c, text_w);
}

/* ---- pages -------------------------------------------------------------- */

static void draw_dots(int pages, int page, int y)
{
    int const gap = 8;
    int const w = pages * gap - (gap - 4);
    int const x0 = (SCR_W - w) / 2;

    for (int i = 0; i < pages; i++)
    {
        int const x = x0 + i * gap;

        scr_fill(x, y, 4, 4, TH_INK);
        if (i != page)
            scr_fill(x + 1, y + 1, 2, 2, TH_PAPER);
    }
}

/* ---- drawing ------------------------------------------------------------ */

void app_menu_init(void)
{
    category = TOOL_CAT_STUDY;
    sel = 0;
    dashboard_count();
}

int app_menu_timeout(void)
{
    return 0;
}

void app_menu_draw(void)
{
    int const size = tool_category_size(category);
    int const page = sel / TILES_PER_PAGE;
    int top = TH_CONTENT_Y + TH_PAD;
    int dots_y = SCR_H - TH_SOFT_H - 9;
    int grid_h;
    int grid_y;
    int tile_w = (SCR_W - 2 * TH_PAD - 2 * TH_GAP) / 3;
    int tile_h;
    char const *const soft[6] = {
        "Study", "Math", "Science", "Tools", "Help", "Settings",
    };

    scr_clear(TH_PAPER);
    w_title("Bench", tool_category_name(category));

    if (category == TOOL_CAT_STUDY)
    {
        draw_dashboard(TH_PAD, top, SCR_W - 2 * TH_PAD, 18);
        top += 18 + TH_GAP;
    }

    grid_h = dots_y - top - 4;
    tile_h = (grid_h - TH_GAP) / 2;
    grid_y = top;
    /* A category with one row of tools sits in the middle of the grid rather
     * than leaving a hole at the bottom of an otherwise empty page. */
    {
        int const rows = (size + TILE_COLS - 1) / TILE_COLS;

        if (rows > 0)
            grid_y = top + (grid_h - (rows * tile_h + (rows - 1) * TH_GAP)) / 2;
    }

    for (int i = 0; i < TILES_PER_PAGE; i++)
    {
        int const index = page * TILES_PER_PAGE + i;
        tool_t const *tool = tool_category_at(category, index);
        int const col = i % TILE_COLS;
        int const row = i / TILE_COLS;

        if (!tool)
            continue;
        draw_tile(tool, TH_PAD + col * (tile_w + TH_GAP),
                  grid_y + row * (tile_h + TH_GAP), tile_w, tile_h,
                  index == sel);
    }

    /* A category with no tools still says something instead of a blank page. */
    if (size == 0)
    {
        w_message("Nothing here yet",
                  "This category fills up as tools ship.");
    }

    if (tool_category_pages(category) > 1)
        draw_dots(tool_category_pages(category), page, dots_y);

    /* First run: say what the four category keys and F5 do, once.  The line
     * goes away for good the first time help is opened (app_cfg()->tour_done). */
    if (!app_cfg()->tour_done)
        scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                        "F1-F4 categories, F5 explains a tile, EXE opens it",
                        TH_INK);

    w_softkeys(soft, category);
    scr_present();
}

/* ---- keys --------------------------------------------------------------- */

static void set_category(tool_category_t c)
{
    if (c == category)
        return;
    category = c;
    sel = 0;
}

app_result_t app_menu_key(bench_event_t ev)
{
    for (int i = 0; i < 4; i++)
        if ((int)ev.key == category_keys[i])
        {
            set_category((tool_category_t)i);
            return APP_CONTINUE;
        }

    switch (ev.key)
    {
    case BK_LEFT:
        /* Left at the start of a page turns back to the previous page. */
        if (sel % TILES_PER_PAGE == 0 && sel >= TILES_PER_PAGE)
            sel -= 1;
        else if (sel > 0)
            sel--;
        break;
    case BK_RIGHT:
        if (sel + 1 < tool_category_size(category))
            sel++;
        break;
    case BK_UP:
        if (sel >= TILE_COLS)
            sel -= TILE_COLS;
        break;
    case BK_DOWN:
        if (sel + TILE_COLS < tool_category_size(category))
            sel += TILE_COLS;
        break;
    case BK_MENU:
        return APP_QUIT;
    case BK_F5:
    {
        /* Help for whatever tile is under the cursor: the same row that the
         * tool screen shows, so there is one help text per tool. */
        tool_t const *tool = tool_category_at(category, sel);

        /* Opening help once is the whole first-run tour. */
        if (!app_cfg()->tour_done)
        {
            app_cfg()->tour_done = 1;
            app_cfg_save();
        }
        app_help_open(tool);
        break;
    }
    case BK_F6:
        app_open(APP_SCREEN_SETTINGS);
        break;
    case BK_EXE:
    {
        tool_t const *tool = tool_category_at(category, sel);

        if (tool)
            app_open(tool->screen);
        break;
    }
    default:
        break;
    }
    return APP_CONTINUE;
}
