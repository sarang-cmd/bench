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

/* A target: a ring with a solid centre, for the root finder. */
static void icon_target(int x, int y, int fill, int edge)
{
    sh_ring_icon(x + 10, y + 10, 8, 3, fill, edge, 1);
    sh_circle_icon(x + 10, y + 10, 3, fill, edge, 1);
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

/* A triangle outline: the triangle solver. */
static void icon_tri_outline(int x, int y, int fill, int edge)
{
    int const top = y + 2, bottom = y + 17;

    for (int i = 0; i < 16; i++)
    {
        int const half = (15 * i) / 15 / 2;

        scr_fill(x + 2 + 8 - half, top + i, 1, 1, fill);
        scr_fill(x + 2 + 8 + half, top + i, 1, 1, fill);
    }
    scr_hline(x + 2, bottom, 17, fill);
    (void)edge;
}

/* Steps that get further apart: a sequence. */
static void icon_steps_up(int x, int y, int fill, int edge)
{
    for (int i = 0; i < 3; i++)
        scr_fill(x + 3 + i * 5, y + 17 - (i + 1) * 4, 4, (i + 1) * 4, fill);
    (void)edge;
}

/* An "i" in dots: two squares, the dot over the stem. */
/* Prime factors: one block, then two smaller ones under it, like a tree that
 * keeps splitting.  Distinct from icon_bars (equal columns) and icon_steps. */
static void icon_split(int x, int y, int fill, int edge)
{
    sh_square_icon(x + 6, y + 1, 8, fill, edge, 1);
    sh_square_icon(x + 1, y + 12, 7, fill, edge, 1);
    sh_square_icon(x + 12, y + 12, 7, fill, edge, 1);
}

static void icon_i(int x, int y, int fill, int edge)
{
    /* The dot: a square outlined, then filled. */
    scr_fill(x + 7, y + 1, 6, 6, edge);
    scr_fill(x + 8, y + 2, 4, 4, fill);
    /* The stem, twice as tall as it is wide. */
    scr_fill(x + 7, y + 9, 6, 10, edge);
    scr_fill(x + 8, y + 10, 4, 8, fill);
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
    "Eight rows. The first five change\n"
    "what a tool shows: uncertainty\n"
    "figures, the Pacer checkpoint\n"
    "size, the Notes wrap width, the\n"
    "unit Triangle opens in, and the\n"
    "first-run line.\n"
    "\n"
    "Opens on: which category the home\n"
    "screen starts on. Show hint line:\n"
    "no hides the first-run line for\n"
    "good. Pacer clock red at: how many\n"
    "minutes left turn the clock red,\n"
    "0 for never.\n"
    "\n"
    "Every value is also a line in\n"
    "Bench/state/settings.cfg, safe to\n"
    "edit by hand. The file is written\n"
    "only when a value changes.\n"
    "\n"
    "Keys: UP/DOWN choose, LEFT/RIGHT\n"
    "change, EXIT back.";

static char const help_factor[] =
    "Prime factors\n"
    "Type a whole number and press EXE.\n"
    "The page shows the factorisation\n"
    "with powers, then the four things\n"
    "that follow from it.\n"
    "\n"
    "360 = 2^3 x 3^2 x 5\n"
    "  divisors 24, their sum 1170\n"
    "  totient 96\n"
    "  square-free part 30\n"
    "\n"
    "Square-free part is the primes\n"
    "multiplied with their powers thrown\n"
    "away: 30 for 360. If it equals the\n"
    "number, the number is square free.\n"
    "\n"
    "Up to 18 digits. F2 halves the\n"
    "number and factors again.\n"
    "\n"
    "Keys: digits, DEL, F1 clear, F2\n"
    "halve, EXE factor, EXIT back.";

static char const help_percent[] =
    "Percentages\n"
    "Four questions, chosen with UP/DOWN.\n"
    "\n"
    "Change: from 40 to 46 is a rise\n"
    "of 15%.\n"
    "\n"
    "Add or take: 40 with 15% added is\n"
    "46, and the page shows the working.\n"
    "\n"
    "Reverse: something costs 34 after\n"
    "15% off, so it was 40. Dividing by\n"
    "0.85, not taking 15% back off.\n"
    "\n"
    "Compound: 500 at 4.5% for 7 years\n"
    "is 680.43, with interest added each\n"
    "year. A negative rate depreciates.\n"
    "\n"
    "A percentage change from zero, and\n"
    "a 100% discount, have no answer and\n"
    "the page says why.\n"
    "\n"
    "Keys: UP/DOWN question, LEFT/RIGHT\n"
    "field, EXE work out, F1 clear.";

static char const help_formula[] =
    "Formula book\n"
    "The IB formula and data booklets,\n"
    "as plain text on the card.\n"
    "\n"
    "Copy them into Bench/formulas, one\n"
    ".txt per booklet. The list shows\n"
    "every file in that folder, so a\n"
    "booklet you write yourself appears\n"
    "there too.\n"
    "\n"
    "In the book: UP/DOWN scroll a line,\n"
    "LEFT/RIGHT jump a section, F1 goes\n"
    "to the top. Headings are red, blue\n"
    "and ink by level.\n"
    "\n"
    "# is a booklet title, ## a topic,\n"
    "### a sub-topic, one space after\n"
    "the hashes.\n"
    "\n"
    "Keys: EXE open, LEFT/RIGHT section,\n"
    "F1 top, EXIT back.";

static char const help_pi[] =
    "Pi game\n"
    "The digits of pi, from memory.\n"
    "\n"
    "The digits you have earned stay on\n"
    "screen, grouped five at a time. Type\n"
    "the next ones and press EXE: every\n"
    "digit right moves you along, and the\n"
    "first wrong one costs a life.\n"
    "\n"
    "Three lives. F2 shows the next digit\n"
    "and costs one, so a hint is never\n"
    "free. When the lives are gone the\n"
    "page shows how far you got and the\n"
    "best of this session.\n"
    "\n"
    "Nothing is written to the card: the\n"
    "best score is gone when the add-in\n"
    "closes.\n"
    "\n"
    "Keys: digits, DEL, EXE check, F1\n"
    "again, F2 hint, EXIT back.";

static char const help_about[] =
    "About Bench\n"
    "Version, build date, target and the\n"
    "storage Bench uses, plus who made\n"
    "it and how: the maths is plain C\n"
    "tested on a PC, the screens are a\n"
    "thin layer over gint, and every\n"
    "picture in the docs is a real frame\n"
    "from the simulator.\n"
    "\n"
    "UP/DOWN scroll the page.\n"
    "\n"
    "Bench never reads or changes Exam\n"
    "Mode. If the calculator is in Exam\n"
    "Mode the add-in does not start at\n"
    "all, which is the OS's rule, not\n"
    "Bench's.\n"
    "\n"
    "Keys: UP/DOWN scroll, EXIT back.";


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


static char const help_triangle[] =
    "Triangle\n"
    "Three sides and three angles. Fill\n"
    "in what you know and leave the rest\n"
    "blank.\n"
    "\n"
    "Give any three values, at least one\n"
    "a side: SSS, SAS, ASA and AAS all\n"
    "solve, and the fields fill in.\n"
    "\n"
    "SSA - two sides and an angle not\n"
    "between them - can have two answers.\n"
    "The title bar then says so and F2\n"
    "swaps between the two triangles.\n"
    "\n"
    "Area and perimeter are under the\n"
    "fields.  F3 degrees, F4 radians.\n"
    "\n"
    "Keys: arrows move, digits type,\n"
    "EXE solves, F1 clears.";


static char const help_seq[] =
    "Sequences\n"
    "Arithmetic and geometric terms,\n"
    "their sums, and binomials.\n"
    "\n"
    "Three screens, F4 next and F3 back.\n"
    "\n"
    "  Arithmetic  a, d, n: the nth term\n"
    "              and the sum, then the\n"
    "              first few terms.\n"
    "  Geometric   a, r, n: as above, plus\n"
    "              the sum to infinity when\n"
    "              |r| < 1.\n"
    "  Binomial    n choose k exactly, and\n"
    "              row n of Pascal.\n"
    "\n"
    "Terms count from 1, so the first term\n"
    "is n = 1.  C(n,k) is refused rather\n"
    "than rounded when it passes 64 bits.\n"
    "\n"
    "Keys: arrows move, digits type,\n"
    "EXE solves, F1 clears.";


static char const help_cplx[] =
    "Complex numbers\n"
    "Two numbers a and b, one operation\n"
    "at a time.\n"
    "\n"
    "F2 picks the operation: a + b,\n"
    "a - b, a x b, a / b and a ^ n, where\n"
    "n is the real part of b.\n"
    "\n"
    "The result is written three ways:\n"
    "rectangular (-1 + 2i), its modulus\n"
    "and argument, and polar form.\n"
    "\n"
    "F3 shows the argument in degrees,\n"
    "F4 in radians.\n"
    "\n"
    "Keys: arrows move, digits type,\n"
    "DEL deletes, F1 clears, EXIT home.";


static char const help_roots[] =
    "Root finder\n"
    "Solves f(x) = 0 one step at a time,\n"
    "so the convergence is visible.\n"
    "\n"
    "F2 picks the method:\n"
    "  bisection needs a and b with a\n"
    "    sign change between them\n"
    "  Newton needs one guess x0\n"
    "  secant needs two guesses x0, x1\n"
    "\n"
    "F3 solves. The table lists n, x and\n"
    "f(x); the plot marks every iterate.\n"
    "\n"
    "Radians always, whatever the angle\n"
    "setting says. Tolerance 1e-6, at most\n"
    "50 steps.\n"
    "\n"
    "F4 lists the last eight answers, so a\n"
    "root does not vanish when the fields\n"
    "change. Nothing is written to storage.\n"
    "\n"
    "Keys: EXE edits f or solves, F1 clears,\n"
    "F2 method, F4 history, arrows move\n"
    "and scroll.";


static char const help_stats[] =
    "Statistics\n"
    "Type a value, press EXE to add it,\n"
    "and the summary is worked out as\n"
    "you go. Up to 200 values.\n"
    "\n"
    "F2 switches between one list and\n"
    "paired data (x, then y). Pairs give\n"
    "the line y = a + bx, r and r squared.\n"
    "\n"
    "F3 picks the quartile method:\n"
    "  median  halves split through the\n"
    "          median\n"
    "  sheet   (n-1)p + 1, interpolated\n"
    "\n"
    "F4 shows the working for the standard\n"
    "deviation. F1 clears the list.\n"
    "\n"
    "Keys: digits type, EXE adds, DEL takes\n"
    "back a digit or a value.";

static char const help_suvat[] =
    "SUVAT and projectiles\n"
    "Type any three of s, u, v, a and t.\n"
    "The other two are worked out, with\n"
    "the equation that gave each one.\n"
    "\n"
    "Use the arrows to pick a quantity,\n"
    "type the number, press EXE. DEL on\n"
    "an empty field un-knows it.\n"
    "\n"
    "F2 draws the flight path: range,\n"
    "greatest height and time in the air\n"
    "for a launch speed, angle and height.\n"
    "\n"
    "F3 switches g between 9.81 and 10.\n"
    "Where a quadratic gives two times,\n"
    "the first is shown and the second\n"
    "is mentioned under it.\n"
    "\n"
    "Keys: digits type, EXE keeps, DEL\n"
    "takes back, F1 starts again.";

static char const help_moles[] =
    "Moles and solutions\n"
    "Five calculations, one per mode:\n"
    "moles from a mass, concentration,\n"
    "the gas equation, dilution, and\n"
    "yield or atom economy.\n"
    "\n"
    "Each mode opens on a worked\n"
    "example; F1 puts it back.\n"
    "\n"
    "The unit under each number is the\n"
    "one the formula wants: volumes in\n"
    "dm3, so 25 cm3 is 0.025, and\n"
    "temperature in kelvin.\n"
    "\n"
    "R is 8.314, for p in kPa and V in\n"
    "dm3. A zero where a formula divides\n"
    "says so instead of printing an\n"
    "infinity.\n"
    "\n"
    "Keys: UP/DOWN pick a field, digits\n"
    "type, F3/F4 change mode, DEL takes\n"
    "a digit back.";

static char const help_base[] =
    "Bases and bits\n"
    "One number in base 2 to 36, and\n"
    "the same number as two's complement\n"
    "at 8, 16 and 32 bits.\n"
    "\n"
    "F1 switches between converting\n"
    "and the bitwise operators.\n"
    "\n"
    "F3 changes the entry base, or the\n"
    "operator: AND, OR, XOR, shift left\n"
    "and shift right. A shift uses the\n"
    "count in b and refuses 63 or more.\n"
    "\n"
    "F2 opens the letter keyboard for\n"
    "digits above 9, since the keypad\n"
    "has no A to F keys this add-in may\n"
    "count on. Digits can still be\n"
    "typed straight in.\n"
    "\n"
    "Keys: digits type, DEL takes one\n"
    "back, F4 swaps a and b.";

static char const help_graph[] =
    "Lab graph\n"
    "Type a measured pair and its error,\n"
    "press EXE, and repeat. F2 draws\n"
    "the graph with a bar up and down\n"
    "through every point and the line of\n"
    "best fit.\n"
    "\n"
    "The gradient comes with its standard\n"
    "error, from the residuals, and r\n"
    "squared says how well the line\n"
    "fits. Fewer than three points leave\n"
    "nothing to measure the error against,\n"
    "so it is left off.\n"
    "\n"
    "The axes hold the whole bar, not\n"
    "just the centre, so a measurement\n"
    "never runs off the top.\n"
    "\n"
    "Up to twelve points. F1 clears,\n"
    "F3 puts the example back, DEL on an\n"
    "empty field drops the last point.\n"
    "\n"
    "Keys: digits type, LEFT/RIGHT pick\n"
    "a field, EXE adds the point.";

static char const help_log[] =
    "Study log\n"
    "What you did, for how long, and\n"
    "on which day, in a text file you\n"
    "can also edit by hand:\n"
    "\n"
    "  2026-10-31 25 pacer\n"
    "\n"
    "A broken line is skipped and\n"
    "counted, never added up wrongly.\n"
    "\n"
    "F1 adds a session: type the\n"
    "minutes, UP/DOWN picks the tool,\n"
    "LEFT/RIGHT moves between the three\n"
    "fields, EXE saves it. F2 deletes\n"
    "the last one, which is the mistake\n"
    "you will make.\n"
    "\n"
    "Bench cannot read the calendar, so\n"
    "a new entry takes the day after the\n"
    "newest one in the log; UP/DOWN on\n"
    "the day moves it. The file is\n"
    "written once, when you leave.";

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
        .id = "log",
        .title = "Study log",
        .subtitle = "minutes per day",
        .category = TOOL_CAT_STUDY,
        .accent = TH_YELLOW,
        .icon = icon_bars,
        .screen = APP_SCREEN_LOG,
        .needs_file = false,
        .help = help_log,
    },
    {
        .id = "cplx",
        .title = "Complex",
        .subtitle = "a + bi",
        .category = TOOL_CAT_MATH,
        .accent = TH_RED,
        .icon = icon_i,
        .screen = APP_SCREEN_CPLX,
        .needs_file = false,
        .help = help_cplx,
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
        .id = "seq",
        .title = "Sequences",
        .subtitle = "terms, sums",
        .category = TOOL_CAT_MATH,
        .accent = TH_YELLOW,
        .icon = icon_steps_up,
        .screen = APP_SCREEN_SEQ,
        .needs_file = false,
        .help = help_seq,
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
        .id = "roots",
        .title = "Root finder",
        .subtitle = "bisect, Newton",
        .category = TOOL_CAT_MATH,
        .accent = TH_VIOLET,
        .icon = icon_target,
        .screen = APP_SCREEN_ROOTS,
        .needs_file = false,
        .help = help_roots,
    },
    {
        .id = "triangle",
        .title = "Triangle",
        .subtitle = "sides, angles",
        .category = TOOL_CAT_MATH,
        .accent = TH_TEAL,
        .icon = icon_tri_outline,
        .screen = APP_SCREEN_TRIANGLE,
        .needs_file = false,
        .help = help_triangle,
    },
    {
        .id = "base",
        .title = "Base and bits",
        .subtitle = "hex, shifts",
        .category = TOOL_CAT_MATH,
        .accent = TH_RED,
        .icon = icon_bars,
        .screen = APP_SCREEN_BASE,
        .needs_file = false,
        .help = help_base,
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
        .id = "factor",
        .title = "Prime factors",
        .subtitle = "factorise n",
        .category = TOOL_CAT_MATH,
        .accent = TH_TEAL,
        .icon = icon_split,
        .screen = APP_SCREEN_FACTOR,
        .needs_file = false,
        .help = help_factor,
    },
    {
        .id = "pi",
        .title = "Pi game",
        .subtitle = "digits from memory",
        .category = TOOL_CAT_STUDY,
        .accent = TH_BLUE,
        .icon = icon_ring,
        .screen = APP_SCREEN_PI,
        .needs_file = false,
        .help = help_pi,
    },
    {
        .id = "formulas",
        .title = "Formula book",
        .subtitle = "IB booklets",
        .category = TOOL_CAT_STUDY,
        .accent = TH_YELLOW,
        .icon = icon_frame,
        .screen = APP_SCREEN_FORMULA,
        .needs_file = true,
        .help = help_formula,
    },
    {
        .id = "percent",
        .title = "Percentages",
        .subtitle = "change, growth",
        .category = TOOL_CAT_MATH,
        .accent = TH_YELLOW,
        .icon = icon_half_circle,
        .screen = APP_SCREEN_PERCENT,
        .needs_file = false,
        .help = help_percent,
    },
    {
        .id = "stats",
        .title = "Statistics",
        .subtitle = "mean to regression",
        .category = TOOL_CAT_MATH,
        .accent = TH_TEAL,
        .icon = icon_bars,
        .screen = APP_SCREEN_STATS,
        .needs_file = false,
        .help = help_stats,
    },
    {
        .id = "graph",
        .title = "Lab graph",
        .subtitle = "error bars",
        .category = TOOL_CAT_SCIENCE,
        .accent = TH_BLUE,
        .icon = icon_bars,
        .screen = APP_SCREEN_GRAPH,
        .needs_file = false,
        .help = help_graph,
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
        .id = "suvat",
        .title = "SUVAT",
        .subtitle = "motion, projectiles",
        .category = TOOL_CAT_SCIENCE,
        .accent = TH_YELLOW,
        .icon = icon_circle,
        .screen = APP_SCREEN_SUVAT,
        .needs_file = false,
        .help = help_suvat,
    },
    {
        .id = "moles",
        .title = "Moles",
        .subtitle = "mass, gas, conc",
        .category = TOOL_CAT_SCIENCE,
        .accent = TH_VIOLET,
        .icon = icon_circle,
        .screen = APP_SCREEN_MOLES,
        .needs_file = false,
        .help = help_moles,
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
