/* input.c - keys and the monotonic clock.  See input.h for the gint facts. */

#include "input.h"

#include <gint/keyboard.h>
#include <gint/keycodes.h>
#include <gint/rtc.h>
#include <gint/timer.h>

/* ---- clock -------------------------------------------------------------- */

static bool clock_ready = false;
static uint32_t clock_last_tick = 0;
static uint64_t clock_ticks = 0;      /* 128 Hz ticks since the call that started it */

static void clock_poll(void)
{
    uint32_t now = rtc_ticks();

    if (!clock_ready)
    {
        clock_last_tick = now;
        clock_ready = true;
        return;
    }
    /* Unsigned difference: correct across the midnight wrap and across the
     * 32-bit wrap of the tick counter itself. */
    clock_ticks += (uint32_t)(now - clock_last_tick);
    clock_last_tick = now;
}

double bench_now_minutes(void)
{
    clock_poll();
    return (double)clock_ticks / (128.0 * 60.0);
}

unsigned int bench_now_ms(void)
{
    clock_poll();
    return (unsigned int)((clock_ticks * 1000u) / 128u);
}

/* ---- key mapping -------------------------------------------------------- */

static bench_event_t map_event(key_event_t e)
{
    bench_event_t ev;

    ev.key = BK_NONE;
    ev.ch = 0;
    if (e.type == KEYEV_NONE)
        return ev;

    switch (e.key)
    {
    case KEY_UP: ev.key = BK_UP; break;
    case KEY_DOWN: ev.key = BK_DOWN; break;
    case KEY_LEFT: ev.key = BK_LEFT; break;
    case KEY_RIGHT: ev.key = BK_RIGHT; break;
    case KEY_EXE: ev.key = BK_EXE; break;
    case KEY_EXIT: ev.key = BK_EXIT; break;
    case KEY_MENU: ev.key = BK_MENU; break;
    case KEY_F1: ev.key = BK_F1; break;
    case KEY_F2: ev.key = BK_F2; break;
    case KEY_F3: ev.key = BK_F3; break;
    case KEY_F4: ev.key = BK_F4; break;
    case KEY_F5: ev.key = BK_F5; break;
    case KEY_F6: ev.key = BK_F6; break;
    case KEY_DEL: ev.key = BK_DEL; break;
    case KEY_ALPHA: ev.key = BK_ALPHA; break;
    case KEY_DOT: ev.key = BK_DOT; ev.ch = '.'; break;
    case KEY_NEG: ev.key = BK_MINUS; ev.ch = '-'; break;
    case KEY_ADD: ev.key = BK_ADD; ev.ch = '+'; break;
    case KEY_SUB: ev.key = BK_SUB; ev.ch = '-'; break;
    case KEY_MUL: ev.key = BK_MUL; ev.ch = '*'; break;
    case KEY_DIV: ev.key = BK_DIV; ev.ch = '/'; break;

    }

    /* Digits, through a table so the key-code layout stays in one place. */
    {
        static struct {
            int code;
            char ch;
        } const digits[] = {
            { KEY_0, '0' }, { KEY_1, '1' }, { KEY_2, '2' }, { KEY_3, '3' },
            { KEY_4, '4' }, { KEY_5, '5' }, { KEY_6, '6' }, { KEY_7, '7' },
            { KEY_8, '8' }, { KEY_9, '9' },
        };

        for (unsigned int i = 0; i < sizeof digits / sizeof digits[0]; i++)
        {
            if (e.key == digits[i].code)
            {
                ev.key = BK_DIGIT;
                ev.ch = digits[i].ch;
                break;
            }
        }
    }

    return ev;
}

bool input_alpha(void)
{
    return keydown(KEY_ALPHA) != 0;
}

bool input_exit_down(void)
{
    /* keydown() reports the scanned state of the key, which is what a long
     * job needs: no event has to be consumed to notice the press. */
    return keydown(KEY_EXIT) != 0;
}

bench_event_t input_next(void)
{
    /* Arrows repeat, everything else is a single press.  Modifiers are
     * reported but text entry does not depend on them (see input.h). */
    key_event_t e = getkey_opt(GETKEY_DEFAULT | GETKEY_REP_ARROWS |
                                   GETKEY_MOD_SHIFT | GETKEY_MOD_ALPHA,
                               NULL);

    return map_event(e);
}

bench_event_t input_next_timeout(int ms)
{
    bench_event_t ev;
    volatile int timeout = 0;
    int tid;
    key_event_t e;

    ev.key = BK_NONE;
    ev.ch = 0;
    if (ms <= 0)
        return input_next();

    tid = timer_configure(TIMER_ANY, (uint64_t)ms * 1000u,
                          GINT_CALL(timer_timeout, (void *)&timeout));
    if (tid < 0)
        return input_next();

    timer_start(tid);
    e = waitevent(&timeout);
    timer_stop(tid);

    if (timeout || e.type == KEYEV_NONE)
        return ev;
    return map_event(e);
}

char const *input_key_name(bench_key_t key)
{
    switch (key)
    {
    case BK_UP: return "UP";
    case BK_DOWN: return "DOWN";
    case BK_LEFT: return "LEFT";
    case BK_RIGHT: return "RIGHT";
    case BK_EXE: return "EXE";
    case BK_EXIT: return "EXIT";
    case BK_MENU: return "MENU";
    case BK_F1: return "F1";
    case BK_F2: return "F2";
    case BK_F3: return "F3";
    case BK_F4: return "F4";
    case BK_F5: return "F5";
    case BK_F6: return "F6";
    case BK_DEL: return "DEL";
    case BK_ALPHA: return "ALPHA";
    default: return "";
    }
}
