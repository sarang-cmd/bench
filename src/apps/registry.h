/* registry.h - the one list of tools.
 *
 * Spec: docs/plan.md (home screen) and docs/design.md (one tile per tool).
 *
 * Every tool is one row here and one file in src/apps.  The home screen, the
 * help pages, the About screen, the site's tool list and the README table all
 * read this table or a file generated from it (tools/gen_tools_json.py), so a
 * new tool is a new row plus a new file and nothing else.  Nothing may be
 * hand-edited into those outputs.
 *
 * A row carries three things:
 *   - how the tile looks: title, subtitle, accent, icon
 *   - where EXE goes: an app_screen_t
 *   - how it reads: help text, 40 columns, at most 20 lines
 */

#ifndef BENCH_REGISTRY_H
#define BENCH_REGISTRY_H

#include <stdbool.h>

#include "app.h"

typedef enum {
    TOOL_CAT_STUDY = 0,
    TOOL_CAT_MATH,
    TOOL_CAT_SCIENCE,
    TOOL_CAT_TOOLS,
    TOOL_CAT_COUNT
} tool_category_t;

typedef struct {
    char const *id;         /* stable, lower case; used by generated files */
    char const *title;      /* on the tile, 14 characters or fewer */
    char const *subtitle;   /* one short line under the title */
    tool_category_t category;
    int accent;             /* TH_* colour of the tile */
    /* A 20x20 icon drawn at (x, y) in `fill`, outlined in `edge`. */
    void (*icon)(int x, int y, int fill, int edge);
    app_screen_t screen;    /* where EXE goes */
    bool needs_file;        /* the empty state explains where the file goes */
    char const *help;       /* lines of at most 40 characters, '\n' separated */
} tool_t;

/* Number of tools, and the row for an index (NULL when out of range). */
int tool_count(void);
tool_t const *tool_at(int index);
tool_t const *tool_by_id(char const *id);
int tool_index(tool_t const *tool);

/* Categories, in the F1..F4 order. */
int tool_category_count(void);
char const *tool_category_name(tool_category_t category);
char const *tool_category_hint(tool_category_t category);
/* Tools in a category, in tile order. */
int tool_category_size(tool_category_t category);
tool_t const *tool_category_at(tool_category_t category, int index);

/* Tiles per page.  The home screen shows three across and two down. */
#define TILE_COLS 3
#define TILE_ROWS 2
#define TILES_PER_PAGE (TILE_COLS * TILE_ROWS)

/* Pages in a category, at least one. */
int tool_category_pages(tool_category_t category);

#endif /* BENCH_REGISTRY_H */
