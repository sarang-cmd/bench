/* app_menu.c - the home screen: categories, pages of tiles, one dashboard.
 *
 * Spec: docs/plan.md section 5 and docs/design.md.
 *
 * The screen is the registry (registry.c) drawn: a category strip on F1..F4,
 * six tiles a page, page dots when a category has more than one page, and a
 * dashboard strip on Study.  Adding a tool means adding a registry row; this
 * file does not know any tool by name.
 *
 * Colour rules, and the bug they exist for: an icon drawn in the same colour
 * as its tile disappears.  So every foreground here is chosen by measurement
 * (core/color.h) - the better of ink and paper on the fill - and a subtitle
 * that still cannot reach 4.5:1 gets a paper plate under it.  tools/sim checks
 * the pixels afterwards, so a wrong constant fails the build.
 *
 * Keys: F1..F4 category, LEFT/RIGHT/UP/DOWN move (LEFT/RIGHT at the edge
 * turns the page), EXE opens, F5 Settings, F6 About, MENU quits.
 */

#include "app.h"

#include <stdio.h>
#include <string.h>

#include "core/color.h"
#include "platform/screen.h"
#include "platform/store.h"
#include "registry.h"
#include "theme.h"
#include "ui/widgets.h"

/* ---- state -------------------------------------------------------------- */

static tool_category_t category;
static int sel;          /* index inside the category */
static int cards_files;  /* dashboard: decks and notes on the card */
static int notes_files;

static int const category_keys[4] = { BK_F1, BK_F2, BK_F3, BK_F4 };

/* ---- colour ------------------------------------------------------------- */

/* The better of the palette's two foregrounds on `fill`. */
static int better_foreground(int fill)
{
    return col_contrast(TH_INK, fill) >= col_contrast(TH_PAPER, fill) ? TH_INK
                                                                     : TH_PAPER;
}

/* The readable foreground on `fill`, and whether it needs a paper plate. */
static int text_colour(int fill, double floor, bool *plate)
{
    int const best = better_foreground(fill);

    *plate = col_contrast(best, fill) < floor;
    return *plate ? TH_INK : best;
}

/* The icon colour: the accent on a paper tile, and on a coloured tile the
 * outline's opposite, so the two can never be confused. */
static int icon_colour(tool_t const *tool, int fill, bool on)
{
    if (on)
        return better_foreground(fill);
    return col_contrast(tool->accent, TH_PAPER) >= COL_ICON_MIN_CONTRAST
               ? tool->accent
               : TH_INK;
}

/* ---- dashboard ---------------------------------------------------------- */

static void dashboard_count(void)
{
    store_entry_t list[STORE_LIST_MAX];

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
    int n = 0;

    scr_fill(x, y, w, h, TH_SHADE);
    scr_frame(x, y, w, h, TH_INK, 2);
    scr_fill(x + 4, y + 4, 4, 4, TH_RED);
    scr_text(x + 12, y + (h - scr_text_h()) / 2, "ON THE CARD", TH_INK);

    /* Cards due and streak need the Study log (F13) and Cards v2 (F15); until
     * then the strip counts what is on the card. */
    n += snprintf(line + n, sizeof line - (size_t)n, "%d deck%s", cards_files,
                  cards_files == 1 ? "" : "s");
    snprintf(line + n, sizeof line - (size_t)n, "  %d note%s", notes_files,
             notes_files == 1 ? "" : "s");
    scr_text_right(x + w - 5, y + (h - scr_text_h()) / 2, line, TH_INK);
}

/* ---- tiles -------------------------------------------------------------- */

static void draw_tile(tool_t const *tool, int x, int y, int w, int h, bool on)
{
    int const fill = on ? tool->accent : TH_PAPER;
    int const icon_fill = icon_colour(tool, fill, on);
    int const icon_edge = icon_fill == TH_INK ? TH_PAPER : TH_INK;
    int const tx = x + 7;
    int const text_w = w - 14;
    /* Icon and text are one block, centred in the tile. */
    int const block = 20 + 4 + 2 * TH_LINE;
    int const iy = y + (h > block ? (h - block) / 2 : 4);
    int const ty = iy + 20 + 4;
    bool plate = false;
    int colour;

    /* Shadow first, outside the tile: 3 px right and bottom, an L, never
     * inside the shape where a filled tile would swallow it. */
    if (on)
    {
        scr_fill(x + TH_SHADOW, y + h, w, TH_SHADOW, TH_INK);
        scr_fill(x + w, y + TH_SHADOW, TH_SHADOW, h, TH_INK);
    }

    scr_fill(x, y, w, h, fill);
    scr_frame(x, y, w, h, TH_INK, on ? TH_BORDER_SEL : TH_BORDER);

    /* Icon: a 20x20 box, filled in one colour and outlined in the other, so it
     * can never merge into the tile. */
    tool->icon(x + 7, iy, icon_fill, icon_edge);
    SCR_TRACE(SCR_TR_ICON, x + 7, iy, 20, 20, icon_fill, fill, tool->title);

    /* Title: the largest text on the screen, so the 3:1 floor. */
    colour = text_colour(fill, COL_TITLE_MIN_CONTRAST, &plate);
    if (plate)
    {
        scr_fill(tx - 2, ty - 1, text_w + 4, TH_LINE - 1, TH_PAPER);
        colour = TH_INK;
    }
    scr_text_ellipsis_as(SCR_TR_TILE_TITLE, tx, ty, tool->title, colour,
                         text_w);

    /* Subtitle: body text, so 4.5:1.  On an accent that cannot carry it the
     * line gets a paper plate hugging the words - the mark that says which
     * tile is selected, rather than a box across the whole tile. */
    colour = text_colour(fill, COL_TEXT_MIN_CONTRAST, &plate);
    if (plate)
    {
        int const pw = scr_text_w(tool->subtitle) + 4;
        int const sw = pw < text_w + 4 ? pw : text_w + 4;

        scr_fill(tx - 2, ty + TH_LINE - 1, sw, TH_LINE - 1, TH_PAPER);
        colour = TH_INK;
    }
    scr_text_ellipsis(tx, ty + TH_LINE, tool->subtitle, colour, text_w);
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

        if (i == page)
            scr_fill(x, y, 4, 4, TH_INK);
        else
            scr_frame(x, y, 4, 4, TH_INK, 1);
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
    int const pages = tool_category_pages(category);
    int grid_top = TH_CONTENT_Y + TH_PAD;
    int const dots_y = SCR_H - TH_SOFT_H - 10;
    int const grid_bottom = dots_y - 6;
    int tile_w = (SCR_W - 2 * TH_PAD - 2 * TH_GAP) / TILE_COLS;
    int tile_h;
    char const *const soft[6] = {
        "Study", "Math", "Science", "Tools", "Settings", "About",
    };

    scr_clear(TH_PAPER);
    w_title("Bench", tool_category_name(category));

    if (category == TOOL_CAT_STUDY)
    {
        draw_dashboard(TH_PAD, TH_CONTENT_Y + TH_GAP, SCR_W - 2 * TH_PAD, 20);
        grid_top = TH_CONTENT_Y + TH_GAP + 20 + 6;
    }

    tile_h = (grid_bottom - grid_top - TH_GAP) / TILE_ROWS;

    for (int i = 0; i < TILES_PER_PAGE; i++)
    {
        int const index = page * TILES_PER_PAGE + i;
        tool_t const *tool = tool_category_at(category, index);
        int const col = i % TILE_COLS;
        int const row = i / TILE_COLS;

        if (!tool)
            continue;
        draw_tile(tool, TH_PAD + col * (tile_w + TH_GAP),
                  grid_top + row * (tile_h + TH_GAP), tile_w, tile_h,
                  index == sel);
    }

    /* A category with no tools still says something instead of a blank page. */
    if (size == 0)
        w_message("Nothing here yet", "This category fills up as tools ship.");

    if (pages > 1)
        draw_dots(pages, page, dots_y);

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
    int const size = tool_category_size(category);

    for (int i = 0; i < 4; i++)
        if ((int)ev.key == category_keys[i])
        {
            set_category((tool_category_t)i);
            return APP_CONTINUE;
        }

    switch (ev.key)
    {
    case BK_LEFT:
        /* Left at the start of a page turns back a page. */
        if (sel > 0)
            sel--;
        else if (size > 0)
            sel = size - 1;
        break;
    case BK_RIGHT:
        if (sel + 1 < size)
            sel++;
        else
            sel = 0;
        break;
    case BK_UP:
        if (sel >= TILE_COLS)
            sel -= TILE_COLS;
        break;
    case BK_DOWN:
        if (sel + TILE_COLS < size)
            sel += TILE_COLS;
        break;
    case BK_MENU:
        return APP_QUIT;
    case BK_F5:
        app_open(APP_SCREEN_SETTINGS);
        break;
    case BK_F6:
        app_open(APP_SCREEN_ABOUT);
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
