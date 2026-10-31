/* registry.c - see registry.h.  The only hand-written list of tools. */

#include "registry.h"

#include <stddef.h>

#include "theme.h"
#include "ui/shapes.h"

/* ---- icons --------------------------------------------------------------
 *
 * Each one draws inside a nominal 20x20 box at (x, y) and leaves a pixel of
 * room for the outline, so a selected tile can use a 2 px edge. */

static void icon_square(int x, int y, int fill, int edge)
{
    sh_square_icon(x + 2, y + 2, 16, fill, edge, 2);
}

static void icon_circle(int x, int y, int fill, int edge)
{
    sh_circle_icon(x + 10, y + 10, 7, fill, edge, 2);
}

static void icon_triangle(int x, int y, int fill, int edge)
{
    sh_triangle_icon(x + 3, y + 2, 15, 15, fill, edge, 2);
}

static void icon_half_circle(int x, int y, int fill, int edge)
{
    sh_half_circle_icon(x + 10, y + 12, 8, 4, fill, edge, 2);
}

static void icon_bars(int x, int y, int fill, int edge)
{
    sh_bars_icon(x + 2, y + 2, 17, 16, fill, edge, 2);
}

static void icon_ring(int x, int y, int fill, int edge)
{
    sh_ring_icon(x + 10, y + 10, 8, 3, fill, edge, 2);
}

static void icon_frame(int x, int y, int fill, int edge)
{
    sh_square_frame_icon(x + 2, y + 2, 16, fill, edge, 2);
}

/* Three bars, each shorter than the last: a number cut to fewer figures. */
static void icon_steps(int x, int y, int fill, int edge)
{
    for (int i = 0; i < 3; i++)
        sh_square_icon(x + 2, y + 2 + i * 6, 17 - i * 5, fill, edge, 1);
}

/* A curve that turns: a parabola, the shape of the quadratic tool. */
static void icon_curve(int x, int y, int fill, int edge)
{
    for (int i = 0; i < 17; i++)
    {
        int const dx = i - 8;
        int const h = (dx * dx) / 12;

        sh_square_icon(x + 2 + i, y + 16 - h, 2, fill, edge, 1);
    }
}

/* Two rows of small cells: a table of values. */
static void icon_grid(int x, int y, int fill, int edge)
{
    for (int r = 0; r < 2; r++)
        for (int c = 0; c < 2; c++)
            sh_square_icon(x + 3 + c * 8, y + 3 + r * 8, 5, fill, edge, 1);
}

/* ---- help text ----------------------------------------------------------
 *
 * 40 columns.  These strings are the single source for the on-calculator help
 * page, docs/tools.json and the site. */

static char const help_pacer[] =
    "Pacer\n"
    "Times a practice paper against a plan.\n"
    "\n"
    "1. Enter minutes, marks and the\n"
    "   checkpoint size.\n"
    "2. EXE starts the clock.\n"
    "3. F1 logs a mark, F2 pauses.\n"
    "\n"
    "The gap line says how far ahead or\n"
    "behind the plan you are.\n"
    "\n"
    "Keys: EXE start/stop, F1 mark, F2\n"
    "pause, F3 finish, EXIT back.";

static char const help_uncert[] =
    "Uncertainty\n"
    "Arithmetic with errors, to 1 or 2\n"
    "significant figures.\n"
    "\n"
    "Type a value and its absolute error,\n"
    "pick an operation, type the second\n"
    "value and error.\n"
    "\n"
    "Chain mode keeps the result as the\n"
    "first value of the next step.\n"
    "\n"
    "Keys: F1..F3 operation, EXE apply,\n"
    "EXIT back.";

static char const help_cards[] =
    "Cards\n"
    "Flashcards with a Leitner schedule.\n"
    "\n"
    "A deck is one file in Bench/cards,\n"
    "one card per line:\n"
    "   question | answer\n"
    "\n"
    "Right answers move a card to the\n"
    "next box, wrong ones send it back.\n"
    "Progress is saved when the session\n"
    "ends, never during it.\n"
    "\n"
    "Keys: EXE reveal/mark, F1 right,\n"
    "F2 wrong, EXIT back.";

static char const help_notes[] =
    "Notes\n"
    "Reads .md and .txt from Bench/notes.\n"
    "\n"
    "Markdown headings, lists, quotes,\n"
    "bold and code are drawn in the\n"
    "palette. Long lines wrap; the wrap\n"
    "width is a setting.\n"
    "\n"
    "Keys: UP/DOWN scroll, EXE page down,\n"
    "ALPHA search, F2 headings, EXIT.";

static char const help_numtheory[] =
    "Number theory\n"
    "Nine functions, each with working.\n"
    "\n"
    "gcd, lcm, prime test, factorise,\n"
    "modular inverse, CRT, divisors,\n"
    "totient and base conversion.\n"
    "\n"
    "Factorisation shows a progress bar;\n"
    "EXIT cancels it.\n"
    "\n"
    "Keys: UP/DOWN choose, EXE open,\n"
    "EXIT back.";

static char const help_settings[] =
    "Settings\n"
    "Uncertainty figures, the Pacer\n"
    "checkpoint size and the Notes wrap\n"
    "width.\n"
    "\n"
    "Every value is also a line in\n"
    "Bench/state/settings.cfg, one per\n"
    "line, safe to edit by hand.\n"
    "\n"
    "The file is written only when a\n"
    "value changes.\n"
    "\n"
    "Keys: UP/DOWN choose, LEFT/RIGHT\n"
    "change, EXIT back.";

static char const help_about[] =
    "About\n"
    "Version, build date, target and the\n"
    "storage Bench uses.\n"
    "\n"
    "Bench never reads or changes Exam\n"
    "Mode. If the calculator is in Exam\n"
    "Mode the add-in does not run at\n"
    "all; that is the OS, not us.";


static char const help_function[] =
    "Function table\n"
    "A table and a value for f(x).\n"
    "\n"
    "1. Pick a slot f, g, h, p, q, r.\n"
    "2. EXE types the expression on the\n"
    "   keyboard: x, pi, e, + - * / ^,\n"
    "   sin cos tan ln log sqrt abs.\n"
    "3. F1 opens the table.\n"
    "\n"
    "In the table: LEFT/RIGHT move the\n"
    "start, F1 changes the step, F2 the\n"
    "row count, EXE probes one value.\n"
    "F4 switches degrees and radians;\n"
    "the mode shows in the title bar.\n"
    "\n"
    "Slots are written to\n"
    "Bench/state/exprs.txt when you\n"
    "leave the tool.\n"
    "\n"
    "Keys: EXE edit, F1 table, F3 clear.";


static char const help_sigfig[] =
    "Sig figs\n"
    "How many figures, and how to write\n"
    "them.\n"
    "\n"
    "UP/DOWN picks a mode, LEFT/RIGHT\n"
    "changes n, EXE types the number.\n"
    "\n"
    "Count: leading zeros never count,\n"
    "and trailing zeros count only after\n"
    "a decimal point. 0.004560 has 4.\n"
    "\n"
    "Round: half up on the decimal\n"
    "digits, so 1234.5 to 3 figures is\n"
    "1230 = 1.23 x 10^3.\n"
    "\n"
    "Practice asks a question; type the\n"
    "count and EXE marks it, F2 moves\n"
    "on.\n"
    "\n"
    "Keys: EXE type, F1 clear.";


static char const help_poly[] =
    "Polynomials\n"
    "Quadratics, expansion and rational\n"
    "roots.\n"
    "\n"
    "Three screens, F4 or F2 next and F3\n"
    "back:\n"
    "  Quadratic  a, b, c give the\n"
    "             discriminant, both roots\n"
    "             and the vertex.\n"
    "  Expand     (ax + b)^n to degree 8.\n"
    "  Roots      c0..cn as coefficients;\n"
    "             whole and half roots are\n"
    "             found, the rest is shown.\n"
    "\n"
    "Arrows move between fields, digits\n"
    "type, DEL deletes, F1 clears.  F2 on\n"
    "the roots screen adds a coefficient.\n"
    "\n"
    "Rounding is half up on the digits.";

/* ---- the table ---------------------------------------------------------- */

static tool_t const tools[] = {
    {
        .id = "pacer",
        .title = "Pacer",
        .subtitle = "min per mark",
        .category = TOOL_CAT_STUDY,
        .accent = TH_RED,
        .icon = icon_square,
        .screen = APP_SCREEN_PACER,
        .needs_file = false,
        .help = help_pacer,
    },
    {
        .id = "cards",
        .title = "Cards",
        .subtitle = "flashcards",
        .category = TOOL_CAT_STUDY,
        .accent = TH_YELLOW,
        .icon = icon_triangle,
        .screen = APP_SCREEN_CARDS,
        .needs_file = true,
        .help = help_cards,
    },
    {
        .id = "notes",
        .title = "Notes",
        .subtitle = "Markdown",
        .category = TOOL_CAT_STUDY,
        .accent = TH_TEAL,
        .icon = icon_half_circle,
        .screen = APP_SCREEN_NOTES,
        .needs_file = true,
        .help = help_notes,
    },
    {
        .id = "function",
        .title = "Function",
        .subtitle = "x and f(x)",
        .category = TOOL_CAT_MATH,
        .accent = TH_BLUE,
        .icon = icon_grid,
        .screen = APP_SCREEN_FUNCTION,
        .needs_file = false,
        .help = help_function,
    },
    {
        .id = "poly",
        .title = "Polynomials",
        .subtitle = "roots, expand",
        .category = TOOL_CAT_MATH,
        .accent = TH_VIOLET,
        .icon = icon_curve,
        .screen = APP_SCREEN_POLY,
        .needs_file = false,
        .help = help_poly,
    },
    {
        .id = "numtheory",
        .title = "Number theory",
        .subtitle = "gcd to CRT",
        .category = TOOL_CAT_MATH,
        .accent = TH_VIOLET,
        .icon = icon_bars,
        .screen = APP_SCREEN_NUMTHEORY,
        .needs_file = false,
        .help = help_numtheory,
    },
    {
        .id = "uncert",
        .title = "Uncertainty",
        .subtitle = "value and error",
        .category = TOOL_CAT_SCIENCE,
        .accent = TH_BLUE,
        .icon = icon_circle,
        .screen = APP_SCREEN_UNCERT,
        .needs_file = false,
        .help = help_uncert,
    },
    {
        .id = "sigfig",
        .title = "Sig figs",
        .subtitle = "round, standard",
        .category = TOOL_CAT_SCIENCE,
        .accent = TH_TEAL,
        .icon = icon_steps,
        .screen = APP_SCREEN_SIGFIG,
        .needs_file = false,
        .help = help_sigfig,
    },
    {
        .id = "settings",
        .title = "Settings",
        .subtitle = "options",
        .category = TOOL_CAT_TOOLS,
        .accent = TH_SHADE,
        .icon = icon_ring,
        .screen = APP_SCREEN_SETTINGS,
        .needs_file = false,
        .help = help_settings,
    },
    {
        .id = "about",
        .title = "About",
        .subtitle = "version",
        .category = TOOL_CAT_TOOLS,
        .accent = TH_INK,
        .icon = icon_frame,
        .screen = APP_SCREEN_ABOUT,
        .needs_file = false,
        .help = help_about,
    },
};

#define TOOL_COUNT ((int)(sizeof tools / sizeof tools[0]))

int tool_count(void)
{
    return TOOL_COUNT;
}

tool_t const *tool_at(int index)
{
    if (index < 0 || index >= TOOL_COUNT)
        return NULL;
    return &tools[index];
}

tool_t const *tool_by_id(char const *id)
{
    if (!id)
        return NULL;
    for (int i = 0; i < TOOL_COUNT; i++)
        if (tools[i].id[0] == id[0] && tools[i].id[1] == id[1])
        {
            char const *a = tools[i].id;
            char const *b = id;

            while (*a && *a == *b)
            {
                a++;
                b++;
            }
            if (!*a && !*b)
                return &tools[i];
        }
    return NULL;
}

int tool_index(tool_t const *tool)
{
    if (!tool)
        return -1;
    for (int i = 0; i < TOOL_COUNT; i++)
        if (&tools[i] == tool)
            return i;
    return -1;
}

/* ---- categories --------------------------------------------------------- */

static char const *const category_names[TOOL_CAT_COUNT] = {
    "Study", "Math", "Science", "Tools",
};

static char const *const category_hints[TOOL_CAT_COUNT] = {
    "practice, notes, cards", "solvers and proofs", "measurements",
    "settings and help",
};

int tool_category_count(void)
{
    return TOOL_CAT_COUNT;
}

char const *tool_category_name(tool_category_t category)
{
    if (category < 0 || category >= TOOL_CAT_COUNT)
        return "";
    return category_names[category];
}

char const *tool_category_hint(tool_category_t category)
{
    if (category < 0 || category >= TOOL_CAT_COUNT)
        return "";
    return category_hints[category];
}

int tool_category_size(tool_category_t category)
{
    int n = 0;

    for (int i = 0; i < TOOL_COUNT; i++)
        if (tools[i].category == category)
            n++;
    return n;
}

tool_t const *tool_category_at(tool_category_t category, int index)
{
    int seen = 0;

    if (index < 0)
        return NULL;
    for (int i = 0; i < TOOL_COUNT; i++)
        if (tools[i].category == category)
        {
            if (seen == index)
                return &tools[i];
            seen++;
        }
    return NULL;
}

int tool_category_pages(tool_category_t category)
{
    int const n = tool_category_size(category);

    return n == 0 ? 1 : (n + TILES_PER_PAGE - 1) / TILES_PER_PAGE;
}
