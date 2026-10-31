/* Bench spike add-in - throwaway, answers the ten M1 questions.
 *
 * Spec: docs/plan.md section 7, M1 spike questions; docs/tasks/TASK-M1.md.
 * Every gint call below is taken from the gint 2.10.0 headers; the header and
 * line are named in a comment next to the call and in docs/spike.md.
 *
 * This file is NOT part of the Bench add-in.  It is built on its own so it can
 * be deleted after the spike (see spike/CMakeLists.txt).
 *
 * Controls: UP/DOWN choose a test, EXE runs it, EXIT leaves the add-in.
 * Each test shows its result lines; press any key to go back to the menu. */

#include <gint/bfile.h>        /* BFile_* (bfile.h) */
#include <gint/defs/attributes.h> /* GUNUSED */
#include <gint/defs/call.h>    /* GINT_CALL (defs/call.h) */
#include <gint/display.h>      /* dclear/drect/dtext/dupdate/dsize */
#include <gint/gint.h>         /* gint_world_switch (gint.h) */
#include <gint/keyboard.h>     /* getkey, key_event_t */
#include <gint/keycodes.h>     /* KEY_UP, KEY_DOWN, KEY_EXE, KEY_EXIT */
#include <gint/kmalloc.h>      /* kmalloc_max, kmalloc_get_arena, kfree */
#include <gint/rtc.h>          /* rtc_ticks, rtc_get_time, rtc_time_t,
                                * rtc_periodic_enable/disable */

#include <stdio.h>             /* snprintf: spike question 3 */
#include <stdlib.h>            /* malloc/free: spike question 7 */
#include <string.h>

/* ---- palette (same values as src/main.c) -------------------------------- */
#define PAPER C_RGB(30, 60, 29)
#define INK C_RGB(4, 8, 4)
#define ACCENT C_RGB(27, 11, 4)

#define LINE_MAX 14
#define LINE_LEN 52

static char lines[LINE_MAX][LINE_LEN];
static int nlines;

static void out_reset(void)
{
    nlines = 0;
    for (int i = 0; i < LINE_MAX; i++)
        lines[i][0] = '\0';
}

static void out(char const *text)
{
    if (nlines >= LINE_MAX)
        return;
    strncpy(lines[nlines], text, LINE_LEN - 1);
    lines[nlines][LINE_LEN - 1] = '\0';
    nlines++;
}

/* label + decimal integer, without printf */
static void out_int(char const *label, long value)
{
    char buf[LINE_LEN];
    char digits[24];
    int n = 0, i = 0;
    unsigned long v = value < 0 ? (unsigned long)(-value) : (unsigned long)value;

    while (*label && i < LINE_LEN - 2)
        buf[i++] = *label++;
    if (value < 0 && i < LINE_LEN - 2)
        buf[i++] = '-';
    do {
        digits[n++] = (char)('0' + (v % 10));
        v /= 10;
    } while (v && n < (int)sizeof digits);
    while (n > 0 && i < LINE_LEN - 1)
        buf[i++] = digits[--n];
    buf[i] = '\0';
    out(buf);
}

/* ---- drawing ------------------------------------------------------------ */

static void draw_lines(char const *title)
{
    int y = 26;

    dclear(PAPER);
    drect(0, 0, DWIDTH - 1, 20, ACCENT);
    dtext(6, 4, PAPER, title);

    for (int i = 0; i < nlines; i++) {
        dtext(6, y, INK, lines[i]);
        y += 13;
    }
    dupdate();
}

static void wait_any_key(void)
{
    out("press any key");
    for (int i = nlines; i < LINE_MAX; i++)
        lines[i][0] = '\0';
    dtext(6, 26 + LINE_MAX * 13, INK, "press any key to go back");
    dupdate();
    getkey();
}

/* ---- BFile helpers (must run under gint_world_switch) ------------------- */

static uint16_t const *path_bench_dir = u"\\\\fls0\\Bench";
static uint16_t const *path_test_file = u"\\\\fls0\\Bench\\spike.txt";
static uint16_t const *path_glob = u"\\\\fls0\\Bench\\*";
static char const *test_text = "Bench spike file";
static char read_back[32];

/* Question 1: which calls list, read, write and delete.
 * Question 2: what the paths look like. */
static int files_create_write(void)
{
    int size = 0;
    int r_create_dir = BFile_Create(path_bench_dir, BFile_Folder, NULL);
    int r_create_file = BFile_Create(path_test_file, BFile_File, &size);
    int fd = BFile_Open(path_test_file, BFile_WriteOnly);
    int r_write = -1;

    if (fd >= 0) {
        r_write = BFile_Write(fd, test_text, 16); /* even size */
        (void)BFile_Close(fd);
    }
    out_int("mkdir Bench: ", r_create_dir);
    out_int("create file: ", r_create_file);
    out_int("open wo: ", fd);
    out_int("write 16: ", r_write);
    return 0;
}

static int files_read_list_delete(void)
{
    int fd = BFile_Open(path_test_file, BFile_ReadOnly);
    int r_read = -1, r_size = -1;
    int shandle = 0;
    struct BFile_FileInfo info;
    uint16_t found[64];
    int r_find = -1, r_next = -1, r_close = -1, r_remove = -1;

    if (fd >= 0) {
        r_size = BFile_Size(fd);
        memset(read_back, 0, sizeof read_back);
        r_read = BFile_Read(fd, read_back, 16, 0);
        (void)BFile_Close(fd);
    }
    read_back[16] = '\0';

    r_find = BFile_FindFirst(path_glob, &shandle, found, &info);
    if (r_find == 0) {
        r_next = BFile_FindNext(shandle, found, &info);
        r_close = BFile_FindClose(shandle);
    }

    out_int("open ro: ", fd);
    out_int("size: ", r_size);
    out_int("read 16: ", r_read);
    out("text: ");
    out(read_back);
    out_int("findfirst: ", r_find);
    out_int("findnext: ", r_next);
    out_int("findclose: ", r_close);

    /* Delete the test file so the card is left clean. */
    r_remove = BFile_Remove(path_test_file);
    out_int("remove file: ", r_remove);
    return 0;
}

/* Question 3: does printf with floats work? */
static void test_printf(void)
{
    char a[48], b[48], c[48];
    int r1 = snprintf(a, sizeof a, "%.2f", 1.5);
    int r2 = snprintf(b, sizeof b, "%f", 1.5);
    int r3 = snprintf(c, sizeof c, "%.3e", 1234.5);

    out_int("snprintf %.2f = ", r1);
    out(a);
    out_int("snprintf %f = ", r2);
    out(b);
    out_int("snprintf %.3e = ", r3);
    out(c);

    /* dprint uses the same formatter. */
    dclear(PAPER);
    dtext(6, 6, INK, "dprint %f:");
    dprint(6, 20, INK, "%f", 1.5);
    dupdate();
    getkey();
}

/* Question 4: which symbols does the default font have? */
static void test_font(void)
{
    /* All samples are UTF-8, the encoding the loader will use. */
    char const *samples[] = {
        "\xC2\xB0" "C",             /* U+00B0 degree */
        "\xC2\xB1" "0.1",           /* U+00B1 plus-minus */
        "\xC3\x97" "3",             /* U+00D7 times */
        "\xC2\xB5" "m",             /* U+00B5 micro */
        "\xCE\xB1\xCE\xB2\xCE\xB3", /* alpha beta gamma */
        "\xE2\x88\x9A" "2",        /* U+221A square root */
        "\xE2\x89\xA4",            /* U+2264 less or equal */
        "\xCF\x80",                 /* U+03C0 pi */
        "\xE2\x86\x92",            /* U+2192 right arrow */
    };
    int widths[9];

    for (unsigned i = 0; i < sizeof samples / sizeof *samples; i++) {
        int w = 0, h = 0;
        dsize(samples[i], NULL, &w, &h);
        widths[i] = w;
    }

    dclear(PAPER);
    dtext(6, 4, INK, "font symbols, 2nd column = width");
    for (unsigned i = 0; i < sizeof samples / sizeof *samples; i++) {
        dtext(6, 20 + (int)i * 20, INK, samples[i]);
        out_int("w=", widths[i]);
        dtext(200, 20 + (int)i * 20, ACCENT, lines[nlines - 1]);
    }
    dupdate();
    getkey();
}

/* Question 5: how do you read ALPHA letters?
 *
 * gint 2.10.0 has no KEY_A..KEY_Z and no keycode_alpha() helper: getkey()
 * reports the physical keycode plus the alpha modifier bit (key_event_t in
 * keyboard.h).  This test prints exactly that, so the mapping from keycode to
 * letter can be read off the real device. */
static void test_alpha(void)
{
    int shown = 0;

    out("press ALPHA then A..Z");
    out("EXIT stops the test");

    while (shown < 12) {
        key_event_t ev = getkey();
        char buf[LINE_LEN];

        if (ev.key == KEY_EXIT)
            break;
        if (ev.type != KEYEV_DOWN && ev.type != KEYEV_HOLD)
            continue;

        snprintf(buf, sizeof buf, "key=0x%02X alpha=%d shift=%d",
                 (unsigned)ev.key, (int)ev.alpha, (int)ev.shift);
        out(buf);
        shown++;

        dclear(PAPER);
        drect(0, 0, DWIDTH - 1, 18, ACCENT);
        dtext(6, 3, PAPER, "5 alpha keys (EXIT stops)");
        for (int i = 0; i < nlines; i++)
            dtext(6, 26 + i * 13, INK, lines[i]);
        dupdate();
    }
    wait_any_key();
}

/* Question 6: which timer or RTC call ticks once per second?
 *
 * Two candidates, both from gint/rtc.h:
 *   rtc_ticks()                      a 128 Hz counter since midnight
 *   rtc_periodic_enable(1, call)     a 1 Hz periodic interrupt
 * The screen shows a counter the interrupt bumps, the live clock and the
 * 128 Hz counter, so the answer can be read off the device. */
static volatile int tick_count = 0;

static int on_rtc_tick(void)
{
    tick_count++;
    return TIMER_CONTINUE;
}

static void test_clock(void)
{
    rtc_time_t t0, t1;
    uint32_t ticks0, ticks1;
    int started;

    out("watch the counter and the s");
    started = rtc_periodic_enable(1, GINT_CALL(on_rtc_tick));
    rtc_get_time(&t0);
    ticks0 = rtc_ticks();

    if (!started)
        out("periodic interrupt busy");

    for (int loops = 0; loops < 4000; loops++) {
        rtc_time_t now;
        rtc_get_time(&now);

        dclear(PAPER);
        drect(0, 0, DWIDTH - 1, 18, ACCENT);
        dtext(6, 3, PAPER, "6 clock: 1 Hz interrupt");
        out_int("1 Hz counter: ", tick_count);
        out_int("rtc_ticks/128 s: ", (long)(rtc_ticks() / 128));
        out_int("RTC seconds: ", now.seconds);
        out_int("RTC sub ticks: ", now.ticks);
        for (int i = 0; i < nlines; i++)
            dtext(6, 28 + i * 20, INK, lines[i]);
        dtext(6, 200, ACCENT, "press a key after about 5 s");
        dupdate();

        if (pollevent().type == KEYEV_DOWN)
            break;
    }

    rtc_periodic_disable();
    rtc_get_time(&t1);
    ticks1 = rtc_ticks();

    out_int("counted: ", tick_count);
    out_int("seconds passed: ", (long)t1.seconds - (long)t0.seconds);
    out_int("ticks passed: ", (long)(ticks1 - ticks0));
    wait_any_key();
}

/* Question 7: how much heap does malloc give? */
static void test_heap(void)
{
    size_t largest = 0;
    size_t odir = 0, ram = 0;
    void *p;

    /* Largest block in each gint arena (names from gint's kernel.c). */
    p = kmalloc_max(&odir, "_ostk");
    if (p)
        kfree(p);
    p = kmalloc_max(&ram, "_uram");
    if (p)
        kfree(p);

    out_int("ostk arena: ", (long)(odir / 1024));
    out("  (KiB)");
    out_int("uram arena: ", (long)(ram / 1024));
    out("  (KiB)");

    /* Largest single malloc that succeeds, by doubling then halving. */
    {
        size_t size = 1024;
        while (size < (size_t)32 * 1024 * 1024) {
            p = malloc(size);
            if (!p)
                break;
            free(p);
            size *= 2;
        }
        largest = size;
        while (largest > 1024) {
            size_t probe = largest / 2;
            p = malloc(probe);
            if (!p)
                break;
            free(p);
            largest = probe;
        }
    }

    out_int("largest malloc KiB: ", (long)(largest / 1024));
    wait_any_key();
}

/* Question 8: exact drawable width and height */
static void draw_corners(void)
{
    dclear(PAPER);
    /* Frame the OS window the header describes: 384x216 centred in 396x224. */
    drect(6, 4, 6 + 383, 4 + 215, INK);
    drect(0, 0, DWIDTH - 1, DHEIGHT - 1, ACCENT);
    dtext(10, 10, ACCENT, "top-left of the OS window");
    dtext(10, 200, ACCENT, "bottom-right");
    dupdate();
}

static void test_size(void)
{
    out_int("DWIDTH: ", DWIDTH);
    out_int("DHEIGHT: ", DHEIGHT);
    out("384x216 frame drawn");
    draw_lines("8 size: check the frame");
    getkey();

    draw_corners();
    getkey();
}

/* Question 9: how long does a full redraw take? */
static void test_redraw(void)
{
    uint32_t t0, t1;
    int frames = 50;

    t0 = rtc_ticks();
    for (int i = 0; i < frames; i++) {
        dclear(PAPER);
        drect(6, 4, 389, 219, INK);
        dtext(20, 100, ACCENT, "redraw test");
        dupdate();
    }
    t1 = rtc_ticks();

    out_int("frames: ", frames);
    out_int("ticks total: ", (long)(t1 - t0));
    /* rtc_ticks() is 128 Hz (gint/rtc.h), so this is milliseconds. */
    out_int("ms per frame x10: ",
            (long)(((uint64_t)(t1 - t0) * 10000u) / 128u / (uint32_t)frames));
    wait_any_key();
}

/* Question 10: how big is the add-in itself? */
static void test_self_size(void)
{
    int type = 0, size = -1;
    int r;

    out("Read the .g3a size in");
    out("Explorer on Windows.");
    out("Bench-spike-0.0.2.g3a");

    r = BFile_Ext_Stat(u"\\\\fls0\\BENCHSPK.G3A", &type, &size);
    out_int("stat addin: ", r);
    out_int("size bytes: ", size);
}

static void run_self_size(void)
{
    gint_world_switch(GINT_CALL(test_self_size));
    draw_lines("10 add-in size");
    getkey();
}

/* ---- menu --------------------------------------------------------------- */

static char const *titles[] = {
    "1 file calls (create/write)",
    "2 file calls (read/list/delete)",
    "3 printf with floats",
    "4 font symbols",
    "5 ALPHA letter reading",
    "6 clock: one tick per second",
    "7 heap size",
    "8 drawable size",
    "9 full redraw time",
    "10 empty add-in size",
};

static void run_test(int index)
{
    out_reset();
    switch (index) {
    case 0:
        gint_world_switch(GINT_CALL(files_create_write));
        draw_lines("1 file calls: create/write");
        getkey();
        break;
    case 1:
        gint_world_switch(GINT_CALL(files_read_list_delete));
        draw_lines("2 file calls: read/list/delete");
        getkey();
        break;
    case 2:
        test_printf();
        break;
    case 3:
        test_font();
        break;
    case 4:
        test_alpha();
        break;
    case 5:
        test_clock();
        break;
    case 6:
        test_heap();
        break;
    case 7:
        test_size();
        break;
    case 8:
        test_redraw();
        break;
    case 9:
        run_self_size();
        break;
    default:
        break;
    }
}

static void draw_menu(int selected)
{
    dclear(PAPER);
    drect(0, 0, DWIDTH - 1, 22, ACCENT);
    dtext(8, 4, PAPER, "Bench spike 0.0.2 - UP/DOWN, EXE, EXIT");

    for (int i = 0; i < 10; i++) {
        int y = 30 + i * 18;
        if (i == selected)
            drect(4, y - 2, DWIDTH - 5, y + 14, ACCENT);
        dtext(10, y, i == selected ? PAPER : INK, titles[i]);
    }
    dupdate();
}

int main(void)
{
    int selected = 0;

    while (1) {
        key_event_t ev;

        draw_menu(selected);
        ev = getkey();

        switch (ev.key) {
        case KEY_UP:
            selected = (selected + 9) % 10;
            break;
        case KEY_DOWN:
            selected = (selected + 1) % 10;
            break;
        case KEY_EXE:
            run_test(selected);
            break;
        case KEY_EXIT:
            return 1;
        default:
            break;
        }
    }
}
