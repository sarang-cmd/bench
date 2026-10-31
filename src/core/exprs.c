/* exprs.c - see exprs.h. */

#include "exprs.h"

#include <string.h>

static char const *const slot_names[EXPR_SLOTS] = { "f", "g", "h", "p", "q", "r" };

char const *exprs_name(int slot)
{
    if (slot < 0 || slot >= EXPR_SLOTS)
        return NULL;
    return slot_names[slot];
}

/* The slot for a name, or -1. */
static int slot_of(char const *name, int len)
{
    for (int i = 0; i < EXPR_SLOTS; i++)
        if (len == 1 && name[0] == slot_names[i][0])
            return i;
    return -1;
}

int exprs_set(expr_slot_t slots[EXPR_SLOTS], int slot, char const *src)
{
    int n = 0;

    if (slot < 0 || slot >= EXPR_SLOTS)
        return 0;
    if (!src)
        src = "";
    while (src[n] && n < EXPR_TEXT_MAX - 1)
    {
        slots[slot].text[n] = src[n];
        n++;
    }
    slots[slot].text[n] = 0;
    return n;
}

int exprs_parse(char const *text, expr_slot_t slots[EXPR_SLOTS])
{
    int accepted = 0;
    int line = 0;

    for (int i = 0; i < EXPR_SLOTS; i++)
        slots[i].text[0] = 0;
    if (!text)
        return 0;

    while (*text && line < 4096)
    {
        char const *start = text;
        int len = 0;
        int eq = -1;

        while (start[len] && start[len] != '\n' && start[len] != '\r')
            len++;
        for (int i = 0; i < len; i++)
            if (start[i] == '=')
            {
                eq = i;
                break;
            }
        /* "name=text": one letter, then '=', then the expression. */
        if (eq > 0)
        {
            int const slot = slot_of(start, eq);

            if (slot >= 0)
            {
                char buf[EXPR_TEXT_MAX];
                int n = len - eq - 1;

                if (n > EXPR_TEXT_MAX - 1)
                    n = EXPR_TEXT_MAX - 1;
                for (int i = 0; i < n; i++)
                    buf[i] = start[eq + 1 + i];
                buf[n] = 0;
                exprs_set(slots, slot, buf);
                accepted++;
            }
        }
        text = start + len;
        while (*text == '\r' || *text == '\n')
            text++;
        line++;
    }
    return accepted;
}

int exprs_format(expr_slot_t const slots[EXPR_SLOTS], char *out, int size)
{
    int n = 0;

    if (!out || size <= 0)
        return -1;
    out[0] = 0;
    for (int i = 0; i < EXPR_SLOTS; i++)
    {
        char const *name = slot_names[i];
        int const need = 2 + (int)strlen(slots[i].text) + 1;

        if (n + need >= size)
            return -1;
        out[n++] = name[0];
        out[n++] = '=';
        for (char const *p = slots[i].text; *p; p++)
            out[n++] = *p;
        out[n++] = '\n';
        out[n] = 0;
    }
    return n;
}

int exprs_same(expr_slot_t const a[EXPR_SLOTS], expr_slot_t const b[EXPR_SLOTS])
{
    for (int i = 0; i < EXPR_SLOTS; i++)
        if (strcmp(a[i].text, b[i].text) != 0)
            return 0;
    return 1;
}
