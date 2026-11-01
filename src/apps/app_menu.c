/* app_menu.c - the home screen: categories, pages of tiles, one dashboard.
 *
 * Spec: docs/plan.md section 5 and docs/design.md section 3.
 *
 * Layout: a title bar, a content area, a softkey strip.  The content area is
 * divided into a category dashboard (Study only) and a 3 x 2 grid of tiles;
 * a category with more than six tools gets pages, marked by a bordered
 * "1 / 2" pill with an arrow at each end.
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
#include "core/layout.h"
#include "core/pageind.h"
#include "platform/screen.h"
#include "platform/store.h"
#include "registry.h"
#include "theme.h"
#include "ui/shapes.h"
#include "ui/widgets.h"

/* ---- state -------------------------------------------------------------- */

static tool_category_t category;
static int sel;          /* index inside the category */
/* Non-zero once the menu has been shown.  Returning from a tool must not
 * reset the category and the tile the user was on: that is how EXIT used to
 * drop them back on Study, first tile, every time. */
static int remembered;
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

/* The page pill.  It replaces the row of 4 px dots, which was reported as too
 * subtle: a bordered box with a number in it cannot be missed, and the arrows
 * say which way there is more.  Ink on paper, shade for the arrow that has
 * nowhere to go, so the pill never looks disabled. */
static void draw_pager(int pages, int page, int y)
{
    char label[16];
    int const h = 15;
    int const pad = 4;
    int const arrow_w = scr_text_w("<");
    int w;
    int x;

    if (pages < 2 || pageind_label(page, pages, label, (int)sizeof label) < 0)
        return;

    w = 2 * (pad + arrow_w + pad) + scr_text_w(label) + 2;
    x = (SCR_W - w) / 2;

    scr_fill(x + TH_SHADOW, y + TH_SHADOW, w, h, TH_SHADE);
    scr_fill(x, y, w, h, TH_PAPER);
    scr_frame(x, y, w, h, TH_INK, TH_BORDER);

    /* Only the arrows that lead somewhere are drawn.  A greyed-out one would be
     * shade on paper - 1.3:1, which the screen checker rejects, and rightly:
     * at that contrast it is a smudge, not a hint. */
    if (pageind_left(page))
        scr_text(x + pad, y + 3, "<", TH_RED);
    if (pageind_right(page, pages))
        scr_text(x + w - pad - arrow_w, y + 3, ">", TH_RED);
    scr_text_center(x, w, y + 3, label, TH_INK);
}

/* ---- drawing ------------------------------------------------------------ */

void app_menu_init(void)
{
    if (!remembered)
    {
        category = TOOL_CAT_STUDY;
        sel = 0;
        remembered = 1;
    }
    else if (sel >= tool_category_size(category))
    {
        /* The tool list shrank under a stale selection: land on the last tile. */
        sel = tool_category_size(category) - 1;
        if (sel < 0)
            sel = 0;
    }
    /* The counts can have changed while a tool was open, so they are always
     * re-read; only the cursor is remembered. */
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
    int const left = tool_category_size(category) - page * TILES_PER_PAGE;
    int top = TH_CONTENT_Y + TH_PAD;
    /* Only a category with pages gives up space for the pill; the rest keep the
     * full grid. */
    int const pages = tool_category_pages(category);
    int dots_y = SCR_H - TH_SOFT_H - (pages > 1 ? 22 : 9);
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
    /* A page with one row of tools sits in the middle of the grid rather than
     * leaving a hole at the bottom of an otherwise empty page.
     *
     * The count is what is on *this* page, not in the category.  Counting the
     * whole category is a bug the Math category had: seven tools meant three
     * rows of two-page height, so the centring offset went negative and the
     * first row was drawn over the title bar, which is why the Math screen
     * arrived with no title at all. */
    {
        int const shown = left > TILES_PER_PAGE ? TILES_PER_PAGE : left;
        int const rows = shown > 0 ? (shown + TILE_COLS - 1) / TILE_COLS : 0;

        if (rows > 0)
            grid_y = top + (grid_h - (rows * tile_h + (rows - 1) * TH_GAP)) / 2;
    }

    for (int i = 0; i < TILES_PER_PAGE; i++)
    {
        int const index = page * TILES_PER_PAGE + i;
        tool_t const *tool = tool_category_at(category, index);
        int const row = i / TILE_COLS;
        int const col = i % TILE_COLS;
        /* A short row is centred under the one above it.  Tools holds two
         * tiles, and drawing them hard against the left margin left a third of
         * the screen empty beside them. */
        int const in_row = layout_row_count(i - col, left, TILE_COLS);
        int const x0 = layout_centre_row(in_row, tile_w, TH_GAP,
                                         SCR_W - 2 * TH_PAD) + TH_PAD;

        if (!tool)
            continue;
        draw_tile(tool, x0 + col * (tile_w + TH_GAP),
                  grid_y + row * (tile_h + TH_GAP), tile_w, tile_h,
                  index == sel);
    }

    /* A category with no tools still says something instead of a blank page. */
    if (size == 0)
    {
        w_message("Nothing here yet",
                  "This category fills up as tools ship.");
    }

    if (pages > 1)
        draw_pager(pages, page, SCR_H - TH_SOFT_H - 21);

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
