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
#include "core/nav.h"
#include "platform/input.h"
#include "platform/screen.h"
#include "platform/starter.h"
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
    VIEW_CPLX,
    VIEW_FUNCTION,
    VIEW_POLY,
    VIEW_ROOTS,
    VIEW_TRIANGLE,
    VIEW_SEQ,
    VIEW_STATS,
    VIEW_SUVAT,
    VIEW_MOLES,
    VIEW_BASE,
    VIEW_GRAPH,
    VIEW_LOG,
    VIEW_SIGFIG,
    VIEW_HWREPORT,
    VIEW_FACTOR,
    VIEW_PI,
    VIEW_FORMULA,
    VIEW_PERCENT,
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
    [VIEW_CPLX] = { app_cplx_init, app_cplx_draw, app_cplx_key,
                    app_cplx_timeout },
    [VIEW_FUNCTION] = { app_function_init, app_function_draw, app_function_key,
                        app_function_timeout },
    [VIEW_POLY] = { app_poly_init, app_poly_draw, app_poly_key,
                    app_poly_timeout },
    [VIEW_ROOTS] = { app_roots_init, app_roots_draw, app_roots_key,
                     app_roots_timeout },
    [VIEW_TRIANGLE] = { app_triangle_init, app_triangle_draw,
                        app_triangle_key, app_triangle_timeout },
    [VIEW_SEQ] = { app_seq_init, app_seq_draw, app_seq_key, app_seq_timeout },
    [VIEW_STATS] = { app_stats_init, app_stats_draw, app_stats_key,
                     app_stats_timeout },
    [VIEW_SUVAT] = { app_suvat_init, app_suvat_draw, app_suvat_key,
                     app_suvat_timeout },
    [VIEW_MOLES] = { app_moles_init, app_moles_draw, app_moles_key,
                     app_moles_timeout },
    [VIEW_BASE] = { app_base_init, app_base_draw, app_base_key,
                    app_base_timeout },
    [VIEW_GRAPH] = { app_graph_init, app_graph_draw, app_graph_key,
                     app_graph_timeout },
    [VIEW_LOG] = { app_log_init, app_log_draw, app_log_key,
                   app_log_timeout },
    [VIEW_SIGFIG] = { app_sigfig_init, app_sigfig_draw, app_sigfig_key,
                      app_sigfig_timeout },
    [VIEW_HWREPORT] = { app_hwreport_init, app_hwreport_draw, app_hwreport_key,
                        app_hwreport_timeout },
    [VIEW_FACTOR] = { app_factor_init, app_factor_draw, app_factor_key,
                      app_factor_timeout },
    [VIEW_PI] = { app_pi_init, app_pi_draw, app_pi_key, app_pi_timeout },
    [VIEW_FORMULA] = { app_formula_init, app_formula_draw, app_formula_key,
                       app_formula_timeout },
    [VIEW_PERCENT] = { app_percent_init, app_percent_draw, app_percent_key,
                       app_percent_timeout },
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

/* Open another screen.  The screen being left is remembered in core/nav.c, so
 * EXIT and the help screen's way out both walk back the way the user came
 * instead of dropping them on the home screen every time. */
void app_open(app_screen_t screen)
{
    nav_push((int)view);
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
    case APP_SCREEN_CPLX: set_view(VIEW_CPLX); break;
    case APP_SCREEN_FUNCTION: set_view(VIEW_FUNCTION); break;
    case APP_SCREEN_POLY: set_view(VIEW_POLY); break;
    case APP_SCREEN_ROOTS: set_view(VIEW_ROOTS); break;
    case APP_SCREEN_TRIANGLE: set_view(VIEW_TRIANGLE); break;
    case APP_SCREEN_SEQ: set_view(VIEW_SEQ); break;
    case APP_SCREEN_STATS: set_view(VIEW_STATS); break;
    case APP_SCREEN_SUVAT: set_view(VIEW_SUVAT); break;
    case APP_SCREEN_MOLES: set_view(VIEW_MOLES); break;
    case APP_SCREEN_BASE: set_view(VIEW_BASE); break;
    case APP_SCREEN_GRAPH: set_view(VIEW_GRAPH); break;
    case APP_SCREEN_LOG: set_view(VIEW_LOG); break;
    case APP_SCREEN_SIGFIG: set_view(VIEW_SIGFIG); break;
    case APP_SCREEN_HWREPORT: set_view(VIEW_HWREPORT); break;
    case APP_SCREEN_FACTOR: set_view(VIEW_FACTOR); break;
    case APP_SCREEN_PI: set_view(VIEW_PI); break;
    case APP_SCREEN_FORMULA: set_view(VIEW_FORMULA); break;
    case APP_SCREEN_PERCENT: set_view(VIEW_PERCENT); break;
    default: set_view(VIEW_MENU); break;
    }
}

int main(void)
{
    bool running = true;

    scr_init();
    store_init();
    /* The starter pack travels inside the .g3a and lands here on the first run, so a
     * fresh card has notes, decks, a study log and three booklets to look at
     * before anything is copied over USB.  Only missing files are written. */
    starter_install();
    nav_reset();
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
        if (r == APP_BACK || r == APP_UP)
        {
            /* Back the way we came; the home screen when there is nothing
             * behind this one.  nav_pop() never returns this screen, so EXIT
             * cannot stand still. */
            int const previous = nav_pop();

            set_view(previous < 0 || previous >= (int)VIEW_COUNT
                         ? VIEW_MENU
                         : (view_t)previous);
        }
    }

    /* Leave the display in a known state for the OS. */
    dclear(TH_PAPER);
    dupdate();
    return 1;
}
