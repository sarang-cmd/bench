/* app_help.h - the shared help screen.
 *
 * Every tool carries its help text in its registry row (`tool->help`, at most
 * 20 lines of at most 40 columns).  One screen renders that string, so a new
 * tool gets a help page by writing its row and nothing else.
 *
 * Call app_help_open(tool) from anywhere; it remembers the tool and asks
 * main.c for APP_SCREEN_HELP.  APP_UP on the way out returns to the screen
 * that asked, not to the home screen.
 */

#ifndef BENCH_APP_HELP_H
#define BENCH_APP_HELP_H

#include "registry.h"

/* Show the help page for `tool` (NULL shows the home screen's own help). */
void app_help_open(tool_t const *tool);

#endif /* BENCH_APP_HELP_H */
