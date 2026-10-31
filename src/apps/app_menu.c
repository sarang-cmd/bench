/* app_menu.c - the home screen: six tiles, two columns, three rows.
 *
 * Spec: docs/plan.md section 5 ("Home screen: five tiles with a square, circle,
 * triangle, half circle, and bar").
 *
 * Keys: LEFT/RIGHT/UP/DOWN move, EXE opens, F6 shows About, MENU leaves. */

#include "app.h"

#include <string.h>

#include "platform/screen.h"
#include "theme.h"
#include "ui/shapes.h"
#include "ui/widgets.h"

typedef enum {
    TILE_PACER = 0,
    TILE_UNCERT,
    TILE_CARDS,
    TILE_NOTES,
    TILE_NUMTHEORY,
    TILE_SETTINGS,
    TILE_COUNT,
} tile_t;

static char const *const titles[TILE_COUNT] = {
    "Pacer", "Uncertainty", "Cards", "Notes", "Number theory", "Settings",
};
static char const *const subs[TILE_COUNT] = {
    "minutes per mark", "value +/- error", "Leitner drill",
    "text and Markdown", "gcd to CRT", "and About",
};
static int const accents[TILE_COUNT] = {
    TH_RED, TH_BLUE, TH_YELLOW, TH_TEAL, TH_VIOLET, TH_SHADE,
};

static int sel;

void app_menu_init(void)
{
    sel = TILE_PACER;
}

int app_menu_timeout(void)
{
    return 0;
}

static void draw_shape(int tile, int x, int y)
{
    switch (tile)
    {
    case TILE_PACER: sh_square(x, y, 18, TH_RED); break;
    case TILE_UNCERT: sh_circle(x + 9, y + 9, 9, TH_BLUE); break;
    case TILE_CARDS: sh_triangle(x, y, 20, 18, TH_YELLOW); break;
    case TILE_NOTES: sh_half_circle(x + 9, y + 11, 9, TH_TEAL, 5); break;
    case TILE_NUMTHEORY: sh_bars(x, y - 2, 20, 20, TH_VIOLET); break;
    default: sh_ring(x + 9, y + 9, 9, 3, TH_INK); break;
    }
}

void app_menu_draw(void)
{
    int tile_w = (SCR_W - 3 * TH_PAD) / 2;
    int tile_h = (TH_CONTENT_H - 4 * TH_PAD) / 3;

    scr_clear(TH_PAPER);
    w_title("Bench", "F6 About");

    for (int i = 0; i < TILE_COUNT; i++)
    {
        int col = i % 2;
        int row = i / 2;
        int x = TH_PAD + col * (tile_w + TH_PAD);
        int y = TH_CONTENT_Y + TH_PAD + row * (tile_h + TH_PAD);
        bool on = i == sel;

        scr_fill(x, y, tile_w, tile_h, on ? accents[i] : TH_PAPER);
        scr_frame(x, y, tile_w, tile_h, TH_INK, on ? 3 : 1);
        draw_shape(i, x + 6, y + 6);
        scr_text(x + 30, y + 6, titles[i], TH_INK);
        scr_text_ellipsis(x + 30, y + 6 + TH_LINE, subs[i], TH_INK,
                          tile_w - 34);
        if (on)
        {
            /* Neobrutalist offset shadow, two pixels thick, on the tile edge. */
            scr_fill(x + 2, y + tile_h, tile_w - 2, 2, TH_INK);
            scr_fill(x + tile_w, y + 2, 2, tile_h, TH_INK);
        }
    }

    w_softkeys((char const *const[6]){ "", "", "", "", "Settings", "About" },
               -1);
    scr_present();
}

app_result_t app_menu_key(bench_event_t ev)
{
    switch (ev.key)
    {
    case BK_LEFT:
        if (sel % 2 == 1)
            sel--;
        break;
    case BK_RIGHT:
        if (sel % 2 == 0 && sel + 1 < TILE_COUNT)
            sel++;
        break;
    case BK_UP:
        if (sel >= 2)
            sel -= 2;
        break;
    case BK_DOWN:
        if (sel + 2 < TILE_COUNT)
            sel += 2;
        break;
    case BK_MENU:
        return APP_QUIT;
    case BK_F5:
        sel = TILE_SETTINGS;
        return APP_CONTINUE;
    case BK_F6:
        app_open(APP_SCREEN_ABOUT);
        return APP_CONTINUE;
    case BK_EXE:
        switch (sel)
        {
        case TILE_PACER: app_open(APP_SCREEN_PACER); break;
        case TILE_UNCERT: app_open(APP_SCREEN_UNCERT); break;
        case TILE_CARDS: app_open(APP_SCREEN_CARDS); break;
        case TILE_NOTES: app_open(APP_SCREEN_NOTES); break;
        case TILE_NUMTHEORY: app_open(APP_SCREEN_NUMTHEORY); break;
        default: app_open(APP_SCREEN_SETTINGS); break;
        }
        break;
    default:
        break;
    }
    return APP_CONTINUE;
}
