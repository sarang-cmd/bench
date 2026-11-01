/* app_log.c - the study log (V2 feature F13).
 *
 * Spec: the V2 master prompt, F13.  What you did, for how long, on which day,
 * read from a plain text file the student can also write by hand:
 *
 *      Study log                    10 days
 *      120 min in 10 sessions
 *      streak 4 days   today 25 min
 *      10-31  60  pacer
 *      10-31  25  cards
 *
 * Bench cannot read the calendar: gint's date fields were never checked on the
 * hardware (docs/spike.md lists only hours, minutes and seconds), so an entry
 * takes the day after the newest one in the log, and UP/DOWN moves it.  That
 * keeps a daily habit one key press and never claims a date the add-in cannot
 * know.
 *
 * Keys
 *   list   UP/DOWN scroll, F1 add, F2 delete the last entry, EXIT home
 *   add    LEFT/RIGHT pick minutes, tool or day; digits type the minutes;
 *          UP/DOWN change the tool or the day; EXE saves
 *
 * Files read:    study/log.txt
 * Files written: study/log.txt, once, when the screen is left. */

#include "app.h"
#include "app_help.h"

#include <stdio.h>
#include <string.h>

#include "core/logbook.h"
#include "platform/screen.h"
#include "platform/store.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

#define LOG_PATH "study/log.txt"
#define LOG_FILE_MAX 8192
#define LIST_ROWS 7

static char const *const TOOLS[] = {
    "pacer", "cards", "notes", "function", "triangle", "poly",
    "roots", "seq", "cplx", "numtheory", "stats", "uncert",
    "sigfig", "suvat", "moles", "base", "graph",
};
#define TOOL_COUNT ((int)(sizeof TOOLS / sizeof TOOLS[0]))

typedef enum { LG_LIST = 0, LG_ADD } screen_t;

static log_entry_t entries[LOG_MAX];
static int count;
static int skipped;
static bool dirty;
static int top;
static screen_t screen = LG_LIST;

/* The entry being typed. */
static char minutes[8];
static int tool_sel;
static int day_num;
static int field;

static void load(void)
{
    static char text[LOG_FILE_MAX];

    count = log_parse_all("", entries, LOG_MAX, &skipped);
    if (store_read(LOG_PATH, text, (int)sizeof text, NULL) == STORE_OK)
        count = log_parse_all(text, entries, LOG_MAX, &skipped);
    dirty = false;
}

static void save(void)
{
    static char text[LOG_FILE_MAX];
    int n = 0;

    for (int i = 0; i < count; i++) {
        char line[LOG_LINE_MAX + 8];
        int const len = snprintf(line, sizeof line, "%04d-%02d-%02d %d %s\n",
                                 entries[i].year, entries[i].month,
                                 entries[i].day, entries[i].minutes,
                                 entries[i].tool);

        if (len <= 0 || n + len >= (int)sizeof text) break;
        memcpy(text + n, line, (size_t)len);
        n += len;
    }
    if (n > 0)
        store_write(LOG_PATH, text, n);
    else
        store_remove(LOG_PATH);
    dirty = false;
}

void app_log_init(void)
{
    load();
    top = count > LIST_ROWS ? count - LIST_ROWS : 0;
    minutes[0] = 0;
    tool_sel = 0;
    field = 0;
    screen = LG_LIST;
    if (!log_latest(entries, count, &day_num))
        day_num = log_day_number(2026, 1, 1);
}

int app_log_timeout(void)
{
    return 0;
}

/* ---- drawing ------------------------------------------------------------ */

static void date_of(int day, int *y, int *m, int *d)
{
    /* Walk back from a known date: the log only ever needs the newest few. */
    *y = 2026;
    *m = 1;
    *d = 1;
    for (int i = 0; i < day; i++) {
        static int const LEN[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        int const leap = (*y % 4 == 0 && *y % 100 != 0) || *y % 400 == 0;

        if (++(*d) > (LEN[*m - 1] + (*m == 2 && leap ? 1 : 0))) {
            *d = 1;
            if (++(*m) > 12) {
                *m = 1;
                (*y)++;
            }
        }
    }
}

static void draw_list(void)
{
    char a[64], b[24];
    int y = TH_CONTENT_Y + 6;

    snprintf(a, sizeof a, "%d min in %d session%s", log_total(entries, count),
             count, count == 1 ? "" : "s");
    scr_text(TH_PAD, y, a, TH_BLUE);
    y += TH_LINE;

    snprintf(a, sizeof a, "days %d   longest streak %d", log_days_logged(entries, count),
             log_longest_streak(entries, count));
    scr_text(TH_PAD, y, a, TH_INK);
    y += TH_LINE + 2;

    if (!count)
    {
        scr_text(TH_PAD, y, "No sessions yet.", TH_INK);
        scr_text(TH_PAD, y + TH_LINE, "F1 adds one, or write", TH_INK);
        scr_text(TH_PAD, y + 2 * TH_LINE, "Bench/study/log.txt by hand.", TH_INK);
        return;
    }

    /* Newest first: the top of the list is what you did last. */
    for (int i = 0; i < LIST_ROWS && top + i < count; i++)
    {
        int const at = count - 1 - (top + i);

        snprintf(a, sizeof a, "%02d-%02d  %3d  %s", entries[at].month,
                 entries[at].day, entries[at].minutes, entries[at].tool);
        scr_text(TH_PAD, y + i * TH_LINE, a, TH_INK);
    }

    w_scrollbar(SCR_W - TH_PAD + 2, y, LIST_ROWS * TH_LINE - 3, count,
                LIST_ROWS, top);

    if (count > LIST_ROWS)
    {
        snprintf(a, sizeof a, "showing %d-%d of %d", top + 1,
                 top + LIST_ROWS > count ? count : top + LIST_ROWS, count);
        scr_text(SCR_W - TH_PAD - scr_text_w(a), SCR_H - TH_SOFT_H - TH_LINE - 4,
                 a, TH_INK);
    }
    if (skipped)
    {
        nt_i64(skipped, b, (int)sizeof b);
        snprintf(a, sizeof a, "%s line%s skipped", b, skipped == 1 ? "" : "s");
        scr_text(TH_PAD, SCR_H - TH_SOFT_H - TH_LINE - 4, a, TH_RED);
    }
}

static void draw_add(void)
{
    char a[64], b[16];
    int y = TH_CONTENT_Y + 8;
    int yy, mm, dd;
    char const *const names[3] = { "minutes", "tool", "day" };

    date_of(day_num, &yy, &mm, &dd);

    for (int i = 0; i < 3; i++)
    {
        char value[40];

        if (i == 0)
            snprintf(value, sizeof value, "%s", minutes[0] ? minutes : "_");
        else if (i == 1)
            snprintf(value, sizeof value, "%s", TOOLS[tool_sel]);
        else
            snprintf(value, sizeof value, "%04d-%02d-%02d", yy, mm, dd);

        scr_text(TH_PAD, y, names[i], i == field ? TH_BLUE : TH_INK);
        scr_text(TH_PAD + 80, y, value, TH_INK);
        if (i == field)
            scr_fill(TH_PAD + 80, y + TH_LINE - 2, scr_text_w(value), 2, TH_RED);
        y += TH_LINE + 4;
    }

    y += 6;
    scr_text(TH_PAD, y, "EXE saves the session.", TH_INK);
    snprintf(a, sizeof a, "%.1f h = ", 0.0);
    (void)a;
    nt_i64(log_total(entries, count), b, (int)sizeof b);
    snprintf(a, sizeof a, "log now holds %s min", b);
    scr_text(TH_PAD, y + TH_LINE, a, TH_INK);
}

void app_log_draw(void)
{
    static char hint[24];

    scr_clear(TH_PAPER);
    snprintf(hint, sizeof hint, "%d session%s", count, count == 1 ? "" : "s");
    w_title("Study log", hint);
    if (screen == LG_ADD)
        draw_add();
    else
        draw_list();

    w_softkeys((char const *const[6]){
                   screen == LG_ADD ? "Save" : "Add",
                   screen == LG_ADD ? "Back" : "Delete", "", "", "", "" },
               -1);
    scr_present();
}

/* ---- keys --------------------------------------------------------------- */

static void add_entry(void)
{
    int minutes_value = 0;

    if (!minutes[0] || count >= LOG_MAX) return;
    for (char const *p = minutes; *p; p++)
        if (*p >= '0' && *p <= '9') minutes_value = minutes_value * 10 + (*p - '0');
    if (minutes_value < 1 || minutes_value > 1440) return;

    {
        log_entry_t *e = &entries[count++];

        date_of(day_num, &e->year, &e->month, &e->day);
        e->minutes = minutes_value;
        snprintf(e->tool, sizeof e->tool, "%s", TOOLS[tool_sel]);
    }
    minutes[0] = 0;
    day_num++;
    dirty = true;
    screen = LG_LIST;
    top = count > LIST_ROWS ? count - LIST_ROWS : 0;
}

app_result_t app_log_key(bench_event_t ev)
{
    switch (ev.key)
    {
    case BK_MENU:
        if (dirty) save();
        return APP_QUIT;
    case BK_F5:
        app_help_open(tool_by_id("log"));
        return APP_CONTINUE;
    case BK_EXIT:
        if (dirty) save();
        return APP_BACK;
    case BK_F2:
        if (screen == LG_ADD)
            screen = LG_LIST;
        else if (count > 0)
        {
            count--;
            dirty = true;
            top = count > LIST_ROWS ? count - LIST_ROWS : 0;
        }
        return APP_CONTINUE;
    case BK_F1:
        if (screen == LG_LIST)
        {
            screen = LG_ADD;
            field = 0;
        }
        else
            add_entry();
        return APP_CONTINUE;
    default:
        break;
    }

    if (screen == LG_LIST)
    {
        if (ev.key == BK_UP && top > 0) top--;
        if (ev.key == BK_DOWN && top + LIST_ROWS < count) top++;
        return APP_CONTINUE;
    }

    switch (ev.key)
    {
    case BK_LEFT:
        field = (field + 2) % 3;
        break;
    case BK_RIGHT:
        field = (field + 1) % 3;
        break;
    case BK_UP:
    case BK_DOWN:
    {
        int const step = ev.key == BK_DOWN ? 1 : -1;

        if (field == 1)
            tool_sel = (tool_sel + step + TOOL_COUNT) % TOOL_COUNT;
        else if (field == 2)
            day_num += step;
        break;
    }
    case BK_DIGIT:
        if (field == 0 && strlen(minutes) < sizeof minutes - 1)
        {
            size_t const n = strlen(minutes);

            minutes[n] = ev.ch;
            minutes[n + 1] = 0;
        }
        break;
    case BK_DEL:
        if (field == 0 && minutes[0])
            minutes[strlen(minutes) - 1] = 0;
        break;
    case BK_EXE:
        add_entry();
        break;
    default:
        break;
    }
    return APP_CONTINUE;
}
