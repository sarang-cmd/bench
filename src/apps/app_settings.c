/* app_settings.c - the settings screen and the file it lives in.
 *
 * Spec: TASK-M2 ("Settings file: load at start, save on change only") and
 * docs/plan.md section 6.2 for the uncertainty setting.
 *
 * Keys: UP/DOWN pick a row, LEFT/RIGHT change it, EXE also changes it,
 * F2 Card check (the folders Bench reads), F3 Hardware report, F5 Help,
 * F6 About, EXIT back.  The file is written the moment a value changes, never
 * on a timer and never inside a loop. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/settings.h"
#include "registry.h"
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

#define SETTINGS_ROWS 9

/* The row order, in one place: draw_row() and change() both switch on these,
 * so a new row cannot end up drawn in one position and edited in another. */
enum {
    ROW_UNCERT = 0,
    ROW_FONT,
    ROW_CHECKPOINT,
    ROW_WRAP,
    ROW_ANGLE,
    ROW_HINT,
    ROW_STARTUP,
    ROW_HINT_LINE,
    ROW_WARN,
};

static void change(int delta)
{
    bench_settings_t *cfg = app_cfg();

    switch (row)
    {
    case ROW_UNCERT:
        cfg->uncert_sig = settings_clamp_uncert_sig(cfg->uncert_sig + delta);
        break;
    case ROW_FONT:
        cfg->font_scale = settings_clamp_font(cfg->font_scale + delta);
        /* Take effect at once: the reader can see the result on this screen. */
        scr_set_text_scale((int)cfg->font_scale);
        break;
    case ROW_CHECKPOINT:
        cfg->pacer_checkpoint =
            settings_clamp_checkpoint(cfg->pacer_checkpoint + delta);
        break;
    case ROW_WRAP:
    {
        int v = cfg->notes_wrap + (delta > 0 ? 25 : -25);

        cfg->notes_wrap = settings_clamp_wrap(v);
        if (v > 0 && cfg->notes_wrap == 0)
            cfg->notes_wrap = 50;   /* never land on "auto" by accident */
        break;
    }
    case ROW_ANGLE:
        cfg->angle_unit = settings_clamp_angle(
            cfg->angle_unit == SETTINGS_ANGLE_RAD ? SETTINGS_ANGLE_DEG
                                                  : SETTINGS_ANGLE_RAD);
        break;
    case ROW_HINT:
        cfg->tour_done = settings_clamp_flag(cfg->tour_done + delta);
        break;
    case ROW_STARTUP:
        cfg->startup_cat = settings_clamp_startup_cat(
            delta > 0 ? (cfg->startup_cat + 1) % SETTINGS_CATEGORY_COUNT
                      : (cfg->startup_cat + SETTINGS_CATEGORY_COUNT - 1) %
                            SETTINGS_CATEGORY_COUNT);
        break;
    case ROW_HINT_LINE:
        cfg->show_hint = settings_clamp_flag(cfg->show_hint ? 0 : 1);
        break;
    case ROW_WARN:
        cfg->pacer_warn = settings_clamp_warn(
            delta > 0 ? (cfg->pacer_warn + 1) % (SETTINGS_WARN_MAX + 1)
                      : (cfg->pacer_warn + SETTINGS_WARN_MAX) %
                            (SETTINGS_WARN_MAX + 1));
        break;
    default:
        break;
    }
    app_cfg_save();
}

/* Rows from the first row down to the key legend at the bottom. */
static int list_top(void)
{
    return TH_CONTENT_Y + TH_PAD + TH_LINE + 4;
}

static int list_space(void)
{
    return SCR_H - TH_SOFT_H - TH_LINE - 2 - 4 - list_top();
}

/* Two status lines ("1 figure", "Saved to ...") sit under the list when there
 * is room for them; at Large text there is not, and the list gets the space. */
static bool status_fits(void)
{
    return list_space() - 2 * TH_LINE >= 4 * TH_LINE;
}

/* How many rows fit.  The list scrolls rather than overlapping the status lines
 * or the key legend, which is what a larger text size makes necessary. */
static int visible_rows(void)
{
    int space = list_space();
    int rows;

    if (status_fits())
        space -= 2 * TH_LINE;
    rows = space / TH_LINE;
    if (rows > SETTINGS_ROWS)
        rows = SETTINGS_ROWS;
    return rows < 1 ? 1 : rows;
}

/* First row shown, so the selected row is always on screen. */
static int first_row(void)
{
    int n = visible_rows();

    if (row < n)
        return 0;
    return row - n + 1;
}

/* One row, by index.  Kept in one place so the scrolling window and the key
 * handler cannot disagree about which row is which. */
static void draw_row(int index, int y, int w, bench_settings_t *cfg, char *num)
{
    bool sel = index == row;

    switch (index)
    {
    case ROW_UNCERT:
        w_row(TH_PAD, y, w, "Uncertainty figures",
              cfg->uncert_sig == 2 ? "1 or 2" : "1", sel);
        break;
    case ROW_FONT:
        w_row(TH_PAD, y, w, "Font", settings_font_text(cfg->font_scale), sel);
        break;
    case ROW_CHECKPOINT:
        nt_u64(cfg->pacer_checkpoint, num, (int)sizeof num);
        w_row(TH_PAD, y, w, "Pacer checkpoint", num, sel);
        break;
    case ROW_WRAP:
        if (cfg->notes_wrap == 0)
            strcpy(num, "auto");
        else
        {
            nt_u64(cfg->notes_wrap, num, (int)sizeof num);
            nt_append(num, (int)sizeof num, "%");
        }
        w_row(TH_PAD, y, w, "Note width", num, sel);
        break;
    case ROW_ANGLE:
        w_row(TH_PAD, y, w, "Angles default",
              settings_angle_text(cfg->angle_unit), sel);
        break;
    case ROW_HINT:
        w_row(TH_PAD, y, w, "First-run hint",
              cfg->tour_done ? "read" : "showing", sel);
        break;
    case ROW_STARTUP:
        w_row(TH_PAD, y, w, "Opens on",
              tool_category_name((tool_category_t)cfg->startup_cat), sel);
        break;
    case ROW_HINT_LINE:
        w_row(TH_PAD, y, w, "Show hint line", cfg->show_hint ? "yes" : "no",
              sel);
        break;
    case ROW_WARN:
        w_row(TH_PAD, y, w, "Pacer clock red at",
              settings_warn_text(cfg->pacer_warn, num, (int)sizeof num), sel);
        break;
    default:
        break;
    }
}

void app_settings_draw(void)
{
    int y;
    int w = SCR_W - 2 * TH_PAD;
    bench_settings_t *cfg = app_cfg();
    char num[8];
    int first = first_row();
    int shown = visible_rows();

    scr_clear(TH_PAPER);
    w_title("Settings", "F6 About");
    y = TH_CONTENT_Y + TH_PAD;
    y += TH_LINE + 4;

    for (int i = first; i < SETTINGS_ROWS && i < first + shown; i++)
    {
        draw_row(i, y, w, cfg, num);
        y += TH_LINE;
    }
    if (shown < SETTINGS_ROWS)
        w_scrollbar(SCR_W - 4, list_top(), shown * TH_LINE, SETTINGS_ROWS,
                    shown, first);

    if (!status_fits())
        goto legend;
    y += 4;
    scr_text(TH_PAD, y, settings_uncert_sig_text(cfg->uncert_sig), TH_INK);
    y += TH_LINE;
    scr_text_ellipsis(TH_PAD, y,
                      saved_ok ? "Saved to state/settings.cfg"
                               : "Could not write the settings file",
                      TH_INK, w);
    if (ignored_lines > 0)
    {
        char line[48];

        nt_u64((uint64_t)ignored_lines, num, (int)sizeof num);
        strcpy(line, num);
        nt_append(line, (int)sizeof line, " line(s) in the file were ignored");
        scr_text_ellipsis(TH_PAD, y + TH_LINE, line, TH_INK, w);
    }

legend:
    w_softkeys((char const *const[6]){ "", "Card", "Report", "", "", "About" }, -1);
    /* Short, because the softkey strip already names Report, Help and About. */
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "UP/DOWN pick   LEFT/RIGHT change", TH_INK);
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
    if (ev.key == BK_F2)
    {
        app_open(APP_SCREEN_CARDCHECK);
        return APP_CONTINUE;
    }
    if (ev.key == BK_F3)
    {
        app_open(APP_SCREEN_HWREPORT);
        return APP_CONTINUE;
    }
    if (ev.key == BK_UP && row > 0)
        row--;
    else if (ev.key == BK_DOWN && row < SETTINGS_ROWS - 1)
        row++;
    else if (ev.key == BK_LEFT)
        change(-1);
    else if (ev.key == BK_RIGHT || ev.key == BK_EXE)
        change(+1);
    return APP_CONTINUE;
}
