/* sim.c - a PC stand-in for gint, so the Bench screens can be run and
 * screenshotted without a calculator.
 *
 * This file is a development tool, never part of the add-in build:
 *   make sim        builds build/sim/bench
 *   BENCH_FS=/tmp/benchfs BENCH_KEYS=tools/sim/tour.txt build/sim/bench
 *
 * It implements the parts of gint the add-in calls (display, keys, timers, rtc,
 * kmalloc, BFile) on top of stdio and a 396x224 framebuffer.  Frames are
 * written as binary PPM; tools/sim/ppm2png.py converts them.
 *
 * Key script format, one token per line:
 *   up down left right exe exit menu f1..f6 del 0..9 . - alg
 *   shot:<file.ppm>    capture the screen and carry on
 *   tool:<id>          open a tool by registry id, skipping the home screen
 * When the script runs out the screen is captured to $BENCH_SHOT (if set) and
 * the program exits, which is what the screenshot targets use. */

#include <gint/gint.h>

#ifdef __EMSCRIPTEN__
/* The browser build of this same file.  Emscripten compiles the add-in and this
 * stand-in exactly as the desktop build does; the only difference is where keys
 * come from and where the frame goes.  See tools/wasm/build.sh. */
#include <emscripten.h>
#endif

#include <dirent.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "sim_font.h"

#include "apps/app.h"
#include "apps/registry.h"
#include "core/color.h"
#include "platform/screen.h"
#include "theme.h"

/* ---- framebuffer -------------------------------------------------------- */

static color_t fb[DHEIGHT][DWIDTH];
static color_t fg_color = 0;
static color_t bg_color = 0xFFFF;

static void put(int x, int y, color_t c)
{
    if (x < 0 || y < 0 || x >= DWIDTH || y >= DHEIGHT)
        return;
    fb[y][x] = c;
}

/* ---- frame trace --------------------------------------------------------
 *
 * src/platform/screen.c reports every rectangle it draws when BENCH_TRACE is
 * defined.  Two checks read that trace: nothing overlaps, and every label is
 * readable on what is behind it.  See docs/design.md section 5. */

enum { TRACE_MAX = 4000 };

typedef struct {
    int kind;
    int x;
    int y;
    int w;
    int h;
    int fg;
    int bg;
    char text[48];
} trace_rec_t;

static trace_rec_t trace[TRACE_MAX];
static int trace_n;
static int trace_dropped;

static int trace_is_text(int kind)
{
    return kind == SCR_TR_TEXT || kind == SCR_TR_TITLE ||
           kind == SCR_TR_SOFTKEY || kind == SCR_TR_TILE_TITLE ||
           kind == SCR_TR_STRIP;
}

void scr_trace(int kind, int x, int y, int w, int h, int fg, int bg,
               char const *text)
{
    trace_rec_t *r;

    if (trace_n >= TRACE_MAX)
    {
        trace_dropped++;
        return;
    }
    r = &trace[trace_n++];
    r->kind = kind;
    r->x = x;
    r->y = y;
    r->w = w;
    r->h = h;
    r->fg = fg;
    r->bg = bg;
    r->text[0] = 0;
    if (text)
    {
        int i = 0;

        for (; text[i] && i < (int)sizeof r->text - 1; i++)
            r->text[i] = text[i];
        r->text[i] = 0;
    }
}

void dclear(color_t color)
{
    for (int y = 0; y < DHEIGHT; y++)
        for (int x = 0; x < DWIDTH; x++)
            fb[y][x] = color;
    trace_n = 0;
    trace_dropped = 0;
}

void drect(int x1, int y1, int x2, int y2, color_t color)
{
    if (x1 > x2)
    {
        int t = x1;
        x1 = x2;
        x2 = t;
    }
    if (y1 > y2)
    {
        int t = y1;
        y1 = y2;
        y2 = t;
    }
    for (int y = y1; y <= y2; y++)
        for (int x = x1; x <= x2; x++)
            put(x, y, color);
}

void dpixel(int x, int y, color_t color)
{
    put(x, y, color);
}

void dline(int x1, int y1, int x2, int y2, color_t color)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy)
                    ? (dx < 0 ? -dx : dx)
                    : (dy < 0 ? -dy : dy);

    if (steps == 0)
    {
        put(x1, y1, color);
        return;
    }
    for (int i = 0; i <= steps; i++)
        put(x1 + dx * i / steps, y1 + dy * i / steps, color);
}

void dhline(int y, color_t color)
{
    drect(0, y, DWIDTH - 1, y, color);
}

void dvline(int x, color_t color)
{
    drect(x, 0, x, DHEIGHT - 1, color);
}

static font_t default_font;

font_t const *dfont_default(void)
{
    return &default_font;
}

font_t const *dfont(font_t const *font)
{
    (void)font;
    return &default_font;
}

/* Text: 6x10 cells, one entry per column (bit 0 = top). */
static void draw_text(int x, int y, color_t fg, color_t bg, char const *str,
                      int size)
{
    int cx = x;

    for (int i = 0; str[i] && (size < 0 || i < size); i++)
    {
        unsigned char c = (unsigned char)str[i];

        if (c >= SIM_FONT_FIRST && c <= SIM_FONT_LAST)
        {
            unsigned short const *glyph = sim_font[c - SIM_FONT_FIRST];

            for (int gx = 0; gx < SIM_FONT_W; gx++)
            {
                for (int gy = 0; gy < SIM_FONT_H; gy++)
                {
                    if (bg != C_NONE)
                        put(cx + gx, y + gy, bg);
                    if (glyph[gx] & (1u << gy))
                        put(cx + gx, y + gy, fg);
                }
            }
        }
        cx += SIM_FONT_W;
    }
}

void dtext(int x, int y, int fg, char const *str)
{
    draw_text(x, y, (color_t)fg, C_NONE, str, -1);
}

void dtext_opt(int x, int y, int fg, int bg, int halign, int valign,
               char const *str, int size)
{
    int w = 0;
    int h = 0;

    dnsize(str, size, NULL, &w, &h);
    if (halign == DTEXT_CENTER)
        x -= w / 2;
    else if (halign == DTEXT_RIGHT)
        x -= w;
    if (valign == DTEXT_MIDDLE)
        y -= h / 2;
    else if (valign == DTEXT_BOTTOM)
        y -= h;
    draw_text(x, y, (color_t)fg, (color_t)bg, str, size);
}

void dnsize(char const *str, int size, font_t const *font, int *w, int *h)
{
    int n = 0;

    (void)font;
    while (str[n] && (size < 0 || n < size))
        n++;
    if (w)
        *w = n * SIM_FONT_W;
    if (h)
        *h = SIM_FONT_H;
}

void dsize(char const *str, font_t const *font, int *w, int *h)
{
    dnsize(str, -1, font, w, h);
}

char const *drsize(char const *str, font_t const *font, int width, int *w)
{
    int n = 0;

    (void)font;
    while (str[n] && (n + 1) * SIM_FONT_W <= width)
        n++;
    if (w)
        *w = n * SIM_FONT_W;
    return str + n;
}


/* ---- the overlap and contrast checks -------------------------------------
 *
 * Run on the last frame of every key script when BENCH_CHECK=1:
 *
 *   1. no two labels overlap
 *   2. no label leaves the visible window
 *   3. no label sits in the title bar or the softkey strip
 *   4. no softkey label is wider than its cell
 *   5. a label is never painted over by a later panel
 *   6. every label clears WCAG AA against the pixels actually under it
 *      (3:1 for the title, which is the largest type on the screen)
 *   7. every tile icon is a different colour from its tile, by at least
 *      1.5:1 - the bug this check exists for
 *
 * The checks work on the frame as drawn, so they catch what the eye misses on
 * a 384x216 screen. */

static int findings;

static void finding(char const *rule, char const *text, int x, int y, int w,
                    int h)
{
    findings++;
    printf("  FAIL [%s] \"%s\" at %d,%d %dx%d\n", rule, text ? text : "", x, y,
           w, h);
}

static int rect_inside(int ax, int ay, int aw, int ah, int bx, int by, int bw,
                       int bh)
{
    return ax >= bx && ay >= by && ax + aw <= bx + bw && ay + ah <= by + bh;
}

static int rect_overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw,
                        int bh)
{
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

/* The two colours actually present in a rectangle, most common first.  A
 * one-bit font means there are at most a handful. */
static int colours_in(int wx, int wy, int w, int h, int *first, int *second)
{
    static int seen[16];
    static int count[16];
    int n = 0;
    /* The add-in draws in window coordinates; the framebuffer is inset by the
     * OS strip, so sample where the pixels really are. */
    int const x = wx + SCR_X;
    int const y = wy + SCR_Y;

    for (int yy = y; yy < y + h; yy++)
    {
        if (yy < 0 || yy >= DHEIGHT)
            continue;
        for (int xx = x; xx < x + w; xx++)
        {
            int c;
            int i;

            if (xx < 0 || xx >= DWIDTH)
                continue;
            c = fb[yy][xx];
            for (i = 0; i < n; i++)
                if (seen[i] == c)
                    break;
            if (i == n)
            {
                if (n == 16)
                    continue;
                seen[n] = c;
                count[n] = 0;
                n++;
            }
            count[i]++;
        }
    }

    {
        int best = -1;
        int best_n = 0;
        int second_c = -1;
        int second_n = 0;

        for (int i = 0; i < n; i++)
            if (count[i] > best_n)
            {
                best_n = count[i];
                best = seen[i];
            }
        for (int i = 0; i < n; i++)
            if (seen[i] != best && count[i] > second_n)
            {
                second_n = count[i];
                second_c = seen[i];
            }
        if (first)
            *first = best;
        if (second)
            *second = second_c;
        return n;
    }
}

static void check_palette(void)
{
    /* theme.h and docs/design.md must agree with the hex values, or every
     * measurement below is about a colour nobody chose. */
    struct { char const *name; int got; int r, g, b; } const want[] = {
        { "paper", TH_PAPER, 0xf4, 0xf3, 0xee },
        { "ink", TH_INK, 0x2f, 0x2d, 0x2e },
        { "red", TH_RED, 0xd9, 0x2d, 0x27 },
        { "yellow", TH_YELLOW, 0xf2, 0xc2, 0x30 },
        { "blue", TH_BLUE, 0x2f, 0x5f, 0xd0 },
        { "teal", TH_TEAL, 0x1f, 0x9c, 0x8a },
        { "violet", TH_VIOLET, 0x8b, 0x5c, 0xf6 },
        { "shade", TH_SHADE, 0xd9, 0xd7, 0xcd },
    };

    for (unsigned int i = 0; i < sizeof want / sizeof want[0]; i++)
        if (want[i].got != col_rgb565(want[i].r, want[i].g, want[i].b))
        {
            findings++;
            printf("  FAIL [palette] %s is 0x%04X, design.md says 0x%04X\n",
                   want[i].name, want[i].got,
                   col_rgb565(want[i].r, want[i].g, want[i].b));
        }
}

static int check_frame(char const *label)
{
    int alive[TRACE_MAX];
    int alive_n = 0;

    findings = 0;
    check_palette();

    /* Pass one: drop labels that a later panel painted over, and complain when
     * a panel only half covers one. */
    for (int i = 0; i < trace_n; i++)
    {
        trace_rec_t const *r = &trace[i];

        if (trace_is_text(r->kind))
        {
            alive[alive_n++] = i;
            continue;
        }
        if (r->kind != SCR_TR_FILL)
            continue;
        for (int a = 0; a < alive_n;)
        {
            trace_rec_t const *t = &trace[alive[a]];

            if (rect_inside(t->x, t->y, t->w, t->h, r->x, r->y, r->w, r->h))
            {
                alive[a] = alive[--alive_n];
                continue;
            }
            if (rect_overlap(t->x, t->y, t->w, t->h, r->x, r->y, r->w, r->h))
                finding("label painted over", t->text, t->x, t->y, t->w, t->h);
            a++;
        }
    }

    /* Pass two: placement. */
    for (int a = 0; a < alive_n; a++)
    {
        trace_rec_t const *t = &trace[alive[a]];
        int const in_title = t->y < TH_CONTENT_Y;
        int const in_soft = t->y + t->h > SCR_H - TH_SOFT_H;

        if (!rect_inside(t->x, t->y, t->w, t->h, 0, 0, SCR_W, SCR_H))
            finding("outside the window", t->text, t->x, t->y, t->w, t->h);

        if (t->kind == SCR_TR_TEXT && (in_title || in_soft))
            finding("label in the title bar or softkey strip", t->text, t->x,
                    t->y, t->w, t->h);

        if (t->kind == SCR_TR_SOFTKEY && t->w > TH_SOFT_W - 4)
            finding("softkey label wider than its cell", t->text, t->x, t->y,
                    t->w, t->h);
    }

    /* Pass three: two labels in the same pixels. */
    for (int a = 0; a < alive_n; a++)
        for (int b = a + 1; b < alive_n; b++)
        {
            trace_rec_t const *p = &trace[alive[a]];
            trace_rec_t const *q = &trace[alive[b]];

            if (rect_overlap(p->x, p->y, p->w, p->h, q->x, q->y, q->w, q->h))
                finding("labels overlap", p->text, p->x, p->y, p->w, p->h);
        }

    /* Pass four: readability, measured from the pixels, not from the constants. */
    for (int a = 0; a < alive_n; a++)
    {
        trace_rec_t const *t = &trace[alive[a]];
        int c1 = -1;
        int blank = 1;

        /* A label of spaces is a spacer, not something to read. */
        for (char const *p = t->text; *p; p++)
            if (*p != ' ')
                blank = 0;
        if (blank)
            continue;
        int c2 = -1;
        int n = colours_in(t->x, t->y, t->w, t->h, &c1, &c2);
        int const big = t->kind == SCR_TR_TITLE || t->kind == SCR_TR_TILE_TITLE;
        int const floor = big ? COL_TITLE_MIN_CONTRAST : COL_TEXT_MIN_CONTRAST;

        if (n < 2 || c1 < 0 || c2 < 0)
        {
            finding("label and background are the same colour", t->text, t->x,
                    t->y, t->w, t->h);
            continue;
        }
        if (col_contrast(c1, c2) < floor)
        {
            finding("label under the contrast floor", t->text, t->x, t->y, t->w,
                    t->h);
            printf("        %.2f:1 between 0x%04X and 0x%04X, floor %d, kind %d\n",
                   col_contrast(c1, c2), c1, c2, floor, t->kind);
        }
    }

    /* Pass five: icons. */
    for (int i = 0; i < trace_n; i++)
    {
        trace_rec_t const *r = &trace[i];

        if (r->kind != SCR_TR_ICON)
            continue;
        if (r->fg == r->bg)
            finding("icon colour equals its tile", r->text, r->x, r->y, r->w,
                    r->h);
        else if (col_contrast(r->fg, r->bg) < COL_ICON_MIN_CONTRAST)
            finding("icon under the 1.5:1 floor", r->text, r->x, r->y, r->w,
                    r->h);
    }

    if (trace_dropped)
    {
        findings++;
        printf("  FAIL [trace] %d records dropped, raise TRACE_MAX\n",
               trace_dropped);
    }

    if (findings)
        printf("  %s: %d finding%s\n", label, findings, findings == 1 ? "" : "s");
    return findings;
}

/* ---- screenshots -------------------------------------------------------- */

static void capture(char const *path)
{
    FILE *f = fopen(path, "wb");

    if (!f)
    {
        fprintf(stderr, "sim: cannot write %s\n", path);
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", DWIDTH, DHEIGHT);
    for (int y = 0; y < DHEIGHT; y++)
    {
        for (int x = 0; x < DWIDTH; x++)
        {
            unsigned int c = fb[y][x];
            /* Standard RGB565, the way the fx-CG 50 hardware reads it. */
            unsigned int r = (c >> 11) & 0x1F;
            unsigned int g = (c >> 5) & 0x3F;
            unsigned int b = c & 0x1F;
            unsigned char px[3];

            px[0] = (unsigned char)(r * 255 / 31);
            px[1] = (unsigned char)(g * 255 / 63);
            px[2] = (unsigned char)(b * 255 / 31);
            fwrite(px, 1, 3, f);
        }
    }
    fclose(f);
}

void dupdate(void)
{
    /* The real dupdate() pushes the frame; here the capture happens on demand
     * so a screenshot always shows the last drawn frame. */
}

/* ---- keys --------------------------------------------------------------- */

static char script[512][128];
static int script_len;
static int script_at;

static int key_for_token(char const *tok)
{
    struct { char const *name; int key; } const names[] = {
        { "up", KEY_UP }, { "down", KEY_DOWN }, { "left", KEY_LEFT },
        { "right", KEY_RIGHT }, { "exe", KEY_EXE }, { "exit", KEY_EXIT },
        { "menu", KEY_MENU }, { "del", KEY_DEL }, { "f1", KEY_F1 },
        { "f2", KEY_F2 }, { "f3", KEY_F3 }, { "f4", KEY_F4 }, { "f5", KEY_F5 },
        { "f6", KEY_F6 }, { "dot", KEY_DOT }, { "minus", KEY_NEG },
        { "alpha", KEY_ALPHA }, { "0", KEY_0 }, { "1", KEY_1 },
        { "2", KEY_2 }, { "3", KEY_3 }, { "4", KEY_4 }, { "5", KEY_5 },
        { "6", KEY_6 }, { "7", KEY_7 }, { "8", KEY_8 }, { "9", KEY_9 },
    };

    for (unsigned int i = 0; i < sizeof names / sizeof names[0]; i++)
        if (strcmp(tok, names[i].name) == 0)
            return names[i].key;
    return -1;
}

static int check_frame(char const *label);

static key_event_t finish(void)
{
    char const *out = getenv("BENCH_SHOT");
    char const *check = getenv("BENCH_CHECK");

    if (out)
        capture(out);
    if (check && *check)
    {
        char const *name = getenv("BENCH_NAME");

        if (check_frame(name ? name : "frame"))
            exit(3);
    }
    exit(0);
}

#ifdef __EMSCRIPTEN__
/* ---- browser keys -------------------------------------------------------
 *
 * JS pushes keys with bench_wasm_key(); getkey() blocks (Asyncify turns the
 * sleep into a return to the browser) until one arrives.  No key script, no
 * screenshot target: the frame is pulled by bench_wasm_frame(). */

static int wasm_queue[16];
static int wasm_head, wasm_tail;

EMSCRIPTEN_KEEPALIVE void bench_wasm_key(int key)
{
    int next = (wasm_tail + 1) % (int)(sizeof wasm_queue / sizeof wasm_queue[0]);

    if (next == wasm_head)
        return;                     /* the queue is full; drop the press */
    wasm_queue[wasm_tail] = key;
    wasm_tail = next;
}
#endif

static key_event_t next_event(void)
{
    for (;;)
    {
        char const *tok;
        int k;

#ifdef __EMSCRIPTEN__
        /* A key from the page, else wait for one.  The key script is empty in
         * the browser, so this is the only path. */
        while (wasm_head == wasm_tail)
            emscripten_sleep(16);
        {
            key_event_t e;

            k = wasm_queue[wasm_head];
            wasm_head = (wasm_head + 1) % (int)(sizeof wasm_queue / sizeof wasm_queue[0]);
            memset(&e, 0, sizeof e);
            e.type = KEYEV_DOWN;
            e.key = (sim_uint)k;
            e.mod = 1;
            return e;
        }
#endif
        if (script_at >= script_len)
            return finish();

        tok = script[script_at++];
        if (strncmp(tok, "shot:", 5) == 0)
        {
            capture(tok + 5);
            continue;
        }
        /* Jump straight to a tool by id: a tour then survives the home screen
         * being reordered, which it did not when the only way in was counting
         * tiles. */
        if (strncmp(tok, "tool:", 5) == 0)
        {
            tool_t const *t = tool_by_id(tok + 5);

            if (!t)
            {
                fprintf(stderr, "sim: no tool '%s'\n", tok + 5);
                continue;
            }
            app_open(t->screen);
            /* Hand back a dead key so the main loop draws the new screen
             * before the script moves on; opening and returning here would
             * leave the old frame in the capture. */
            key_event_t e;

            memset(&e, 0, sizeof e);
            e.type = KEYEV_DOWN;
            e.key = BK_NONE;
            return e;
        }
        k = key_for_token(tok);
        if (k < 0)
        {
            fprintf(stderr, "sim: unknown key script token '%s'\n", tok);
            continue;
        }
        key_event_t e;

        memset(&e, 0, sizeof e);
        e.type = KEYEV_DOWN;
        e.key = (sim_uint)k;
        e.mod = 1;
        return e;
    }
}

key_event_t getkey(void)
{
    return next_event();
}

key_event_t getkey_opt(int options, volatile int *timeout)
{
    (void)options;
    if (timeout && *timeout)
        return next_event();
    return next_event();
}

key_event_t pollevent(void)
{
    return next_event();
}

key_event_t waitevent(volatile int *timeout)
{
    (void)timeout;
    return next_event();
}

int keydown(int key)
{
    (void)key;
    return 0;
}

int keydown_all(int first, ...)
{
    (void)first;
    return 0;
}

int keydown_any(int first, ...)
{
    (void)first;
    return 0;
}

void clearevents(void)
{
}

/* ---- timers, clock ------------------------------------------------------ */

int gint_call(gint_call_t cb)
{
    if (!cb.function)
        return 0;
    return ((int (*)(void *))cb.function)(cb.args[0].p);
}

int timer_configure(int timer, uint64_t delay_us, gint_call_t callback)
{
    (void)delay_us;
    (void)callback;
    return timer;
}

void timer_start(int timer)
{
    (void)timer;
}

void timer_stop(int timer)
{
    (void)timer;
}

int timer_timeout(volatile void *arg)
{
    if (arg)
        *(volatile int *)arg = 1;
    return TIMER_STOP;
}

static uint32_t sim_ticks = 128u * 60u * 7u;

uint32_t rtc_ticks(void)
{
    sim_ticks += 8;             /* a bit faster than real time, so clocks move */
    return sim_ticks;
}

/* ---- heap --------------------------------------------------------------- */

void *kmalloc(size_t size, char const *arena_name)
{
    (void)arena_name;
    return malloc(size);
}

void *krealloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void kfree(void *ptr)
{
    free(ptr);
}

void *kmalloc_max(size_t *size, char const *arena_name)
{
    (void)arena_name;
    if (size)
        *size = 512u * 1024u;
    return malloc(1024);
}

/* ---- filesystem --------------------------------------------------------- */

static char root[512];

static void path_to_host(uint16_t const *path, char *out, int size)
{
    char ascii[512];
    int n = 0;

    while (path[n] && n < (int)sizeof ascii - 1)
    {
        ascii[n] = (char)(path[n] == '\\' ? '/' : path[n]);
        n++;
    }
    ascii[n] = 0;

    /* "\\fls0/Bench/..." becomes "$BENCH_FS/..." */
    if (strncmp(ascii, "//fls0/Bench", 12) == 0)
        snprintf(out, (size_t)size, "%s%s", root, ascii + 12);
    else if (strncmp(ascii, "//fls0", 6) == 0)
        snprintf(out, (size_t)size, "%s%s", root, ascii + 6);
    else
        snprintf(out, (size_t)size, "%s/%s", root, ascii);
}

#define SIM_MAX_FILES 16

typedef struct {
    FILE *f;
    int used;
} sim_file_t;

static sim_file_t sim_files[SIM_MAX_FILES];

int BFile_Create(uint16_t const *path, int type, int *size)
{
    char host[600];

    path_to_host(path, host, (int)sizeof host);
    if (getenv("SIM_TRACE"))
        fprintf(stderr, "sim: create '%s' type %d size %d\n", host, type,
                size ? *size : -1);
    if (type == BFile_Folder)
    {
        mkdir(host, 0777);
        return 0;
    }
    {
        FILE *f = fopen(host, "wb");

        if (!f)
            return -1;
        if (size && *size > 0)
        {
            for (int i = 0; i < *size; i++)
                fputc(0xFF, f);
        }
        fclose(f);
    }
    return 0;
}

int BFile_Remove(uint16_t const *path)
{
    char host[600];

    path_to_host(path, host, (int)sizeof host);
    remove(host);
    return 0;
}

int BFile_Rename(uint16_t const *oldpath, uint16_t const *newpath)
{
    char a[600];
    char b[600];

    path_to_host(oldpath, a, (int)sizeof a);
    path_to_host(newpath, b, (int)sizeof b);
    return rename(a, b);
}

int BFile_Open(uint16_t const *path, int mode)
{
    char host[600];

    path_to_host(path, host, (int)sizeof host);
    for (int i = 0; i < SIM_MAX_FILES; i++)
    {
        if (!sim_files[i].used)
        {
            FILE *f = fopen(host, (mode & BFile_WriteOnly) ? "r+b" : "rb");

            if (!f)
                return -1;
            sim_files[i].used = 1;
            sim_files[i].f = f;
            return i;
        }
    }
    return -1;
}

int BFile_Close(int fd)
{
    if (fd < 0 || fd >= SIM_MAX_FILES || !sim_files[fd].used)
        return -1;
    fclose(sim_files[fd].f);
    sim_files[fd].used = 0;
    return 0;
}

int BFile_Size(int fd)
{
    long pos;
    long end;

    if (fd < 0 || fd >= SIM_MAX_FILES || !sim_files[fd].used)
        return -1;
    pos = ftell(sim_files[fd].f);
    fseek(sim_files[fd].f, 0, SEEK_END);
    end = ftell(sim_files[fd].f);
    fseek(sim_files[fd].f, pos, SEEK_SET);
    return (int)end;
}

int BFile_Write(int fd, void const *data, int even_size)
{
    size_t n;

    if (fd < 0 || fd >= SIM_MAX_FILES || !sim_files[fd].used)
        return -1;
    n = fwrite(data, 1, (size_t)even_size, sim_files[fd].f);
    return even_size - (int)n;
}

int BFile_Read(int handle, void *data, int size, int whence)
{
    size_t n;

    if (handle < 0 || handle >= SIM_MAX_FILES || !sim_files[handle].used)
        return -1;
    if (whence >= 0)
        fseek(sim_files[handle].f, whence, SEEK_SET);
    n = fread(data, 1, (size_t)size, sim_files[handle].f);
    return (int)n;
}

int BFile_Seek(int fd, int offset)
{
    if (fd < 0 || fd >= SIM_MAX_FILES || !sim_files[fd].used)
        return -1;
    fseek(sim_files[fd].f, offset, SEEK_SET);
    return 0;
}

static glob_t sim_glob[4];
static int sim_glob_used[4];
static int sim_glob_at[4];

int BFile_FindFirst(uint16_t const *pattern, int *shandle, uint16_t *foundfile,
                    struct BFile_FileInfo *fileinfo)
{
    char host[600];
    int slot = -1;

    path_to_host(pattern, host, (int)sizeof host);
    if (getenv("SIM_TRACE"))
        fprintf(stderr, "sim: find '%s'\n", host);
    for (int i = 0; i < 4; i++)
        if (!sim_glob_used[i])
        {
            slot = i;
            break;
        }
    if (slot < 0)
        return -1;
    memset(&sim_glob[slot], 0, sizeof sim_glob[slot]);
    if (glob(host, 0, NULL, &sim_glob[slot]) != 0)
    {
        sim_glob_used[slot] = 1;
        sim_glob_at[slot] = 0;
        *shandle = slot;
        return -1;
    }
    sim_glob_used[slot] = 1;
    sim_glob_at[slot] = 0;
    *shandle = slot;

    /* First match. */
    {
        char const *m = sim_glob[slot].gl_pathv[0];
        int n = 0;

        while (m[n] && n < 100)
        {
            foundfile[n] = (uint16_t)(m[n] == '/' ? '\\' : m[n]);
            n++;
        }
        foundfile[n] = 0;
        if (fileinfo)
        {
            memset(fileinfo, 0, sizeof *fileinfo);
            fileinfo->type = BFile_File;
            fileinfo->file_size = 512;
        }
    }
    return 0;
}

int BFile_FindNext(int shandle, uint16_t *foundfile,
                   struct BFile_FileInfo *fileinfo)
{
    char const *m;
    int n = 0;

    if (shandle < 0 || shandle >= 4 || !sim_glob_used[shandle])
        return -1;
    sim_glob_at[shandle]++;
    if (sim_glob_at[shandle] >= (int)sim_glob[shandle].gl_pathc)
        return -1;
    m = sim_glob[shandle].gl_pathv[sim_glob_at[shandle]];
    while (m && m[n] && n < 100)
    {
        foundfile[n] = (uint16_t)(m[n] == '/' ? '\\' : m[n]);
        n++;
    }
    foundfile[n] = 0;
    if (fileinfo)
    {
        memset(fileinfo, 0, sizeof *fileinfo);
        fileinfo->type = BFile_File;
        fileinfo->file_size = 512;
    }
    return 0;
}

int BFile_FindClose(int shandle)
{
    if (shandle < 0 || shandle >= 4 || !sim_glob_used[shandle])
        return -1;
    globfree(&sim_glob[shandle]);
    sim_glob_used[shandle] = 0;
    return 0;
}

int gint_world_switch(gint_call_t function)
{
    if (getenv("SIM_TRACE") && function.function)
        fprintf(stderr, "sim: world switch\n");
    return gint_call(function);
}

void gint_osmenu(void)
{
}

/* ---- registry dump ------------------------------------------------------
 *
 * `BENCH_DUMP_TOOLS=1 build/sim/bench` writes the tool registry as JSON and
 * exits.  tools/gen_tools_json.py runs it to write docs/tools.json, so the JSON
 * the site and the README read is the same table the menu draws - never a
 * hand-edited copy. */

static void json_string(char const *s)
{
    putchar('"');
    for (; s && *s; s++)
    {
        switch (*s)
        {
        case '"': fputs("\\\"", stdout); break;
        case '\\': fputs("\\\\", stdout); break;
        case '\n': fputs("\\n", stdout); break;
        case '\t': fputs("\\t", stdout); break;
        default: putchar(*s); break;
        }
    }
    putchar('"');
}

static int dump_tools(void)
{
    printf("[\n");
    for (int i = 0; i < tool_count(); i++)
    {
        tool_t const *t = tool_at(i);

        printf("  {\n    \"id\": ");
        json_string(t->id);
        printf(",\n    \"title\": ");
        json_string(t->title);
        printf(",\n    \"subtitle\": ");
        json_string(t->subtitle);
        printf(",\n    \"category\": ");
        json_string(tool_category_name(t->category));
        printf(",\n    \"screen\": %d", (int)t->screen);
        printf(",\n    \"needs_file\": %s", t->needs_file ? "true" : "false");
        printf(",\n    \"help\": ");
        json_string(t->help);
        printf("\n  }%s\n", i + 1 < tool_count() ? "," : "");
    }
    printf("]\n");
    return 0;
}

/* ---- entry -------------------------------------------------------------- */

int bench_main(void);

static void load_script(char const *path)
{
    FILE *f = fopen(path, "r");
    char line[128];

    if (!f)
    {
        fprintf(stderr, "sim: no key script at %s\n", path);
        return;
    }
    while (script_len < (int)(sizeof script / sizeof script[0]) &&
           fgets(line, (int)sizeof line, f))
    {
        size_t n = strlen(line);

        while (n && (line[n - 1] == '\n' || line[n - 1] == '\r'))
            line[--n] = 0;
        if (n == 0 || line[0] == '#')
            continue;
        if (n >= sizeof script[0])
        {
            fprintf(stderr, "sim: key script line too long\n");
            continue;
        }
        snprintf(script[script_len], sizeof script[0], "%s", line);
        script_len++;
    }
    fclose(f);
}

#ifdef __EMSCRIPTEN__
/* Called once from the page.  bench_main() never returns: it is the add-in's
 * own event loop, parked inside getkey(). */
EMSCRIPTEN_KEEPALIVE int bench_wasm_run(void)
{
    snprintf(root, sizeof root, "%s", "/benchfs");
    script_len = 0;
    script_at = 0;
    dclear(C_RGB(30, 60, 29));
    bench_main();
    return 0;
}

/* The framebuffer as RGBA, one byte per channel, top row first. */
EMSCRIPTEN_KEEPALIVE void bench_wasm_frame(unsigned char *out)
{
    for (int y = 0; y < DHEIGHT; y++)
        for (int x = 0; x < DWIDTH; x++)
        {
            color_t c = fb[y][x];
            unsigned char *p = out + ((size_t)y * DWIDTH + (size_t)x) * 4;

            p[0] = (unsigned char)(((c >> 11) & 0x1F) * 255 / 31);
            p[1] = (unsigned char)(((c >> 5) & 0x3F) * 255 / 63);
            p[2] = (unsigned char)((c & 0x1F) * 255 / 31);
            p[3] = 255;
        }
}

EMSCRIPTEN_KEEPALIVE int bench_wasm_width(void)
{
    return DWIDTH;
}

EMSCRIPTEN_KEEPALIVE int bench_wasm_height(void)
{
    return DHEIGHT;
}

#endif /* __EMSCRIPTEN__ */

int main(void)
{
    char const *fs = getenv("BENCH_FS");
    char const *keys = getenv("BENCH_KEYS");

    if (getenv("BENCH_DUMP_TOOLS"))
        return dump_tools();

    snprintf(root, sizeof root, "%s", fs ? fs : "/tmp/benchfs");
    load_script(keys ? keys : "tools/sim/tour.txt");
    dclear(C_RGB(30, 60, 29));
    bench_main();
    finish();
    return 0;
}
