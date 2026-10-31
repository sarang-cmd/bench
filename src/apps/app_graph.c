/* app_graph.c - a lab graph with error bars (V2 feature F8).
 *
 * Spec: the V2 master prompt, F8.  Type measured pairs with their uncertainty,
 * then look at the graph: the points, a bar up and down through each one, and
 * the line of best fit with its gradient quoted to an uncertainty.
 *
 *      Lab graph                   5 points
 *      x _   y _   err _
 *      1 0.50 +- 0.10
 *      2 1.00 +- 0.10
 *      gradient 0.500 +- 0.000
 *
 * Keys
 *   digits . -  type into the x, y or err field
 *   LEFT/RIGHT  pick a field
 *   EXE         add the point
 *   DEL         a digit back; on an empty field, drop the last point
 *   F1          clear, F2 the graph, F3 the example data
 *   F5 / EXIT   help / home
 *
 * Nothing is written to the card. */

#include "app.h"
#include "app_help.h"

#include <stdio.h>
#include <string.h>

#include "core/graph.h"
#include "core/numparse.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

#define LIST_ROWS 6

typedef enum { GS_LIST = 0, GS_PLOT, GS_STAGES } stage_t;

static graph_point_t points[GRAPH_MAX_POINTS];
static int count;
static char entry[3][12];
static int field;
static stage_t stage = GS_LIST;

/* The example: a spring, x in cm against y in N, with a tenth of a newton on
 * each reading. */
static void load_example(void)
{
    static graph_point_t const EXAMPLE[] = {
        { 1, 0.52, 0.10 }, { 2, 0.98, 0.10 }, { 3, 1.51, 0.10 },
        { 4, 2.03, 0.10 }, { 5, 2.47, 0.10 },
    };

    count = (int)(sizeof EXAMPLE / sizeof EXAMPLE[0]);
    for (int i = 0; i < count; i++)
        points[i] = EXAMPLE[i];
}

void app_graph_init(void)
{
    count = 0;
    entry[0][0] = entry[1][0] = entry[2][0] = 0;
    field = 0;
    stage = GS_LIST;
    load_example();
}

int app_graph_timeout(void)
{
    return 0;
}

/* ---- drawing ------------------------------------------------------------ */

static void fmt(double v, char *buf, int size)
{
    nt_fit(v, 3, 9, buf, size);
}

static void draw_entry_row(int y)
{
    static char const *names[3] = { "x", "y", "err" };
    int x = TH_PAD;

    for (int i = 0; i < 3; i++)
    {
        int const w = scr_text_w(entry[i][0] ? entry[i] : "_");

        scr_text(x, y, names[i], TH_INK);
        scr_text(x + 26, y, entry[i][0] ? entry[i] : "_", TH_INK);
        if (field == i)
            scr_fill(x + 26, y + TH_LINE - 2, w, 2, TH_RED);
        x += 26 + (i == 2 ? 0 : w + 22);
    }
}

static void draw_list(void)
{
    char a[64], b[24];
    graph_fit_t fit;
    int y = TH_CONTENT_Y + 6;
    int const first = count > LIST_ROWS ? count - LIST_ROWS : 0;

    w_title("Lab graph", "points");
    draw_entry_row(y);
    y += TH_LINE + 4;

    if (count == 0)
    {
        scr_text(TH_PAD, y, "Type x, y and the error, then EXE.", TH_INK);
        return;
    }

    for (int i = first; i < count; i++)
    {
        fmt(points[i].x, b, (int)sizeof b);
        snprintf(a, sizeof a, "%s  ", b);
        fmt(points[i].y, b, (int)sizeof b);
        strncat(a, b, sizeof a - strlen(a) - 1);
        strncat(a, " +- ", sizeof a - strlen(a) - 1);
        fmt(points[i].err, b, (int)sizeof b);
        strncat(a, b, sizeof a - strlen(a) - 1);
        scr_text(TH_PAD, y + (i - first) * TH_LINE, a, TH_INK);
    }
    y += (count - first) * TH_LINE + 4;

    if (!graph_fit(points, count, &fit))
    {
        scr_text(TH_PAD, y, "Two points on different x, please.", TH_INK);
        return;
    }

    fmt(fit.slope, b, (int)sizeof b);
    snprintf(a, sizeof a, "gradient %s", b);
    if (fit.errors_known)
    {
        strncat(a, " +- ", sizeof a - strlen(a) - 1);
        fmt(fit.slope_err, b, (int)sizeof b);
        strncat(a, b, sizeof a - strlen(a) - 1);
    }
    scr_text(TH_PAD, y, a, TH_BLUE);

    fmt(fit.intercept, b, (int)sizeof b);
    snprintf(a, sizeof a, "intercept %s", b);
    scr_text(TH_PAD, y + TH_LINE, a, TH_INK);

    fmt(fit.r2, b, (int)sizeof b);
    snprintf(a, sizeof a, "r^2 %s   n %d", b, fit.n);
    scr_text(TH_PAD, y + 2 * TH_LINE, a, TH_INK);
}

/* One axis: a line with a tick at each step and the two ends labelled. */
static void draw_axis(int x, int y, int w, int h, double lo, double hi,
                      double step, bool vertical)
{
    char b[24];

    if (vertical)
    {
        scr_fill(x, y, 1, h, TH_INK);
        for (double v = lo; v <= hi + step * 0.001; v += step)
        {
            int const py = y + h - graph_project(v, lo, hi, h);

            scr_fill(x - 3, py, 4, 1, TH_INK);
        }
        fmt(lo, b, (int)sizeof b);
        scr_text(x - 5 - scr_text_w(b), y + h - scr_text_h(), b, TH_INK);
        fmt(hi, b, (int)sizeof b);
        scr_text(x - 5 - scr_text_w(b), y, b, TH_INK);
    }
    else
    {
        /* The x axis is the bottom of the box, not the top: the y axis runs
         * upwards from it. */
        int const base = y + h;

        scr_fill(x, base, w, 1, TH_INK);
        for (double v = lo; v <= hi + step * 0.001; v += step)
        {
            int const px = x + graph_project(v, lo, hi, w);

            scr_fill(px, base - 3, 1, 3, TH_INK);
        }
        fmt(lo, b, (int)sizeof b);
        scr_text(x, base + 4, b, TH_INK);
        fmt(hi, b, (int)sizeof b);
        scr_text(x + w - scr_text_w(b), base + 4, b, TH_INK);
    }
}

static void draw_plot(void)
{
    graph_fit_t fit;
    int const plot_x = 40, plot_y = TH_CONTENT_Y + 10;
    int const plot_w = SCR_W - plot_x - 16;
    int const plot_h = SCR_H - TH_SOFT_H - 34 - plot_y;
    double xlo, xhi, xstep, ylo, yhi, ystep;

    w_title("Lab graph", "with error bars");

    if (count < 2)
    {
        scr_text(TH_PAD, plot_y, "Two points make a line.", TH_INK);
        return;
    }

    /* The axes hold the bars, not just the centres: a bar that runs off the
     * top of the graph is a lie about the measurement. */
    {
        double xmin = points[0].x, xmax = points[0].x;
        double ymin = graph_bar_low(&points[0]), ymax = graph_bar_high(&points[0]);

        for (int i = 1; i < count; i++)
        {
            if (points[i].x < xmin) xmin = points[i].x;
            if (points[i].x > xmax) xmax = points[i].x;
            if (graph_bar_low(&points[i]) < ymin) ymin = graph_bar_low(&points[i]);
            if (graph_bar_high(&points[i]) > ymax) ymax = graph_bar_high(&points[i]);
        }
        graph_range(xmin, xmax, 6, &xlo, &xhi, &xstep);
        graph_range(ymin, ymax, 5, &ylo, &yhi, &ystep);
    }

    draw_axis(plot_x, plot_y, plot_w, plot_h, xlo, xhi, xstep, false);
    draw_axis(plot_x, plot_y, plot_w, plot_h, ylo, yhi, ystep, true);

    /* The fit first, so the points sit on top of it and stay visible. */
    if (graph_fit(points, count, &fit))
    {
        int const x1 = plot_x + graph_project(xlo, xlo, xhi, plot_w);
        int const x2 = plot_x + graph_project(xhi, xlo, xhi, plot_w);
        int const y1 = plot_y + plot_h - graph_project(
                           fit.intercept + fit.slope * xlo, ylo, yhi, plot_h);
        int const y2 = plot_y + plot_h - graph_project(
                           fit.intercept + fit.slope * xhi, ylo, yhi, plot_h);

        scr_line(x1, y1, x2, y2, TH_BLUE);
    }

    for (int i = 0; i < count; i++)
    {
        int const px = plot_x + graph_project(points[i].x, xlo, xhi, plot_w);
        int const py = plot_y + plot_h - graph_project(points[i].y, ylo, yhi, plot_h);
        int const bar_lo = plot_y + plot_h - graph_project(
                               graph_bar_low(&points[i]), ylo, yhi, plot_h);
        int const bar_hi = plot_y + plot_h - graph_project(
                               graph_bar_high(&points[i]), ylo, yhi, plot_h);

        if (bar_hi < bar_lo)
            scr_fill(px, bar_hi, 1, bar_lo - bar_hi, TH_INK);
        scr_fill(px - 2, py - 2, 5, 5, TH_RED);
        scr_frame(px - 2, py - 2, 5, 5, TH_INK, 1);
    }

    scr_text(TH_PAD, plot_y - 2, "y", TH_INK);
    scr_text(TH_PAD, SCR_H - TH_SOFT_H - 14, "x", TH_INK);
}

void app_graph_draw(void)
{
    scr_clear(TH_PAPER);
    if (stage == GS_PLOT)
        draw_plot();
    else
        draw_list();

    w_softkeys((char const *const[6]){
                   "Clear", stage == GS_PLOT ? "List" : "Graph", "Example",
                   "", "", "" },
               -1);
    scr_present();
}

/* ---- keys --------------------------------------------------------------- */

static void add_point(void)
{
    double x, y, err = 0;

    if (count >= GRAPH_MAX_POINTS) return;
    if (!entry[0][0] || !entry[1][0]) return;
    if (numparse_cstr(entry[0], &x) != NUMPARSE_OK) return;
    if (numparse_cstr(entry[1], &y) != NUMPARSE_OK) return;
    if (entry[2][0] && numparse_cstr(entry[2], &err) != NUMPARSE_OK) return;

    points[count].x = x;
    points[count].y = y;
    points[count].err = err < 0 ? -err : err;
    count++;
    entry[0][0] = entry[1][0] = entry[2][0] = 0;
    field = 0;
}

app_result_t app_graph_key(bench_event_t ev)
{
    char *buf = entry[field];

    switch (ev.key)
    {
    case BK_MENU:
        return APP_QUIT;
    case BK_F5:
        app_help_open(tool_by_id("graph"));
        return APP_CONTINUE;
    case BK_DIGIT:
    case BK_DOT:
        if (strlen(buf) < sizeof entry[0] - 1)
        {
            size_t const n = strlen(buf);

            buf[n] = ev.key == BK_DIGIT ? ev.ch : '.';
            buf[n + 1] = 0;
        }
        break;
    case BK_MINUS:
        if (buf[0] == '-')
            memmove(buf, buf + 1, strlen(buf));
        else if (strlen(buf) < sizeof entry[0] - 1)
        {
            memmove(buf + 1, buf, strlen(buf) + 1);
            buf[0] = '-';
        }
        break;
    case BK_LEFT:
        field = (field + 2) % 3;
        break;
    case BK_RIGHT:
        field = (field + 1) % 3;
        break;
    case BK_UP:
    case BK_DOWN:
        field = (field + 1) % 3;
        break;
    case BK_EXE:
        add_point();
        break;
    case BK_DEL:
        if (buf[0])
            buf[strlen(buf) - 1] = 0;
        else if (count > 0)
            count--;
        break;
    case BK_F1:
        count = 0;
        entry[0][0] = entry[1][0] = entry[2][0] = 0;
        field = 0;
        break;
    case BK_F2:
        stage = stage == GS_PLOT ? GS_LIST : GS_PLOT;
        break;
    case BK_F3:
        load_example();
        entry[0][0] = entry[1][0] = entry[2][0] = 0;
        field = 0;
        break;
    case BK_EXIT:
        return APP_BACK;
    default:
        break;
    }
    return APP_CONTINUE;
}
