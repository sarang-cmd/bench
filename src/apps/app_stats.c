/* app_stats.c - statistics with working (V2 feature F7).
 *
 * Spec: the V2 master prompt, F7.  Type the numbers, read the summary, then
 * look at the working for the standard deviation.
 *
 *      Statistics                    median method
 *      x = 7_
 *      n 8   mean 5.000   range 7
 *      median 4.500   mode 4 (x3)
 *      sd 2.000 pop / 2.138 sample
 *      Q1 4.000  Q3 6.000  IQR 2.000
 *      fences 1.000 .. 9.000, 0 out
 *      2 4 4 4 5 5 7 9
 *
 * Keys
 *   digits . -  type a value into the entry field
 *   EXE         add it to the list (or run the regression in pairs mode)
 *   DEL         remove the last character, or the last value with the field empty
 *   F1 / F2     clear the list / single list and pairs
 *   F3          the two quartile methods
 *   F4          the working for the standard deviation
 *   F5 / EXIT   help / home
 *
 * Nothing is written to the card. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/numparse.h"
#include "core/stats.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum { ST_LIST = 0, ST_PAIRS, ST_WORK, ST_COUNT } stage_t;

static double values[STATS_MAX];   /* in pairs mode: x, y, x, y ... */
static int count;
static char entry[16];
static stats_quart_t quart = STATS_QUART_MEDIAN;
static stage_t stage = ST_LIST;

static int pair_count(void)
{
    return count / 2;
}

void app_stats_init(void)
{
    count = 0;
    entry[0] = 0;
    quart = STATS_QUART_MEDIAN;
    stage = ST_LIST;

    /* The screen is worth more with something in it than with nothing. */
    {
        double const seed[] = { 2, 4, 4, 4, 5, 5, 7, 9 };

        for (int i = 0; i < 8; i++)
            values[count++] = seed[i];
    }
}

int app_stats_timeout(void)
{
    return 0;
}

static char const *quart_name(void)
{
    return quart == STATS_QUART_MEDIAN ? "median method" : "spreadsheet method";
}

/* ---- drawing ------------------------------------------------------------ */

/* A box plot across the width of the panel: whiskers at the fences clamped to
 * the data, the box from Q1 to Q3, a line at the median, dots for outliers. */
static void draw_box_plot(stats_summary_t const *s, int x, int y, int w)
{
    double lo = s->fence_low < s->min ? s->min : s->fence_low;
    double hi = s->fence_high > s->max ? s->max : s->fence_high;
    int const mid = y + 12;

    if (hi <= lo)
        return;

    /* The axis, then the box. Both are ink on paper, so a photo still reads. */
    scr_fill(x, mid, w, 1, TH_SHADE);
    {
        int const q1 = x + (int)((s->q1 - lo) / (hi - lo) * (w - 1));
        int const q3 = x + (int)((s->q3 - lo) / (hi - lo) * (w - 1));
        int const md = x + (int)((s->median - lo) / (hi - lo) * (w - 1));

        if (q3 > q1)
        {
            scr_fill(q1, mid - 7, q3 - q1 + 1, 15, TH_YELLOW);
            scr_frame(q1, mid - 7, q3 - q1 + 1, 15, TH_INK, 1);
        }
        scr_fill(md, mid - 7, 2, 15, TH_INK);
    }
}

static void fmt_num(double v, char *buf, int size)
{
    nt_fit(v, 3, 12, buf, size);
}

static void draw_list(void)
{
    stats_summary_t s;
    char a[48], b[32];
    int y = 22;
    bool ok = stats_summary(values, count, quart, &s);

    scr_text(TH_PAD, y, "x =", TH_INK);
    scr_text(TH_PAD + 24, y, entry, TH_INK);
    scr_fill(TH_PAD + 24 + scr_text_w(entry) + 1, y + 1, 1, scr_text_h() - 2,
             TH_INK);
    y += TH_LINE + 4;

    if (!ok)
    {
        scr_text(TH_PAD, y, "Type a value, then EXE to add it.", TH_INK);
        y += TH_LINE;
        scr_text(TH_PAD, y, "F1 clears, F2 swaps list and pairs.", TH_INK);
        return;
    }

    strcpy(a, "n ");
    nt_i64(s.n, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, "   mean ");
    fmt_num(s.mean, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, "   range ");
    fmt_num(s.range, b, (int)sizeof b);
    strcat(a, b);
    scr_text(TH_PAD, y, a, TH_INK);
    y += TH_LINE;

    strcpy(a, "median ");
    fmt_num(s.median, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, "   mode ");
    fmt_num(s.mode, b, (int)sizeof b);
    strcat(a, b);
    if (s.mode_unique)
    {
        strcat(a, " x");
        nt_i64(s.mode_count, b, (int)sizeof b);
        strcat(a, b);
    }
    else
        strcat(a, " (tie)");
    scr_text(TH_PAD, y, a, TH_INK);
    y += TH_LINE;

    strcpy(a, "sd ");
    fmt_num(s.sd_pop, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, " pop / ");
    fmt_num(s.sd_sample, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, " sample");
    scr_text(TH_PAD, y, a, TH_INK);
    y += TH_LINE;

    strcpy(a, "Q1 ");
    fmt_num(s.q1, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, "  Q3 ");
    fmt_num(s.q3, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, "  IQR ");
    fmt_num(s.iqr, b, (int)sizeof b);
    strcat(a, b);
    scr_text(TH_PAD, y, a, TH_INK);
    y += TH_LINE;

    strcpy(a, "fences ");
    fmt_num(s.fence_low, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, " .. ");
    fmt_num(s.fence_high, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, "  outliers ");
    nt_i64(s.outliers, b, (int)sizeof b);
    strcat(a, b);
    scr_text(TH_PAD, y, a, s.outliers ? TH_RED : TH_INK);
    y += TH_LINE;

    /* The values themselves, wrapped, so a typo is visible before the summary
     * is trusted. */
    {
        char row[64] = "values";
        int shown = 0;

        for (int i = 0; i < count && shown < 12; i++)
        {
            fmt_num(values[i], b, (int)sizeof b);
            strcat(row, " ");
            strcat(row, b);
            shown++;
        }
        if (count > shown)
            strcat(row, " ...");
        scr_text(TH_PAD, y, row, TH_INK);
    }

    draw_box_plot(&s, TH_PAD, y + TH_LINE + 4, SCR_W - 2 * TH_PAD);
}

static void draw_pairs(void)
{
    static double xs[STATS_MAX / 2];
    static double ys[STATS_MAX / 2];
    stats_line_t line;
    char a[48], b[32];
    int y = 22;
    int const n = pair_count();
    bool ok;

    for (int i = 0; i < n; i++)
    {
        xs[i] = values[2 * i];
        ys[i] = values[2 * i + 1];
    }
    ok = n >= 2 && stats_regression(xs, ys, n, &line);


    scr_text(TH_PAD, y, "pairs, x then y", TH_INK);
    y += TH_LINE;

    if (!ok)
    {
        scr_text(TH_PAD, y, "Add at least two points: x, then y.", TH_INK);
        return;
    }

    strcpy(a, "y = ");
    fmt_num(line.intercept, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, " + ");
    fmt_num(line.slope, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, "x");
    scr_text(TH_PAD, y, a, TH_INK);
    y += TH_LINE;

    strcpy(a, "r ");
    fmt_num(line.r, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, "   r2 ");
    fmt_num(line.r2, b, (int)sizeof b);
    strcat(a, b);
    scr_text(TH_PAD, y, a, TH_INK);
    y += TH_LINE;

    strcpy(a, "mean x ");
    fmt_num(line.mean_x, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, "   mean y ");
    fmt_num(line.mean_y, b, (int)sizeof b);
    strcat(a, b);
    scr_text(TH_PAD, y, a, TH_INK);
}

static void draw_work(void)
{
    stats_summary_t s;
    char a[52], b[24];
    int y = 22;

    if (!stats_summary(values, count, quart, &s))
    {
        scr_text(TH_PAD, y, "Nothing to work out yet.", TH_INK);
        return;
    }

    scr_text(TH_PAD, y, "standard deviation, step by step", TH_INK);
    y += TH_LINE;

    strcpy(a, "mean = sum / n = ");
    fmt_num(s.sum, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, " / ");
    nt_i64(s.n, b, (int)sizeof b);
    strcat(a, b);
    scr_text(TH_PAD, y, a, TH_INK);
    y += TH_LINE;

    /* The three numbers a student writes down, in order. */
    strcpy(a, "sum of squares = ");
    fmt_num(s.var_pop * (double)s.n, b, (int)sizeof b);
    strcat(a, b);
    scr_text(TH_PAD, y, a, TH_INK);
    y += TH_LINE;

    strcpy(a, "variance pop = ");
    fmt_num(s.var_pop, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, "  sd ");
    fmt_num(s.sd_pop, b, (int)sizeof b);
    strcat(a, b);
    scr_text(TH_PAD, y, a, TH_INK);
    y += TH_LINE;

    if (s.n > 1)
    {
        strcpy(a, "variance sample = ");
        fmt_num(s.var_sample, b, (int)sizeof b);
        strcat(a, b);
        strcat(a, "  sd ");
        fmt_num(s.sd_sample, b, (int)sizeof b);
        strcat(a, b);
        scr_text(TH_PAD, y, a, TH_INK);
        y += TH_LINE;
    }

    strcpy(a, "IQR = Q3 - Q1 = ");
    fmt_num(s.q3, b, (int)sizeof b);
    strcat(a, b);
    strcat(a, " - ");
    fmt_num(s.q1, b, (int)sizeof b);
    strcat(a, b);
    scr_text(TH_PAD, y, a, TH_INK);
}

void app_stats_draw(void)
{
    scr_clear(TH_PAPER);

    switch (stage)
    {
    case ST_PAIRS: w_title("Statistics", "pairs"); break;
    case ST_WORK: w_title("Statistics", "working"); break;
    default: w_title("Statistics", quart_name()); break;
    }

    switch (stage)
    {
    case ST_PAIRS: draw_pairs(); break;
    case ST_WORK: draw_work(); break;
    default: draw_list(); break;
    }

    w_softkeys(stage == ST_LIST
                   ? (char const *const[6]){ "Clear", "Pairs", "Quart", "Work", "", "" }
                   : (char const *const[6]){ "Clear", "List", "", "", "", "" },
               -1);
    scr_present();
}

/* ---- keys --------------------------------------------------------------- */

static void add_entry(void)
{
    double v;

    if (!entry[0] || numparse_cstr(entry, &v) != NUMPARSE_OK)
        return;
    if (count >= STATS_MAX)
        return;
    values[count++] = v;
    entry[0] = 0;
}

app_result_t app_stats_key(bench_event_t ev)
{
    if (ev.key == BK_MENU)
        return APP_QUIT;
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("stats"));
        return APP_CONTINUE;
    }

    switch (ev.key)
    {
    case BK_EXE:
        if (stage == ST_LIST)
            add_entry();
        else if (count % 2 == 0)
            stage = ST_LIST;   /* an odd number of values cannot be pairs */
        break;
    case BK_DEL:
        if (entry[0])
            entry[strlen(entry) - 1] = 0;
        else if (count > 0)
            count--;
        break;
    case BK_MINUS:
        if (entry[0] == '-')
            strcpy(entry, entry + 1);
        else
        {
            memmove(entry + 1, entry, strlen(entry) + 1);
            entry[0] = '-';
        }
        break;
    case BK_DOT:
    case BK_DIGIT:
        if (strlen(entry) < sizeof entry - 2 && count < STATS_MAX)
        {
            size_t const n = strlen(entry);

            entry[n] = ev.key == BK_DIGIT ? ev.ch : '.';
            entry[n + 1] = 0;
        }
        break;
    case BK_F1:
        count = 0;
        entry[0] = 0;
        stage = ST_LIST;
        break;
    case BK_F2:
        stage = stage == ST_PAIRS ? ST_LIST : ST_PAIRS;
        break;
    case BK_F3:
        quart = quart == STATS_QUART_MEDIAN ? STATS_QUART_LINEAR
                                            : STATS_QUART_MEDIAN;
        break;
    case BK_F4:
        stage = stage == ST_WORK ? ST_LIST : ST_WORK;
        break;
    case BK_EXIT:
        return APP_BACK;
    default:
        break;
    }
    return APP_CONTINUE;
}
