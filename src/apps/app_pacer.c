/* app_pacer.c - the Pacer tool.
 *
 * Spec: docs/plan.md section 6.1, TASK-M3.
 *
 * Screens
 *   setup  three fields: total minutes, total marks, marks per checkpoint
 *   plan   minutes per mark and the checkpoint table (scrolls, EXE starts)
 *   run    clock, marks logged, gap line, next checkpoint (redraws at 250 ms)
 *   done   total time and final gap
 *
 * Keys
 *   setup  UP/DOWN pick a field, digits and . type, DEL removes, EXE validates
 *   plan   UP/DOWN scroll, EXE start, EXIT back to setup
 *   run    F1 log a mark, F2 pause/resume, F3 finish, EXIT back to plan
 *   done   F1 restart, EXE back to the home screen
 *
 * Files: none. */

#include "app.h"

#include <string.h>

#include "core/numfmt.h"
#include "core/numparse.h"
#include "core/pacer.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum { ST_SETUP, ST_PLAN, ST_RUN, ST_DONE } stage_t;

static stage_t stage;
static pacer_setup_t setup;
static pacer_clock_t clock;
static uint32_t done_marks;
static double plan_now;        /* minutes at the moment the run started */
static int field;              /* 0 minutes, 1 marks, 2 checkpoint */
static int plan_top;
static bool paused;
static char err[64];
static char in_minutes[12] = "90";
static char in_marks[8] = "80";
static char in_check[8] = "10";

void app_pacer_init(void)
{
    char buf[8];

    stage = ST_SETUP;
    field = 0;
    plan_top = 0;
    err[0] = 0;
    paused = false;
    done_marks = 0;
    nt_u64(app_cfg()->pacer_checkpoint, buf, (int)sizeof buf);
    strncpy(in_check, buf, sizeof in_check - 1);
    in_check[sizeof in_check - 1] = 0;
    pacer_clock_init(&clock);
}

int app_pacer_timeout(void)
{
    return stage == ST_RUN && !paused ? 250 : 0;
}

/* ---- setup -------------------------------------------------------------- */

static char *active_field(void)
{
    return field == 0 ? in_minutes : field == 1 ? in_marks : in_check;
}

static int active_size(void)
{
    return field == 0 ? (int)sizeof in_minutes
                      : field == 1 ? (int)sizeof in_marks : (int)sizeof in_check;
}

static void draw_setup(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int w = SCR_W - 2 * TH_PAD;
    int fh = TH_LINE + 8;

    scr_text(TH_PAD, y, "Practice paper", TH_INK);
    y += TH_LINE + 2;
    w_field(TH_PAD, y, w, fh, "Total minutes", in_minutes, field == 0);
    y += fh + TH_GAP;
    w_field(TH_PAD, y, w, fh, "Total marks  ", in_marks, field == 1);
    y += fh + TH_GAP;
    w_field(TH_PAD, y, w, fh, "Per checkpoint", in_check, field == 2);

    y += fh + TH_GAP + 2;
    if (err[0])
        scr_text(TH_PAD, y, err, TH_INK);
    else
    {
        scr_text(TH_PAD, y, "Enter: time and marks, then EXE.", TH_INK);
        scr_text(TH_PAD, y + TH_LINE, "Checkpoints default to "
                 "10 marks.", TH_INK);
    }

    w_softkeys((char const *const[6]){ "", "", "", "", "", "" }, -1);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "EXE next   UP/DOWN field   EXIT back", TH_INK);
}

static bool validate(void)
{
    double minutes = 0.0;
    double marks = 0.0;
    double check = 0.0;

    if (numparse_cstr(in_minutes, &minutes) != NUMPARSE_OK)
    {
        strcpy(err, "Total minutes is not a number");
        return false;
    }
    if (numparse_cstr(in_marks, &marks) != NUMPARSE_OK || marks < 0.0)
    {
        strcpy(err, "Total marks is not a number");
        return false;
    }
    if (numparse_cstr(in_check, &check) != NUMPARSE_OK || check < 0.0)
    {
        strcpy(err, "Checkpoint size is not a number");
        return false;
    }
    if (marks > (double)PACER_MAX_MARKS || check > (double)PACER_MAX_MARKS)
    {
        strcpy(err, "Too many marks for this tool");
        return false;
    }

    pacer_status_t st = pacer_setup(&setup, minutes, (uint32_t)marks,
                                    (uint32_t)check);

    if (st != PACER_OK)
    {
        switch (st)
        {
        case PACER_ERR_NO_TIME: strcpy(err, "Time must be more than 0"); break;
        case PACER_ERR_NO_MARKS: strcpy(err, "Marks must be more than 0"); break;
        case PACER_ERR_CHECKPOINT:
            strcpy(err, "Checkpoint size must be 1 or more");
            break;
        default: strcpy(err, "Those numbers do not make a plan"); break;
        }
        return false;
    }
    err[0] = 0;
    return true;
}

/* ---- plan --------------------------------------------------------------- */

static void draw_plan(void)
{
    char num[24];
    char line[48];
    uint32_t count = pacer_checkpoint_count(&setup);
    int rows = w_rows_in(TH_CONTENT_H - 4 * TH_LINE - TH_PAD);
    int y = TH_CONTENT_Y + TH_PAD;

    scr_text(TH_PAD, y, "Minutes per mark", TH_INK);
    nt_fit(pacer_mpm(&setup), 4, 12, num, (int)sizeof num);
    scr_text_right(SCR_W - TH_PAD, y, num, TH_INK);
    y += TH_LINE;

    nt_u64(count, num, (int)sizeof num);
    nt_append(num, (int)sizeof num, " checkpoints");
    scr_text(TH_PAD, y, num, TH_INK);
    y += TH_LINE + 2;

    scr_hline(TH_PAD, y - 2, SCR_W - 2 * TH_PAD, TH_SHADE);

    if (plan_top > (int)count - rows)
        plan_top = (int)count - rows;
    if (plan_top < 0)
        plan_top = 0;
    for (int i = 0; i < rows && (uint32_t)(plan_top + i) < count; i++)
    {
        uint32_t k = (uint32_t)plan_top + (uint32_t)i + 1u;
        double at = pacer_checkpoint_minutes(&setup, k);
        char mark[16];
        char dur[24];

        nt_u64(k * setup.checkpoint_marks, mark, (int)sizeof mark);
        nt_duration(at, dur, (int)sizeof dur);
        nt_append(mark, (int)sizeof mark, " marks");
        w_row(TH_PAD, y + i * TH_LINE, SCR_W - 2 * TH_PAD, mark, dur, false);
    }
    w_scrollbar(SCR_W - TH_PAD - 2, y, rows * TH_LINE, (int)count, rows,
                plan_top);

    (void)line;
    w_softkeys((char const *const[6]){ "Start", "", "", "", "", "" }, -1);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "EXE start   UP/DOWN scroll   EXIT back", TH_INK);
}

/* ---- run ---------------------------------------------------------------- */

static double elapsed(void)
{
    return pacer_clock_elapsed(&clock, bench_now_minutes());
}

static void draw_run(void)
{
    double t = elapsed();
    char num[24];
    char line[64];
    pacer_gap_state_t gs;
    double gap;
    uint32_t next;
    int y = TH_CONTENT_Y + TH_PAD;
    int w = SCR_W - 2 * TH_PAD;

    /* Clock, big: the panel plus the time in the middle. */
    scr_fill(TH_PAD, y, w, 46, paused ? TH_SHADE : TH_PAPER);
    scr_frame(TH_PAD, y, w, 46, TH_INK, 2);
    nt_clock(t, num, (int)sizeof num);
    scr_text_center(TH_PAD, w, y + 46 / 2 - scr_text_h() / 2, num,
                    paused ? TH_INK : TH_RED);

    y += 46 + TH_GAP + 2;

    /* Marks done and target. */
    nt_u64(done_marks, num, (int)sizeof num);
    strcpy(line, "Marks  ");
    nt_append(line, (int)sizeof line, num);
    nt_append(line, (int)sizeof line, " / ");
    nt_u64(setup.total_marks, num, (int)sizeof num);
    nt_append(line, (int)sizeof line, num);
    scr_text(TH_PAD, y, line, TH_INK);

    nt_u64((uint64_t)(pacer_target_marks(&setup, t) + 0.5), num,
           (int)sizeof num);
    strcpy(line, "Target ");
    nt_append(line, (int)sizeof line, num);
    scr_text_right(SCR_W - TH_PAD, y, line, TH_INK);

    /* The gap line, the whole point of the tool. */
    y += TH_LINE + 2;
    gap = pacer_gap_minutes(&setup, t, done_marks);
    gs = pacer_gap_state(gap);
    scr_fill(TH_PAD, y, w, TH_LINE + 4,
             gs == PACER_GAP_BEHIND ? TH_RED
             : gs == PACER_GAP_AHEAD ? TH_TEAL : TH_SHADE);
    if (gs == PACER_GAP_ON_PACE)
        strcpy(line, "On pace");
    else
    {
        double a = gap < 0 ? -gap : gap;

        nt_duration(a, num, (int)sizeof num);
        strcpy(line, gs == PACER_GAP_BEHIND ? "Behind by " : "Ahead by ");
        nt_append(line, (int)sizeof line, num);
    }
    /* The label sits on a paper plate inside the coloured band.  Paper on
     * red is 4.46:1 and on teal 3.00:1 - both under AA for body text - so the
     * band stays as the signal and the words get ink on paper (12.3:1). */
    if (gs != PACER_GAP_ON_PACE)
        scr_fill(TH_PAD + 2, y + 2, w - 4, TH_LINE, TH_PAPER);
    scr_text(TH_PAD + 4, y + 2, line,
             gs == PACER_GAP_ON_PACE ? TH_INK : TH_INK);

    /* Next checkpoint and time left. */
    y += TH_LINE + 10;
    next = pacer_next_checkpoint(&setup, t);
    if (next > pacer_checkpoint_count(&setup))
    {
        strcpy(line, "Last checkpoint passed");
    }
    else
    {
        double at = pacer_checkpoint_minutes(&setup, next);
        nt_u64(next * setup.checkpoint_marks, num, (int)sizeof num);
        strcpy(line, "Next: ");
        nt_append(line, (int)sizeof line, num);
        nt_append(line, (int)sizeof line, " marks at ");
        nt_clock(at, num, (int)sizeof num);
        nt_append(line, (int)sizeof line, num);
    }
    scr_text(TH_PAD, y, line, TH_INK);

    nt_clock(pacer_remaining_minutes(&setup, t), num, (int)sizeof num);
    strcpy(line, "Left ");
    nt_append(line, (int)sizeof line, num);
    scr_text(TH_PAD, y + TH_LINE, line, TH_INK);

    if (paused)
        scr_text_right(SCR_W - TH_PAD, y + TH_LINE, "PAUSED", TH_INK);

    w_softkeys((char const *const[6]){ "Mark", paused ? "Resume" : "Pause",
                                       "Finish", "", "", "" }, -1);
}

/* ---- done --------------------------------------------------------------- */

static void draw_done(void)
{
    double t = elapsed();
    double gap = pacer_gap_minutes(&setup, t, done_marks);
    char num[24];
    char line[64];
    int y = TH_CONTENT_Y + TH_PAD + 4;

    nt_clock(t, num, (int)sizeof num);
    scr_text(TH_PAD, y, "Total time", TH_INK);
    scr_text_right(SCR_W - TH_PAD, y, num, TH_INK);
    y += TH_LINE + 2;

    nt_u64(done_marks, num, (int)sizeof num);
    strcpy(line, "Marks logged  ");
    nt_append(line, (int)sizeof line, num);
    scr_text(TH_PAD, y, line, TH_INK);
    y += TH_LINE;

    nt_duration(gap < 0 ? -gap : gap, num, (int)sizeof num);
    strcpy(line, gap >= 0 ? "Finished behind by " : "Finished ahead by ");
    nt_append(line, (int)sizeof line, num);
    scr_text_ellipsis(TH_PAD, y, line, TH_INK, SCR_W - 2 * TH_PAD);
    y += TH_LINE + 2;

    nt_u64(setup.total_marks, num, (int)sizeof num);
    strcpy(line, "of ");
    nt_append(line, (int)sizeof line, num);
    nt_append(line, (int)sizeof line, " marks in ");
    nt_u64((uint64_t)(setup.total_minutes + 0.5), num, (int)sizeof num);
    nt_append(line, (int)sizeof line, num);
    nt_append(line, (int)sizeof line, " min planned");
    scr_text(TH_PAD, y, line, TH_INK);

    w_softkeys((char const *const[6]){ "Restart", "", "", "", "", "" }, -1);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "F1 restart   EXE home", TH_INK);
}

/* ---- entry points ------------------------------------------------------- */

void app_pacer_draw(void)
{
    scr_clear(TH_PAPER);
    w_title("Pacer", stage == ST_RUN ? "F1 mark" : "");

    switch (stage)
    {
    case ST_SETUP: draw_setup(); break;
    case ST_PLAN: draw_plan(); break;
    case ST_RUN: draw_run(); break;
    default: draw_done(); break;
    }
    scr_present();
}

app_result_t app_pacer_key(bench_event_t ev)
{
    if (ev.key == BK_MENU)
        return APP_QUIT;
    if (ev.key == BK_EXIT)
    {
        switch (stage)
        {
        case ST_SETUP: return APP_BACK;
        case ST_PLAN: stage = ST_SETUP; break;
        case ST_RUN:
            pacer_clock_pause(&clock, bench_now_minutes());
            paused = true;
            stage = ST_PLAN;
            break;
        default: stage = ST_SETUP; break;
        }
        return APP_CONTINUE;
    }

    switch (stage)
    {
    case ST_SETUP:
        if (ev.key == BK_UP || ev.key == BK_DOWN)
        {
            field = (field + (ev.key == BK_UP ? 2 : 1)) % 3;
            err[0] = 0;
        }
        else if (ev.key == BK_DIGIT || ev.key == BK_DOT)
        {
            char *buf = active_field();
            int n = (int)strlen(buf);

            if (n < active_size() - 1)
            {
                buf[n] = ev.key == BK_DIGIT ? ev.ch : '.';
                buf[n + 1] = 0;
            }
        }
        else if (ev.key == BK_DEL)
        {
            char *buf = active_field();
            int n = (int)strlen(buf);

            if (n > 0)
                buf[n - 1] = 0;
        }
        else if (ev.key == BK_EXE)
        {
            if (field < 2)
                field++;
            else if (validate())
                stage = ST_PLAN;
        }
        break;

    case ST_PLAN:
        if (ev.key == BK_DOWN)
            plan_top++;
        else if (ev.key == BK_UP && plan_top > 0)
            plan_top--;
        else if (ev.key == BK_EXE)
        {
            pacer_clock_init(&clock);
            done_marks = 0;
            paused = false;
            plan_now = bench_now_minutes();
            pacer_clock_start(&clock, plan_now);
            stage = ST_RUN;
        }
        break;

    case ST_RUN:
        if (ev.key == BK_F1)
            done_marks++;
        else if (ev.key == BK_F2)
        {
            if (paused)
            {
                pacer_clock_resume(&clock, bench_now_minutes());
                paused = false;
            }
            else
            {
                pacer_clock_pause(&clock, bench_now_minutes());
                paused = true;
            }
        }
        else if (ev.key == BK_F3)
        {
            pacer_clock_pause(&clock, bench_now_minutes());
            stage = ST_DONE;
        }
        break;

    case ST_DONE:
        if (ev.key == BK_F1 || ev.key == BK_EXE)
            app_pacer_init();
        break;
    }

    if (stage == ST_RUN && !paused &&
        pacer_finished(&setup, elapsed()))
    {
        pacer_clock_pause(&clock, bench_now_minutes());
        stage = ST_DONE;
    }
    return APP_CONTINUE;
}
