/* app_settings.c - the settings screen and the file it lives in.
 *
 * Spec: TASK-M2 ("Settings file: load at start, save on change only") and
 * docs/plan.md section 6.2 for the uncertainty setting.
 *
 * Keys: UP/DOWN pick a row, LEFT/RIGHT change it, EXE also changes it,
 * F6 About, EXIT back.  The file is written the moment a value changes, never
 * on a timer and never inside a loop. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/settings.h"
#include "platform/screen.h"
#include "platform/store.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

#define SETTINGS_PATH "state/settings.cfg"

static int row;
static bool saved_ok = true;
static int ignored_lines;

void app_settings_init(void)
{
    row = 0;
    saved_ok = true;
    ignored_lines = 0;
}

int app_settings_timeout(void)
{
    return 0;
}

/* Load the file.  Called by main.c at start-up; the defaults survive anything
 * the file says. */
void app_settings_load(bench_settings_t *cfg)
{
    static char buf[SETTINGS_BUFSIZE * 2];
    int len = 0;
    int ignored = 0;

    settings_defaults(cfg);
    if (store_read(SETTINGS_PATH, buf, (int)sizeof buf - 1, &len) == STORE_OK)
    {
        buf[len] = 0;
        settings_parse(buf, len, cfg, &ignored);
        ignored_lines = ignored;
    }
    else
    {
        /* No file yet: write the defaults once so the user can edit it. */
        ignored_lines = 0;
        app_cfg_save();
    }
}

/* Only writes when the values differ from the file on disk. */
void app_cfg_save(void)
{
    char text[SETTINGS_BUFSIZE];
    int n = settings_format(app_cfg(), text, (int)sizeof text);

    if (n > 0)
        saved_ok = store_write(SETTINGS_PATH, text, n) == STORE_OK;
}

static void change(int delta)
{
    bench_settings_t *cfg = app_cfg();

    if (row == 0)
        cfg->uncert_sig = settings_clamp_uncert_sig(cfg->uncert_sig + delta);
    else if (row == 1)
        cfg->pacer_checkpoint =
            settings_clamp_checkpoint(cfg->pacer_checkpoint + delta);
    else if (row == 3)
        cfg->angle_unit = settings_clamp_angle(
            cfg->angle_unit == SETTINGS_ANGLE_RAD ? SETTINGS_ANGLE_DEG
                                                  : SETTINGS_ANGLE_RAD);
    else if (row == 4)
        cfg->tour_done = settings_clamp_flag(cfg->tour_done + delta);
    else
    {
        int v = cfg->notes_wrap;

        if (delta > 0)
            v += 25;
        else
            v -= 25;
        cfg->notes_wrap = settings_clamp_wrap(v);
        if (v > 0 && cfg->notes_wrap == 0)
            cfg->notes_wrap = 50;   /* never land on "auto" by accident */
    }
    app_cfg_save();
}

void app_settings_draw(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int w = SCR_W - 2 * TH_PAD;
    bench_settings_t *cfg = app_cfg();
    char num[8];

    scr_clear(TH_PAPER);
    w_title("Settings", "F6 About");
    y = TH_CONTENT_Y + TH_PAD;
    y += TH_LINE + 4;

    w_row(TH_PAD, y, w, "Uncertainty figures",
          cfg->uncert_sig == 2 ? "1 or 2" : "1",
          row == 0);
    y += TH_LINE;
    nt_u64(cfg->pacer_checkpoint, num, (int)sizeof num);
    w_row(TH_PAD, y, w, "Pacer checkpoint", num, row == 1);
    y += TH_LINE;
    if (cfg->notes_wrap == 0)
        strcpy(num, "auto");
    else
    {
        nt_u64(cfg->notes_wrap, num, (int)sizeof num);
        nt_append(num, (int)sizeof num, "%");
    }
    w_row(TH_PAD, y, w, "Note width", num, row == 2);
    y += TH_LINE;
    w_row(TH_PAD, y, w, "Angles default", settings_angle_text(cfg->angle_unit), row == 3);
    y += TH_LINE;
    w_row(TH_PAD, y, w, "First-run hint", cfg->tour_done ? "read" : "showing",
          row == 4);
    y += TH_LINE + 6;

    scr_text(TH_PAD, y, settings_uncert_sig_text(cfg->uncert_sig), TH_INK);
    y += TH_LINE + 2;
    scr_text(TH_PAD, y, saved_ok ? "Saved to state/settings.cfg"
                                 : "Could not write the settings file",
             TH_INK);
    if (ignored_lines > 0)
    {
        char line[48];

        nt_u64((uint64_t)ignored_lines, num, (int)sizeof num);
        strcpy(line, num);
        nt_append(line, (int)sizeof line, " line(s) in the file were ignored");
        scr_text_ellipsis(TH_PAD, y + TH_LINE, line, TH_INK, w);
    }

    w_softkeys((char const *const[6]){ "", "", "Report", "", "", "About" }, -1);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "LEFT/RIGHT change   F3 hardware report   EXIT back", TH_INK);
    scr_present();
}

app_result_t app_settings_key(bench_event_t ev)
{
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("settings"));
        return APP_CONTINUE;
    }
    if (ev.key == BK_MENU)
        return APP_QUIT;
    if (ev.key == BK_EXIT)
        return APP_BACK;
    if (ev.key == BK_F6)
    {
        app_open(APP_SCREEN_ABOUT);
        return APP_CONTINUE;
    }
    if (ev.key == BK_F3)
    {
        app_open(APP_SCREEN_HWREPORT);
        return APP_CONTINUE;
    }
    if (ev.key == BK_UP && row > 0)
        row--;
    else if (ev.key == BK_DOWN && row < 4)
        row++;
    else if (ev.key == BK_LEFT)
        change(-1);
    else if (ev.key == BK_RIGHT || ev.key == BK_EXE)
        change(+1);
    return APP_CONTINUE;
}
