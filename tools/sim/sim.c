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
 * When the script runs out the screen is captured to $BENCH_SHOT (if set) and
 * the program exits, which is what the screenshot targets use. */

#include <gint/gint.h>

#include <dirent.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "sim_font.h"

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

void dclear(color_t color)
{
    for (int y = 0; y < DHEIGHT; y++)
        for (int x = 0; x < DWIDTH; x++)
            fb[y][x] = color;
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

static key_event_t finish(void)
{
    char const *out = getenv("BENCH_SHOT");

    if (out)
        capture(out);
    exit(0);
}

static key_event_t next_event(void)
{
    for (;;)
    {
        char const *tok;
        int k;

        if (script_at >= script_len)
            return finish();

        tok = script[script_at++];
        if (strncmp(tok, "shot:", 5) == 0)
        {
            capture(tok + 5);
            continue;
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

int main(void)
{
    char const *fs = getenv("BENCH_FS");
    char const *keys = getenv("BENCH_KEYS");

    snprintf(root, sizeof root, "%s", fs ? fs : "/tmp/benchfs");
    load_script(keys ? keys : "tools/sim/tour.txt");
    dclear(C_RGB(30, 60, 29));
    bench_main();
    finish();
    return 0;
}
