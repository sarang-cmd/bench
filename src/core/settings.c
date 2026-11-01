/* settings.c - see settings.h.  Plain C11, no gint, no allocation. */

#include "settings.h"

#include <string.h>

/* ---- small helpers ------------------------------------------------------ */

static bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

static bool key_is(char const *key, int klen, char const *want)
{
    int n = (int)strlen(want);

    return klen == n && memcmp(key, want, (size_t)n) == 0;
}

/* Decimal integer from a byte range.  Returns false when there is no digit. */
static bool parse_int(char const *s, int len, int *out)
{
    int i = 0;
    int sign = 1;
    int v = 0;
    bool any = false;

    while (i < len && is_space(s[i]))
        i++;
    if (i < len && (s[i] == '-' || s[i] == '+'))
    {
        sign = s[i] == '-' ? -1 : 1;
        i++;
    }
    for (; i < len; i++)
    {
        if (s[i] < '0' || s[i] > '9')
            break;
        if (v < 100000)
            v = v * 10 + (s[i] - '0');
        any = true;
    }
    while (i < len && is_space(s[i]))
        i++;
    if (!any || i != len)
        return false;
    *out = sign * v;
    return true;
}

/* ---- ranges ------------------------------------------------------------- */

uint8_t settings_clamp_uncert_sig(int v)
{
    if (v < SETTINGS_UNCERT_SIG_MIN)
        return SETTINGS_UNCERT_SIG_MIN;
    if (v > SETTINGS_UNCERT_SIG_MAX)
        return SETTINGS_UNCERT_SIG_MAX;
    return (uint8_t)v;
}

uint8_t settings_clamp_checkpoint(int v)
{
    if (v < SETTINGS_CHECKPOINT_MIN)
        return SETTINGS_CHECKPOINT_MIN;
    if (v > SETTINGS_CHECKPOINT_MAX)
        return SETTINGS_CHECKPOINT_MAX;
    return (uint8_t)v;
}

uint8_t settings_clamp_wrap(int v)
{
    /* 0 keeps "fit the screen".  Anything else is a percentage width. */
    if (v <= 0)
        return 0;
    if (v < 50)
        return 50;
    if (v > 150)
        return 150;
    return (uint8_t)v;
}

uint8_t settings_clamp_angle(int v)
{
    return v == SETTINGS_ANGLE_RAD ? SETTINGS_ANGLE_RAD : SETTINGS_ANGLE_DEG;
}

uint8_t settings_clamp_startup_cat(int v)
{
    /* Four categories on the softkeys; anything else opens on Study. */
    if (v < 0 || v >= SETTINGS_CATEGORY_COUNT)
        return 0;
    return (uint8_t)v;
}

uint8_t settings_clamp_warn(int v)
{
    if (v < SETTINGS_WARN_MIN)
        return SETTINGS_WARN_MIN;
    if (v > SETTINGS_WARN_MAX)
        return SETTINGS_WARN_MAX;
    return (uint8_t)v;
}

uint8_t settings_clamp_flag(int v)
{
    return v ? 1 : 0;
}

char const *settings_angle_text(uint8_t v)
{
    return v == SETTINGS_ANGLE_RAD ? "radians" : "degrees";
}

char const *settings_warn_text(uint8_t v, char *buf, int size)
{
    int n = 0;

    if (!buf || size < 2)
        return "?";
    if (v == 0)
        return "off";
    if (v >= 10)
        buf[n++] = (char)('0' + v / 10);
    buf[n++] = (char)('0' + v % 10);
    buf[n] = 0;
    if (n + 5 < size)
    {
        buf[n++] = ' ';
        buf[n++] = 'm';
        buf[n++] = 'i';
        buf[n++] = 'n';
        buf[n] = 0;
    }
    return buf;
}

/* ---- defaults, compare -------------------------------------------------- */

void settings_defaults(bench_settings_t *s)
{
    if (!s)
        return;
    s->uncert_sig = 1;
    s->pacer_checkpoint = 10;
    s->notes_wrap = 0;
    s->angle_unit = SETTINGS_ANGLE_DEG;
    s->tour_done = 0;
    s->startup_cat = 0;
    s->show_hint = 1;
    s->pacer_warn = 5;
}

bool settings_equal(bench_settings_t const *a, bench_settings_t const *b)
{
    if (!a || !b)
        return false;
    return a->uncert_sig == b->uncert_sig &&
           a->pacer_checkpoint == b->pacer_checkpoint &&
           a->notes_wrap == b->notes_wrap &&
           a->angle_unit == b->angle_unit &&
           a->tour_done == b->tour_done &&
           a->startup_cat == b->startup_cat &&
           a->show_hint == b->show_hint &&
           a->pacer_warn == b->pacer_warn;
}

/* ---- parse -------------------------------------------------------------- */

int settings_parse(char const *text, int len, bench_settings_t *s, int *ignored)
{
    int i = 0;
    int applied = 0;
    int bad = 0;

    if (ignored)
        *ignored = 0;
    if (!text || len < 0 || !s)
        return -1;

    while (i < len)
    {
        int start = i;
        int end;
        int p;
        int klen;
        int vlen;
        int value;

        while (i < len && text[i] != '\n')
            i++;
        end = i;
        if (i < len)
            i++;            /* step over the newline */

        while (start < end && is_space(text[start]))
            start++;
        while (end > start && is_space(text[end - 1]))
            end--;
        if (start == end || text[start] == '#')
            continue;

        /* Split at the first run of spaces. */
        p = start;
        while (p < end && !is_space(text[p]))
            p++;
        klen = p - start;
        while (p < end && is_space(text[p]))
            p++;
        vlen = end - p;

        if (vlen <= 0 || !parse_int(text + p, vlen, &value))
        {
            bad++;
            continue;
        }

        if (key_is(text + start, klen, "uncert_sig"))
        {
            s->uncert_sig = settings_clamp_uncert_sig(value);
            applied++;
        }
        else if (key_is(text + start, klen, "pacer_checkpoint"))
        {
            s->pacer_checkpoint = settings_clamp_checkpoint(value);
            applied++;
        }
        else if (key_is(text + start, klen, "notes_wrap"))
        {
            s->notes_wrap = settings_clamp_wrap(value);
            applied++;
        }
        else if (key_is(text + start, klen, "angle_unit"))
        {
            s->angle_unit = settings_clamp_angle(value);
            applied++;
        }
        else if (key_is(text + start, klen, "tour_done"))
        {
            s->tour_done = settings_clamp_flag(value);
            applied++;
        }
        else if (key_is(text + start, klen, "startup_cat"))
        {
            s->startup_cat = settings_clamp_startup_cat(value);
            applied++;
        }
        else if (key_is(text + start, klen, "show_hint"))
        {
            s->show_hint = settings_clamp_flag(value);
            applied++;
        }
        else if (key_is(text + start, klen, "pacer_warn"))
        {
            s->pacer_warn = settings_clamp_warn(value);
            applied++;
        }
        else if (key_is(text + start, klen, "version"))
        {
            /* Informational only: a file from a newer build still loads. */
            applied++;
        }
        else
        {
            bad++;
        }
    }

    if (ignored)
        *ignored = bad;
    return applied;
}

/* ---- format ------------------------------------------------------------- */

/* "key 12\n" into buf at n.  Returns the new length, or -1 when it does not
 * fit - the whole file is rejected rather than written half-way. */
static int put_key_int(char *buf, int n, int size, char const *key, int value)
{
    char digits[4];
    int d = 0;
    int kl = 0;

    while (key[kl])
        kl++;
    if (value < 0)
        value = 0;
    if (value == 0)
        digits[d++] = '0';
    while (value > 0 && d < 4)
    {
        digits[d++] = (char)('0' + value % 10);
        value /= 10;
    }
    if (n + kl + 1 + d + 1 >= size)
        return -1;
    for (int i = 0; i < kl; i++)
        buf[n++] = key[i];
    buf[n++] = ' ';
    while (d > 0)
        buf[n++] = digits[--d];
    buf[n++] = '\n';
    return n;
}

int settings_format(bench_settings_t const *s, char *buf, int size)
{
    int n = 0;

    if (!s || !buf || size <= 0)
        return -1;

#define PUT(str)                                                              \
    do {                                                                      \
        int const _l = (int)sizeof(str) - 1;                                  \
        if (n + _l >= size)                                                   \
            return -1;                                                        \
        memcpy(buf + n, (str), (size_t)_l);                                   \
        n += _l;                                                              \
    } while (0)

    PUT("# Bench settings v1\n");
    if (s->uncert_sig == 2)
        PUT("uncert_sig 2\n");
    else
        PUT("uncert_sig 1\n");

    if (s->pacer_checkpoint == 10)
        PUT("pacer_checkpoint 10\n");
    else if (s->pacer_checkpoint == 20)
        PUT("pacer_checkpoint 20\n");
    else if (s->pacer_checkpoint == 5)
        PUT("pacer_checkpoint 5\n");
    else if (s->pacer_checkpoint == 1)
        PUT("pacer_checkpoint 1\n");
    else if (s->pacer_checkpoint == 25)
        PUT("pacer_checkpoint 25\n");
    else if (s->pacer_checkpoint == 50)
        PUT("pacer_checkpoint 50\n");
    else
    {
        char digits[4];
        int v = s->pacer_checkpoint;
        int d = 0;

        if (v == 0)
            digits[d++] = '0';
        while (v > 0 && d < 4)
        {
            digits[d++] = (char)('0' + v % 10);
            v /= 10;
        }
        if (n + 18 + d >= size)
            return -1;
        memcpy(buf + n, "pacer_checkpoint ", 17);
        n += 17;
        while (d > 0)
            buf[n++] = digits[--d];
        buf[n++] = '\n';
    }

    /* Literal by literal: PUT() takes sizeof, and a ?: of two string literals
     * is a pointer, so sizeof would be the width of a pointer. */
    if (s->angle_unit == SETTINGS_ANGLE_RAD)
        PUT("angle_unit 1\n");
    else
        PUT("angle_unit 0\n");
    if (s->tour_done)
        PUT("tour_done 1\n");
    else
        PUT("tour_done 0\n");
    if (s->notes_wrap == 0)
        PUT("notes_wrap 0\n");
    else if (s->notes_wrap == 100)
        PUT("notes_wrap 100\n");
    else if (s->notes_wrap == 50)
        PUT("notes_wrap 50\n");
    else if (s->notes_wrap == 150)
        PUT("notes_wrap 150\n");
    else if (s->notes_wrap == 75)
        PUT("notes_wrap 75\n");
    else
    {
        int v = s->notes_wrap;
        char digits[3];
        int d = 0;

        while (v > 0 && d < 3)
        {
            digits[d++] = (char)('0' + v % 10);
            v /= 10;
        }
        if (n + 11 + d >= size)
            return -1;
        memcpy(buf + n, "notes_wrap ", 11);
        n += 11;
        while (d > 0)
            buf[n++] = digits[--d];
        buf[n++] = '\n';
    }

    /* The three V2 keys go out with a small decimal writer, so a value the
     * literal cases above do not cover still round-trips. */
    n = put_key_int(buf, n, size, "startup_cat", s->startup_cat);
    n = put_key_int(buf, n, size, "show_hint", s->show_hint);
    n = put_key_int(buf, n, size, "pacer_warn", s->pacer_warn);
    if (n < 0)
        return -1;

#undef PUT

    buf[n] = 0;
    return n;
}

char const *settings_uncert_sig_text(uint8_t v)
{
    return v == 2 ? "1 or 2 if digit is 1" : "1 figure";
}
