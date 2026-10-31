/* main.c - entry point and router for the Bench add-in.
 *
 * Spec: docs/plan.md section 5 (screen and input model).
 *
 * One screen at a time.  main.c keeps the current view, asks it to draw, waits
 * for a key (or, on the Pacer run screen, a short timeout) and passes the key
 * back.  Apps never call each other: they return APP_BACK, or ask for a screen
 * with app_open().
 *
 * Files: store_init() creates Bench/notes, Bench/cards and Bench/state once,
 * then app_settings_load() reads state/settings.cfg.  Nothing else is touched
 * here. */

#include <gint/display.h>

#include "apps/app.h"
#include "platform/input.h"
#include "platform/screen.h"
#include "platform/store.h"
#include "theme.h"
#include "version.h"

typedef enum {
    VIEW_MENU = 0,
    VIEW_PACER,
    VIEW_UNCERT,
    VIEW_CARDS,
    VIEW_NOTES,
    VIEW_NUMTHEORY,
    VIEW_SETTINGS,
    VIEW_ABOUT,
    VIEW_HELP,
    VIEW_COUNT,
} view_t;

typedef struct {
    void (*init)(void);
    void (*draw)(void);
    app_result_t (*key)(bench_event_t ev);
    int (*timeout)(void);
} app_entry_t;

static bench_settings_t cfg;
static view_t view = VIEW_MENU;
/* The screen to come back to when a screen opened a modal-ish one (help). */
static view_t view_before = VIEW_MENU;

/* Declared before the table so the initialisers below can name the apps. */
static app_entry_t const apps[VIEW_COUNT] = {
    [VIEW_MENU] = { app_menu_init, app_menu_draw, app_menu_key,
                    app_menu_timeout },
    [VIEW_PACER] = { app_pacer_init, app_pacer_draw, app_pacer_key,
                     app_pacer_timeout },
    [VIEW_UNCERT] = { app_uncert_init, app_uncert_draw, app_uncert_key,
                      app_uncert_timeout },
    [VIEW_CARDS] = { app_cards_init, app_cards_draw, app_cards_key,
                     app_cards_timeout },
    [VIEW_NOTES] = { app_notes_init, app_notes_draw, app_notes_key,
                     app_notes_timeout },
    [VIEW_NUMTHEORY] = { app_numtheory_init, app_numtheory_draw,
                         app_numtheory_key, app_numtheory_timeout },
    [VIEW_SETTINGS] = { app_settings_init, app_settings_draw, app_settings_key,
                        app_settings_timeout },
    [VIEW_ABOUT] = { app_about_init, app_about_draw, app_about_key,
                     app_about_timeout },
    [VIEW_HELP] = { app_help_init, app_help_draw, app_help_key,
                    app_help_timeout },
};

bench_settings_t *app_cfg(void)
{
    return &cfg;
}

static void set_view(view_t v)
{
    if (v < 0 || v >= VIEW_COUNT)
        v = VIEW_MENU;
    view = v;
    apps[view].init();
}

void app_open(app_screen_t screen)
{
    view_before = view;
    switch (screen)
    {
    case APP_SCREEN_PACER: set_view(VIEW_PACER); break;
    case APP_SCREEN_UNCERT: set_view(VIEW_UNCERT); break;
    case APP_SCREEN_CARDS: set_view(VIEW_CARDS); break;
    case APP_SCREEN_NOTES: set_view(VIEW_NOTES); break;
    case APP_SCREEN_NUMTHEORY: set_view(VIEW_NUMTHEORY); break;
    case APP_SCREEN_SETTINGS: set_view(VIEW_SETTINGS); break;
    case APP_SCREEN_ABOUT: set_view(VIEW_ABOUT); break;
    case APP_SCREEN_HELP: set_view(VIEW_HELP); break;
    default: set_view(VIEW_MENU); break;
    }
}

int main(void)
{
    bool running = true;

    scr_init();
    store_init();
    app_settings_load(&cfg);
    set_view(VIEW_MENU);

    while (running)
    {
        app_entry_t const *cur = &apps[view];
        int tmo;
        bench_event_t ev;
        app_result_t r;

        cur->draw();
        tmo = cur->timeout();
        ev = tmo > 0 ? input_next_timeout(tmo) : input_next();

        if (ev.key == BK_MENU)
            break;
        if (ev.key == BK_NONE)
            continue;

        r = cur->key(ev);
        if (r == APP_QUIT)
            break;
        if (r == APP_BACK)
            set_view(VIEW_MENU);
        if (r == APP_UP)
            set_view(view_before);
    }

    /* Leave the display in a known state for the OS. */
    dclear(TH_PAPER);
    dupdate();
    return 1;
}
