/* app.h - the contract between main.c and every screen.
 *
 * Each app is init/draw/key: init resets its own static state, draw paints one
 * full frame, key handles one event and returns where to go.  No app calls
 * another app's functions; main.c routes. */

#ifndef BENCH_APP_H
#define BENCH_APP_H

#include <stdbool.h>

#include "core/settings.h"
#include "platform/input.h"

typedef enum {
    APP_CONTINUE = 0,   /* stay here */
    APP_BACK = 1,       /* return to the home screen */
    APP_QUIT = 2,       /* leave the add-in */
    APP_UP = 3,         /* return to the screen that was open before this one */
} app_result_t;

/* Screens an app may ask for.  main.c maps these to its own view ids. */
typedef enum {
    APP_SCREEN_MENU = 0,
    APP_SCREEN_PACER,
    APP_SCREEN_UNCERT,
    APP_SCREEN_CARDS,
    APP_SCREEN_NOTES,
    APP_SCREEN_NUMTHEORY,
    APP_SCREEN_SETTINGS,
    APP_SCREEN_ABOUT,
    APP_SCREEN_HELP,
    APP_SCREEN_CPLX,
    APP_SCREEN_FUNCTION,
    APP_SCREEN_POLY,
    APP_SCREEN_ROOTS,
    APP_SCREEN_TRIANGLE,
    APP_SCREEN_SEQ,
    APP_SCREEN_STATS,
    APP_SCREEN_SUVAT,
    APP_SCREEN_MOLES,
    APP_SCREEN_BASE,
    APP_SCREEN_GRAPH,
    APP_SCREEN_LOG,
    APP_SCREEN_SIGFIG,
    APP_SCREEN_HWREPORT,
    APP_SCREEN_FACTOR,
    APP_SCREEN_PI,
    APP_SCREEN_FORMULA,
    APP_SCREEN_PERCENT,
} app_screen_t;

/* Switch to another screen (implemented in main.c). */
void app_open(app_screen_t screen);

/* Settings are owned by main.c: the managers load them once, apps read them
 * through app_cfg() and write with app_cfg_save(), which only touches the file
 * when a value changed. */
bench_settings_t *app_cfg(void);
void app_cfg_save(void);
/* Read state/settings.cfg into cfg (defaults survive a missing or broken file).
 * main.c calls this once, before the first frame. */
void app_settings_load(bench_settings_t *cfg);

/* How long main.c may wait for a key before redrawing (ms).  Zero blocks until
 * a key arrives; the Pacer run screen returns 250 so its clock ticks. */
#define DECLARE_APP(name)                                                    \
    void app_##name##_init(void);                                            \
    void app_##name##_draw(void);                                            \
    app_result_t app_##name##_key(bench_event_t ev);                         \
    int app_##name##_timeout(void)

DECLARE_APP(menu);
DECLARE_APP(pacer);
DECLARE_APP(uncert);
DECLARE_APP(cards);
DECLARE_APP(notes);
DECLARE_APP(numtheory);
DECLARE_APP(settings);
DECLARE_APP(about);
DECLARE_APP(help);
DECLARE_APP(cplx);
DECLARE_APP(function);
DECLARE_APP(poly);
DECLARE_APP(roots);
DECLARE_APP(triangle);
DECLARE_APP(seq);
DECLARE_APP(stats);
DECLARE_APP(suvat);
DECLARE_APP(moles);
DECLARE_APP(base);
DECLARE_APP(graph);
DECLARE_APP(log);
DECLARE_APP(sigfig);
DECLARE_APP(hwreport);
DECLARE_APP(factor);
DECLARE_APP(pi);
DECLARE_APP(formula);
DECLARE_APP(percent);

#endif /* BENCH_APP_H */
