/* hwreport.c - see hwreport.h. */

#include "hwreport.h"

#include <string.h>

static char const *const labels[HWREPORT_ITEMS] = {
    "File padding",
    "Write by rename",
    "No float printf",
    "ASCII-only font",
    "ALPHA not letters",
    "rtc_ticks clock",
    "Note and deck caps",
    "Frame time",
};

static char const *const details[HWREPORT_ITEMS] = {
    "1 BFile files are 0xFF padded, so store_read trims trailing 0xFF and NUL",
    "2 A file is written once, then renamed over the target",
    "3 No float printf: every number goes through numfmt",
    "4 The default font covers ASCII only, anything above 0x7E is a ?",
    "5 ALPHA may not map to letters, so text entry uses the on-screen keyboard",
    "6 rtc_ticks() is the clock and unsigned differences handle midnight",
    "7 A 32 KiB note and a 500-card deck fit the heap",
    "8 A frame costs 10-20 ms, so Pacer redraws at 4 Hz",
};

void hwreport_reset(hw_state_t *state)
{
    if (!state)
        return;
    for (int i = 0; i < HWREPORT_ITEMS; i++)
        state[i] = HW_UNKNOWN;
}

char const *hwreport_label(int i)
{
    if (i < 0 || i >= HWREPORT_ITEMS)
        return "";
    return labels[i];
}

char const *hwreport_detail(int i)
{
    if (i < 0 || i >= HWREPORT_ITEMS)
        return "";
    return details[i];
}

char const *hwreport_state_text(hw_state_t s)
{
    switch (s)
    {
    case HW_WORKS:
        return "works";
    case HW_FAILS:
        return "fails";
    default:
        return "not tried";
    }
}

hw_state_t hwreport_next(hw_state_t s)
{
    switch (s)
    {
    case HW_UNKNOWN:
        return HW_WORKS;
    case HW_WORKS:
        return HW_FAILS;
    default:
        return HW_UNKNOWN;
    }
}

int hwreport_marked(hw_state_t const *state)
{
    int n = 0;

    if (!state)
        return 0;
    for (int i = 0; i < HWREPORT_ITEMS; i++)
        if (state[i] != HW_UNKNOWN)
            n++;
    return n;
}

/* The report is built twice: once to measure it, once to write it.  Nothing
 * is written unless the whole thing fits. */
typedef struct {
    char *out;
    int size;
    int at;
    bool too_small;
} writer_t;

static void put(writer_t *w, char const *text)
{
    int n = (int)strlen(text);

    if (w->too_small)
        return;
    if (w->at + n >= w->size)
    {
        w->too_small = true;
        return;
    }
    if (w->out)
        memcpy(w->out + w->at, text, (size_t)n);
    w->at += n;
}

static void build(writer_t *w, char const *version, hw_state_t const *state)
{
    put(w, "Bench hardware report\n");
    put(w, "version ");
    put(w, version && version[0] ? version : "unknown");
    put(w, "\n\n");

    for (int i = 0; i < HWREPORT_ITEMS; i++)
    {
        put(w, details[i]);
        put(w, "\n   ");
        put(w, hwreport_state_text(state[i]));
        put(w, "\n");
    }

    put(w, hwreport_marked(state) == HWREPORT_ITEMS
               ? "\nAll eight marked.\n"
               : "\nSome rows are still not tried.\n");
}

int hwreport_format(char const *version, hw_state_t const *state, char *out,
                    int size)
{
    writer_t w;

    if (!out || size <= 0 || !state)
        return -1;

    w = (writer_t){ NULL, size, 0, false };
    build(&w, version, state);
    if (w.too_small || w.at + 1 > size)
        return -1;

    w = (writer_t){ out, size, 0, false };
    build(&w, version, state);
    out[w.at] = 0;
    return w.at;
}
