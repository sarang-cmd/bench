/* test_pacer.c - every acceptance row and edge case of docs/plan.md 6.1. */

#include "pacer.h"
#include "test_util.h"

static void acceptance_rows(void)
{
    T_SECTION("acceptance table: T / M / mpm / checkpoint at 10 marks");

    pacer_setup_t s;
    T_INT(pacer_setup(&s, 90.0, 80, 10), PACER_OK);
    T_NEAR(pacer_mpm(&s), 1.125, 1e-12);
    T_NEAR(pacer_checkpoint_minutes(&s, 1), 11.25, 1e-12);
    T_INT(pacer_checkpoint_count(&s), 8);
    T_NEAR(pacer_checkpoint_minutes(&s, 8), 90.0, 1e-9);

    T_INT(pacer_setup(&s, 120.0, 110, 10), PACER_OK);
    T_NEAR(pacer_mpm(&s), 1.0909090909090908, 1e-12);
    T_NEAR(pacer_checkpoint_minutes(&s, 1), 10.909090909090908, 1e-9);
    /* The plan prints this checkpoint as 10.91 min. */
    T_NEAR(pacer_checkpoint_minutes(&s, 1), 10.91, 0.005);
    T_INT(pacer_checkpoint_count(&s), 11);

    /* T = 60, M = 0: error line, no next screen. */
    T_INT(pacer_setup(&s, 60.0, 0, 10), PACER_ERR_NO_MARKS);
    T_NEAR(pacer_mpm(&s), 0.0, 0.0);
    T_INT(pacer_checkpoint_count(&s), 0);
    T_FALSE(pacer_finished(&s, 0.0));
    /* Checkpoint 10 of a rejected setup cannot be produced. */
    T_TRUE(pacer_checkpoint_minutes(&s, 10) < 0.0);
}

static void validation(void)
{
    T_SECTION("validation: zero, empty, out of range");

    pacer_setup_t s;
    T_INT(pacer_setup(&s, 0.0, 80, 10), PACER_ERR_NO_TIME);
    T_INT(pacer_setup(&s, -5.0, 80, 10), PACER_ERR_NO_TIME);
    T_INT(pacer_setup(&s, 90.0, 0, 10), PACER_ERR_NO_MARKS);
    T_INT(pacer_setup(&s, 90.0, 80, 0), PACER_ERR_CHECKPOINT);
    T_INT(pacer_setup(&s, 90.0, PACER_MAX_MARKS + 1u, 10), PACER_ERR_RANGE);
    T_INT(pacer_setup(&s, 90.0, 80, PACER_MAX_MARKS + 1u), PACER_ERR_RANGE);

    /* Non-finite and negative-zero input must not slip through. */
    T_INT(pacer_setup(&s, (double)NAN, 80, 10), PACER_ERR_NO_TIME);
    T_INT(pacer_setup(&s, (double)INFINITY, 80, 10), PACER_ERR_NO_TIME);
    T_INT(pacer_setup(&s, -0.0, 80, 10), PACER_ERR_NO_TIME);

    /* A rejected setup leaves no half-filled values behind. */
    (void)pacer_setup(&s, 90.0, 80, 10);
    T_INT(pacer_setup(&s, 90.0, 0, 10), PACER_ERR_NO_MARKS);
    T_NEAR(s.total_minutes, 0.0, 0.0);
    T_UINT(s.total_marks, 0);
    T_UINT(s.checkpoint_marks, 0);

    /* The last accepted mark count is the boundary itself. */
    T_INT(pacer_setup(&s, 500.0, PACER_MAX_MARKS, 1000), PACER_OK);
    T_INT(pacer_setup(&s, 500.0, PACER_MAX_MARKS + 1u, 1000), PACER_ERR_RANGE);
}

static void marks_above_999(void)
{
    T_SECTION("marks above 999");

    pacer_setup_t s;
    T_INT(pacer_setup(&s, 600.0, 5000, 100), PACER_OK);
    T_NEAR(pacer_mpm(&s), 0.12, 1e-12);
    T_NEAR(pacer_checkpoint_minutes(&s, 1), 12.0, 1e-9);
    T_INT(pacer_checkpoint_count(&s), 50);
    T_NEAR(pacer_checkpoint_minutes(&s, 50), 600.0, 1e-9);
    T_NEAR(pacer_target_marks(&s, 300.0), 2500.0, 1e-6);
    T_INT(pacer_next_checkpoint(&s, 300.0), 26);

    /* 1000 marks is exactly the old 3-digit ceiling. */
    T_INT(pacer_setup(&s, 1000.0, 1000, 10), PACER_OK);
    T_NEAR(pacer_mpm(&s), 1.0, 1e-12);
    T_INT(pacer_checkpoint_count(&s), 100);
}

static void checkpoints(void)
{
    T_SECTION("checkpoint maths");

    pacer_setup_t s;
    (void)pacer_setup(&s, 90.0, 80, 10);

    /* k = 0 is not a checkpoint. */
    T_TRUE(pacer_checkpoint_minutes(&s, 0) < 0.0);

    /* Fewer marks than one checkpoint -> no checkpoints, and the next index is
     * past the end. */
    pacer_setup_t t;
    T_INT(pacer_setup(&t, 60.0, 5, 10), PACER_OK);
    T_INT(pacer_checkpoint_count(&t), 0);
    T_INT(pacer_next_checkpoint(&t, 0.0), 1);
    T_INT(pacer_next_checkpoint(&t, 30.0), 1);

    /* Marks that are not a multiple of the checkpoint size. */
    pacer_setup_t u;
    T_INT(pacer_setup(&u, 100.0, 95, 10), PACER_OK);
    T_INT(pacer_checkpoint_count(&u), 9);
    T_NEAR(pacer_checkpoint_minutes(&u, 9), 9.0 * 10.0 * (100.0 / 95.0), 1e-9);

    /* next_checkpoint: before the first, exactly on one, between, past the end.
     * C * mpm = 10 * 1.125 = 11.25 min. */
    T_INT(pacer_next_checkpoint(&s, 0.0), 1);
    T_INT(pacer_next_checkpoint(&s, 11.0), 1);
    T_INT(pacer_next_checkpoint(&s, 11.25), 2); /* the 11.25 checkpoint is done */
    T_INT(pacer_next_checkpoint(&s, 11.3), 2);
    T_INT(pacer_next_checkpoint(&s, 89.9), 8); /* 88.875 passed, 90.0 not yet */
    T_INT(pacer_next_checkpoint(&s, 90.0), 9); /* all eight passed */
    T_INT(pacer_next_checkpoint(&s, 500.0), 9);
}

static void gap_and_pace(void)
{
    T_SECTION("gap to pace, target marks");

    pacer_setup_t s;
    (void)pacer_setup(&s, 90.0, 80, 10); /* mpm = 1.125 */

    /* On pace: 20 minutes, 17.77 marks -> measured at a whole mark instead. */
    T_NEAR(pacer_gap_minutes(&s, 11.25, 10), 0.0, 1e-12);
    T_INT(pacer_gap_state(pacer_gap_minutes(&s, 11.25, 10)), PACER_GAP_ON_PACE);

    /* Behind: 20 minutes in, only 10 marks done. */
    T_NEAR(pacer_gap_minutes(&s, 20.0, 10), 8.75, 1e-12);
    T_INT(pacer_gap_state(8.75), PACER_GAP_BEHIND);

    /* Ahead: 10 minutes in, 12 marks done. */
    T_NEAR(pacer_gap_minutes(&s, 10.0, 12), -3.5, 1e-12);
    T_INT(pacer_gap_state(-3.5), PACER_GAP_AHEAD);

    /* Half a second either way is still on pace, beyond it is not. */
    T_INT(pacer_gap_state(0.008), PACER_GAP_ON_PACE);
    T_INT(pacer_gap_state(-0.008), PACER_GAP_ON_PACE);
    T_INT(pacer_gap_state(0.0084), PACER_GAP_BEHIND);
    T_INT(pacer_gap_state(-0.0084), PACER_GAP_AHEAD);

    T_NEAR(pacer_target_marks(&s, 0.0), 0.0, 0.0);
    T_NEAR(pacer_target_marks(&s, 11.25), 10.0, 1e-9);
    T_NEAR(pacer_target_marks(&s, 90.0), 80.0, 1e-9);

    /* No setup, no gap. */
    pacer_setup_t bad;
    (void)pacer_setup(&bad, 60.0, 0, 10);
    T_NEAR(pacer_gap_minutes(&bad, 10.0, 5), 0.0, 0.0);
    T_NEAR(pacer_target_marks(&bad, 10.0), 0.0, 0.0);
}

static void clock_end(void)
{
    T_SECTION("clock reaches T");

    pacer_setup_t s;
    (void)pacer_setup(&s, 90.0, 80, 10);

    T_FALSE(pacer_finished(&s, 0.0));
    T_FALSE(pacer_finished(&s, 89.99));
    T_TRUE(pacer_finished(&s, 90.0));
    T_TRUE(pacer_finished(&s, 91.0));

    T_NEAR(pacer_remaining_minutes(&s, 0.0), 90.0, 0.0);
    T_NEAR(pacer_remaining_minutes(&s, 30.0), 60.0, 0.0);
    T_NEAR(pacer_remaining_minutes(&s, 90.0), 0.0, 0.0);
    T_NEAR(pacer_remaining_minutes(&s, 120.0), 0.0, 0.0);

    /* The gap at the end is the final gap. */
    T_NEAR(pacer_gap_minutes(&s, 90.0, 80), 0.0, 1e-9);
    T_NEAR(pacer_gap_minutes(&s, 90.0, 70), 11.25, 1e-9);
}

static void clock_pause_resume(void)
{
    T_SECTION("pause and resume");

    pacer_clock_t c;
    pacer_clock_init(&c);
    T_FALSE(pacer_clock_running(&c));
    T_NEAR(pacer_clock_elapsed(&c, 5.0), 0.0, 0.0);

    pacer_clock_start(&c, 100.0);
    T_TRUE(pacer_clock_running(&c));
    T_NEAR(pacer_clock_elapsed(&c, 105.0), 5.0, 1e-12);

    /* Pause banks 5 minutes, then time passes with no effect. */
    pacer_clock_pause(&c, 105.0);
    T_FALSE(pacer_clock_running(&c));
    T_NEAR(pacer_clock_elapsed(&c, 130.0), 5.0, 1e-12);

    /* A second pause is a no-op. */
    pacer_clock_pause(&c, 130.0);
    T_NEAR(pacer_clock_elapsed(&c, 140.0), 5.0, 1e-12);

    /* Resume at 140, run to 145 -> 10 minutes total, paused time excluded. */
    pacer_clock_resume(&c, 140.0);
    T_TRUE(pacer_clock_running(&c));
    T_NEAR(pacer_clock_elapsed(&c, 145.0), 10.0, 1e-12);

    /* Resume while running is a no-op. */
    pacer_clock_resume(&c, 146.0);
    T_NEAR(pacer_clock_elapsed(&c, 147.0), 12.0, 1e-12);

    /* Restart resets the bank. */
    pacer_clock_start(&c, 200.0);
    T_NEAR(pacer_clock_elapsed(&c, 202.0), 2.0, 1e-12);

    /* A clock reading before the start never gives negative time. */
    T_NEAR(pacer_clock_elapsed(&c, 199.0), 0.0, 0.0);

    /* Garbage in: no crash, no change. */
    pacer_clock_start(&c, (double)NAN);
    T_TRUE(pacer_clock_running(&c));
    pacer_clock_pause(NULL, 1.0);
    pacer_clock_start(NULL, 1.0);
    T_NEAR(pacer_clock_elapsed(NULL, 1.0), 0.0, 0.0);
    T_FALSE(pacer_clock_running(NULL));

    /* Full session with pauses: 90 minutes of work measured in 120 minutes. */
    pacer_clock_init(&c);
    pacer_clock_start(&c, 0.0);
    pacer_clock_pause(&c, 20.0);   /* 20 worked */
    pacer_clock_resume(&c, 30.0);
    pacer_clock_pause(&c, 40.0);   /* +10 worked = 30 */
    pacer_clock_resume(&c, 60.0);
    T_NEAR(pacer_clock_elapsed(&c, 120.0), 90.0, 1e-12); /* +60 = 90 */

    pacer_setup_t s;
    (void)pacer_setup(&s, 90.0, 80, 10);
    T_TRUE(pacer_finished(&s, pacer_clock_elapsed(&c, 120.0)));
}

static void integration_run(void)
{
    T_SECTION("a whole paper, minute by minute");

    pacer_setup_t s;
    T_INT(pacer_setup(&s, 120.0, 110, 10), PACER_OK);

    pacer_clock_t c;
    pacer_clock_init(&c);
    pacer_clock_start(&c, 0.0);

    /* Log 10 marks every 11 minutes: mpm = 1.0909..., so keeping to 11 min per
     * 10 marks drifts slightly behind. */
    uint32_t done = 0;
    double worst = 0.0;
    for (int block = 1; block <= 11; block++) {
        double t = 11.0 * (double)block;
        done = (uint32_t)block * 10u;
        if (done > s.total_marks)
            done = s.total_marks;
        double gap = pacer_gap_minutes(&s, pacer_clock_elapsed(&c, t), done);
        if (gap > worst)
            worst = gap;
        /* next_checkpoint must never point at a checkpoint already passed. */
        uint32_t next = pacer_next_checkpoint(&s, t);
        T_TRUE(next == 12 || pacer_checkpoint_minutes(&s, next) > t);
    }
    T_TRUE(worst > 0.0);       /* 11 min per 10 marks is slower than 10.909 */
    /* The drift is 0.0909 min per checkpoint, so 11 checkpoints is ~1.0 min. */
    T_NEAR(worst, 11.0 * (11.0 - 10.0 * (120.0 / 110.0)) - 0.0, 0.02);
    T_TRUE(worst < 1.05);
    T_TRUE(pacer_finished(&s, pacer_clock_elapsed(&c, 120.0)));
    T_NEAR(pacer_remaining_minutes(&s, 120.0), 0.0, 0.0);
}

static void run_tests(void)
{
    acceptance_rows();
    validation();
    marks_above_999();
    checkpoints();
    gap_and_pace();
    clock_end();
    clock_pause_resume();
    integration_run();
}

T_MAIN("test_pacer")
