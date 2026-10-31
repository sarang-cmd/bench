/* main.c - Bench entry point.
 *
 * M0 build (version 0.0.1): clear the screen to paper, draw one red square,
 * wait for a key press, exit.  No file, timer or text entry code here; those
 * arrive at the milestones that need them.
 *
 * Colours: the plan's Bauhaus palette, converted to the display's 16-bit
 * R5G6B5 format by C_RGB (gint/display-cg.h).
 *     paper #f4f3ee -> r 244, g 243, b 238 -> (30, 60, 29)
 *     ink   #2f2d2e -> r  47, g  45, b  46 -> ( 5, 11,  5)
 *     red   #d92d27 -> r 217, g  45, b  39 -> (27, 11,  4)
 * The palette moves to src/ui/theme.h in M2. */

#include <gint/display.h>
#include <gint/keyboard.h>

#define BENCH_PAPER C_RGB(30, 60, 29)
#define BENCH_RED C_RGB(27, 11, 4)

int main(void)
{
    /* The OS shows a 384x216 window inside gint's 396x224 surface
     * (gint/display-cg.h), so the visible area starts at (6, 4). */
    int const cx = 6 + 384 / 2;
    int const cy = 4 + 216 / 2;
    int const half = 48; /* a 97x97 square */

    dclear(BENCH_PAPER);
    drect(cx - half, cy - half, cx + half, cy + half, BENCH_RED);
    dupdate();

    getkey();
    return 1;
}
