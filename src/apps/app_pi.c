/* app_pi.c - the pi game.
 *
 * Spec: the request for "a pi game".  The rules are the smallest ones that make
 * remembering digits a game rather than a recital:
 *
 *   - the digits you have earned are always on screen, grouped as you would
 *     read them off a page
 *   - type the next ones, any number of them, and press EXE
 *   - every digit right earns a place further along; the first wrong one costs
 *     a life and the entry is cleared
 *   - F2 gives away the next digit, and costs a life: a hint has to cost
 *     something or it is not a game
 *   - three lives; the round ends when they are gone, and the page shows how
 *     far you got and the best of this session
 *
 * The digits and the comparison are core/pi.c, which is tested against a spigot
 * computation rather than against a copy of itself.  Nothing here is written to
 * the card: the best score lasts as long as the add-in is open, which AGENTS.md
 * asks for (no file writes outside a session end).
 *
 * Unverified on device. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/pi.h"
#include "platform/input.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

#define LIVES 3
#define ENTRY_MAX 12

static int at;                 /* digits earned, counting the leading 3 */
static int lives;
static int best;               /* best of this session, in digits */
static int round_over;
static int gained;             /* the last batch that was right */
static int lost_at;            /* where the last wrong digit was, -1 for none */
static char entry[ENTRY_MAX];
static int cursor;
static char shown[96];

void app_pi_init(void)
{
    at = 1;
    lives = LIVES;
    round_over = 0;
    gained = 0;
    lost_at = -1;
    cursor = 0;
    entry[0] = 0;
}

int app_pi_timeout(void)
{
    return 0;
}

static void submit(void)
{
    int const n = pi_match(at, entry, cursor);

    lost_at = -1;
    gained = n;
    if (n == cursor && cursor > 0)
    {
        at += n;
        if (at > best)
            best = at;
    }
    else
    {
        lost_at = at + n;
        lives--;
        if (lives <= 0)
        {
            lives = 0;
            round_over = 1;
        }
    }
    cursor = 0;
    entry[0] = 0;
}

/* A small decimal, for the lines that mix text and a number. */
static char const *u_text(int v)
{
    static char buf[8];
    char d[8];
    int k = 0;
    int m = 0;

    if (v < 0)
        v = 0;
    if (v == 0)
        d[k++] = '0';
    while (v > 0)
    {
        d[k++] = (char)('0' + v % 10);
        v /= 10;
    }
    while (k > 0)
        buf[m++] = d[--k];
    buf[m] = 0;
    return buf;
}

static void draw_lives(int y, int right)
{
    for (int i = 0; i < LIVES; i++)
    {
        int const x = right - (LIVES - i) * 10;

        if (i < lives)
            scr_fill(x, y, 7, 7, TH_RED);
        else
            scr_frame(x, y, 7, 7, TH_INK, 1);
    }
}

void app_pi_draw(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int const w = SCR_W - 2 * TH_PAD;
    char line[80];

    scr_clear(TH_PAPER);
    w_title("Pi game", round_over ? "F1 again" : "F1 clear");

    /* The digits earned so far.  pi_text() can fill 96 bytes at this length, so
     * a long game wraps rather than being cut in the middle of a group. */
    {
        /* The last line still fits: 40 digits is 48 characters. */
        int const count = at > 40 ? 40 : at;

        pi_text(count, shown, (int)sizeof shown);
        scr_text_ellipsis(TH_PAD, y, shown, TH_INK, w);
    }
    y += TH_LINE + 4;

    draw_lives(y, SCR_W - TH_PAD);
    scr_text(TH_PAD, y - 1, "lives", TH_INK);
    y += 16;

    if (round_over)
    {
        scr_text(TH_PAD, y, "Out of lives.", TH_INK);
        y += TH_LINE;
        strcpy(line, "You reached digit ");
        nt_append(line, (int)sizeof line, u_text(at));
        nt_append(line, (int)sizeof line, " of pi.");
        scr_text(TH_PAD, y, line, TH_INK);
        y += TH_LINE;
        strcpy(line, "Best this session: ");
        nt_append(line, (int)sizeof line, u_text(best));
        scr_text(TH_PAD, y, line, TH_INK);
        y += TH_LINE + 4;
        scr_text(TH_PAD, y, "F1 starts again.", TH_INK);
    }
    else
    {
        char field[ENTRY_MAX + 1];

        if (lost_at >= 0)
        {
            scr_fill(TH_PAD, y, w, TH_LINE + 6, TH_RED);
            scr_text_ellipsis(TH_PAD + 4, y + 3, "not that one - a life gone",
                              TH_PAPER, w - 8);
            y += TH_LINE + 10;
        }

        strcpy(line, "next digit");
        scr_text(TH_PAD, y, line, TH_INK);
        y += TH_LINE + 2;

        strcpy(field, entry);
        if (!field[0])
            strcpy(field, "_");
        w_field(TH_PAD, y, w, w_field_h(), ">", field, 1);
        y += 24;

        if (gained > 0)
        {
            strcpy(line, "+");
            nt_append(line, (int)sizeof line, u_text(gained));
            nt_append(line, (int)sizeof line, " digits");
            scr_text(TH_PAD, y, line, TH_BLUE);
            y += TH_LINE;
        }
        scr_text(TH_PAD, y, "Type the next digits, EXE checks.", TH_INK);
        y += TH_LINE + 4;
        scr_text(TH_PAD, y, "F2 shows the next digit and costs a", TH_INK);
        scr_text(TH_PAD, y + TH_LINE, "life. Three wrong ones end the round.", TH_INK);
    }

    w_softkeys((char const *const[6]){ round_over ? "Again" : "Clear",
                                       round_over ? "" : "Hint", "", "", "Help",
                                       "" },
               -1);
    scr_present();
}

app_result_t app_pi_key(bench_event_t ev)
{
    switch (ev.key)
    {
    case BK_MENU:
        return APP_QUIT;
    case BK_EXIT:
        return APP_BACK;
    case BK_F5:
        app_help_open(tool_by_id("pi"));
        return APP_CONTINUE;
    case BK_F1:
        app_pi_init();
        break;
    case BK_F2:
        /* A hint costs a life, and shows the digit in the field so it can be
         * typed: the point is to keep playing, not to be told. */
        if (!round_over && lives > 1)
        {
            int const d = pi_digit(at);

            if (d >= 0)
            {
                lives--;
                entry[0] = (char)('0' + d);
                entry[1] = 0;
                cursor = 1;
            }
        }
        break;
    case BK_DEL:
        if (cursor > 0)
            entry[--cursor] = 0;
        break;
    case BK_EXE:
        if (!round_over && cursor > 0)
            submit();
        break;
    default:
        if (!round_over && ev.key == BK_DIGIT && cursor < ENTRY_MAX - 1)
        {
            entry[cursor++] = ev.ch;
            entry[cursor] = 0;
        }
        break;
    }
    return APP_CONTINUE;
}
