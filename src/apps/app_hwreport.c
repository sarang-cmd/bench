/* app_hwreport.c - the eight assumptions Bench makes about the calculator,
 * marked on the device and written out as a text file.
 *
 * Spec: docs/STATUS.md "Assumptions that still need the calculator" and
 * docs/HARDWARE.md, the checklist the user runs.  Reached with F3 from
 * Settings; there is no tile for it, because it is a diagnostic, not a tool.
 *
 * Keys: UP/DOWN pick a row, LEFT/RIGHT (or EXE) marks it unknown -> works ->
 * fails, F1 clears every mark, F2 writes state/hwreport.txt, EXIT back.
 * Files: state/hwreport.txt, written only when F2 is pressed. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/hwreport.h"
#include "platform/screen.h"
#include "platform/store.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"
#include "version.h"

#define HWREPORT_PATH "state/hwreport.txt"

/* Not a registry row: the hardware report is a diagnostic screen, not a tool,
 * so it carries its own help text instead of a tile. */
static tool_t const report_help = {
    "hwreport", "Hardware report", "what the device does", TOOL_CAT_TOOLS,
    TH_TEAL, NULL, APP_SCREEN_HWREPORT, false,
    "Hardware report\n"
    "The eight things Bench assumes about\n"
    "the calculator, from docs/STATUS.md.\n"
    "\n"
    "Run docs/HARDWARE.md on a real fx-CG\n"
    "50 and mark each row: LEFT/RIGHT\n"
    "cycle not tried, works, fails.\n"
    "\n"
    "F2 writes state/hwreport.txt, a plain\n"
    "text file you can copy off the\n"
    "calculator and paste into the report.\n"
    "Nothing is written until you press F2.\n"
    "\n"
    "F1 clears every mark. EXIT goes back.\n"
    "\n"
    "Keys: UP/DOWN pick, LEFT/RIGHT mark,\n"
    "F1 clear, F2 write, EXIT back.",
};

static int row;
static hw_state_t state[HWREPORT_ITEMS];
static int saved_what;   /* 0 nothing yet, 1 written, -1 failed */

void app_hwreport_init(void)
{
    row = 0;
    saved_what = 0;
    hwreport_reset(state);
}

int app_hwreport_timeout(void)
{
    return 0;
}

void app_hwreport_draw(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int w = SCR_W - 2 * TH_PAD;
    char num[8];
    int marked = hwreport_marked(state);

    scr_clear(TH_PAPER);
    w_title("Hardware report", "mark what the device does");
    y = TH_CONTENT_Y + TH_PAD;

    scr_text(TH_PAD, y, "Tried on an fx-CG 50? Mark each row.", TH_INK);
    y += TH_LINE + 2;

    for (int i = 0; i < HWREPORT_ITEMS; i++)
    {
        w_row(TH_PAD, y, w, hwreport_label(i),
              hwreport_state_text(state[i]), row == i);
        y += TH_LINE;
    }

    y += 2;
    {
        char line[32];

        /* "marked 3 of 8", built without float printf. */
        strcpy(line, "marked ");
        num[0] = 0;
        nt_u64((uint64_t)marked, num, (int)sizeof num);
        nt_append(line, (int)sizeof line, num);
        nt_append(line, (int)sizeof line, " of 8");
        scr_text(TH_PAD, y, line, TH_INK);
    }

    y += TH_LINE + 2;
    if (saved_what == 1)
        scr_text(TH_PAD, y, "Saved state/hwreport.txt", TH_INK);
    else if (saved_what == -1)
        scr_text(TH_PAD, y, "Could not write the report", TH_INK);
    else
        scr_text(TH_PAD, y, "F2 writes it to state/hwreport.txt", TH_INK);

    w_softkeys((char const *const[6]){ "Clear", "", "Write", "", "", "" }, -1);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "UP/DOWN pick   LEFT/RIGHT mark   EXIT back", TH_INK);
    scr_present();
}

static void write_report(void)
{
    static char buf[1024];
    int n = hwreport_format(BENCH_VERSION, state, buf, (int)sizeof buf);

    if (n <= 0)
        saved_what = -1;
    else
        saved_what = store_write(HWREPORT_PATH, buf, n) == STORE_OK ? 1 : -1;
}

app_result_t app_hwreport_key(bench_event_t ev)
{
    if (ev.key == BK_EXIT)
        return APP_BACK;
    if (ev.key == BK_F1)
    {
        hwreport_reset(state);
        saved_what = 0;
        return APP_CONTINUE;
    }
    if (ev.key == BK_F2)
    {
        write_report();
        return APP_CONTINUE;
    }
    if (ev.key == BK_F5)
    {
        app_help_open(&report_help);
        return APP_CONTINUE;
    }
    if (ev.key == BK_UP && row > 0)
        row--;
    else if (ev.key == BK_DOWN && row < HWREPORT_ITEMS - 1)
        row++;
    else if (ev.key == BK_LEFT || ev.key == BK_RIGHT || ev.key == BK_EXE)
    {
        if (ev.key == BK_LEFT)
        {
            /* LEFT goes back one step, so a mis-tap costs one press, not two. */
            state[row] = state[row] == HW_UNKNOWN   ? HW_FAILS
                         : state[row] == HW_WORKS ? HW_UNKNOWN
                                                  : HW_WORKS;
        }
        else
            state[row] = hwreport_next(state[row]);
        saved_what = 0;
    }
    return APP_CONTINUE;
}
