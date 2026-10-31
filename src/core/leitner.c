/* leitner.c - see leitner.h. */

#include "leitner.h"

#include <string.h>

/* ---- box rules ---------------------------------------------------------- */

void leitner_new(uint8_t *box)
{
    if (box)
        *box = 1;
}

uint8_t leitner_clamp(uint8_t box)
{
    if (box < 1)
        return 1;
    if (box > LEITNER_BOXES)
        return LEITNER_BOXES;
    return box;
}

void leitner_right(uint8_t *box)
{
    uint8_t b;

    if (!box)
        return;
    b = leitner_clamp(*box); /* a corrupted value steps up from 1, not from 0 */
    *box = b < LEITNER_BOXES ? (uint8_t)(b + 1) : LEITNER_BOXES;
}

void leitner_wrong(uint8_t *box)
{
    if (box)
        *box = 1;
}

bool leitner_due(uint8_t box, uint32_t session)
{
    /* Sessions are 0-based: session 0 is the first one ever. */
    switch (leitner_clamp(box)) {
    case 1:
        return true;
    case 2:
        return (session % 2u) == 1u;   /* every second session */
    case 3:
        return (session % 4u) == 3u;   /* every fourth session */
    }
    return false;
}

int leitner_session_build(uint8_t const *boxes, int n, uint32_t session,
                          int *order, int max_order)
{
    int written = 0, i;
    uint8_t box;

    if (!boxes || !order || n < 0 || max_order <= 0)
        return -1;

    for (box = 1; box <= LEITNER_BOXES; box++) {
        if (!leitner_due(box, session))
            continue;
        for (i = 0; i < n && written < max_order; i++) {
            if (leitner_clamp(boxes[i]) == box)
                order[written++] = i;
        }
    }
    return written;
}

/* ---- progress state ----------------------------------------------------- */

void leitner_state_init(leitner_state_t *st)
{
    if (!st)
        return;
    memset(st, 0, sizeof *st);
}

static int find_entry(leitner_state_t const *st, uint32_t id)
{
    int i;
    for (i = 0; i < st->count; i++)
        if (st->entry[i].id == id)
            return i;
    return -1;
}

uint8_t leitner_state_box(leitner_state_t const *st, uint32_t id)
{
    int i;

    if (!st)
        return 1;
    i = find_entry(st, id);
    return i < 0 ? 1 : leitner_clamp(st->entry[i].box);
}

int leitner_state_set(leitner_state_t *st, uint32_t id, uint8_t box)
{
    int i;

    if (!st)
        return -1;
    i = find_entry(st, id);
    if (i >= 0) {
        st->entry[i].box = leitner_clamp(box);
        return 0;
    }
    if (st->count >= LEITNER_MAX_ENTRIES)
        return -1;
    st->entry[st->count].id = id;
    st->entry[st->count].box = leitner_clamp(box);
    st->count++;
    return 0;
}

int leitner_apply_result(leitner_state_t *st, uint32_t id, bool right)
{
    uint8_t box;

    if (!st)
        return -1;
    box = leitner_state_box(st, id);
    if (right)
        leitner_right(&box);
    else
        leitner_wrong(&box);
    return leitner_state_set(st, id, box);
}

void leitner_end_session(leitner_state_t *st)
{
    if (st)
        st->sessions++;
}

/* ---- parsing and formatting --------------------------------------------- */

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_blank(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

/* Parses an unsigned decimal.  Returns the number of characters consumed, or 0
 * when there is no number.  Sets *overflow when the value does not fit. */
static int parse_u32(char const *s, uint32_t *out, bool *overflow)
{
    uint32_t v = 0;
    int n = 0;

    *overflow = false;
    while (is_digit(s[n])) {
        uint32_t d = (uint32_t)(s[n] - '0');
        if (v > (0xFFFFFFFFu - d) / 10u) {
            *overflow = true;
            /* Keep consuming digits so the caller can skip the token. */
            while (is_digit(s[n]))
                n++;
            return n;
        }
        v = v * 10u + d;
        n++;
    }
    *out = v;
    return n;
}

int leitner_state_parse(char const *text, int len, leitner_state_t *st)
{
    int pos = 0, loaded = 0;

    if (!text || len < 0 || !st)
        return -1;

    leitner_state_init(st);

    while (pos < len) {
        int start = pos, end;
        char line[64];
        int copy_len, n;
        uint32_t value = 0;
        bool overflow = false;

        while (pos < len && text[pos] != '\n')
            pos++;
        end = pos;
        if (pos < len && text[pos] == '\n')
            pos++;
        if (end > start && text[end - 1] == '\r')
            end--;

        copy_len = end - start;
        if (copy_len > (int)sizeof line - 1)
            copy_len = (int)sizeof line - 1;
        memcpy(line, text + start, (size_t)copy_len);
        line[copy_len] = '\0';

        /* Skip leading blanks. */
        {
            char *p = line;
            while (is_blank(*p))
                p++;

            if (*p == '\0' || *p == '#')
                continue;

            if (strncmp(p, "session", 7) == 0 &&
                (p[7] == ' ' || p[7] == '\t')) {
                char *num = p + 7;
                while (is_blank(*num))
                    num++;
                n = parse_u32(num, &value, &overflow);
                if (n == 0 || overflow) {
                    st->bad_lines++;
                    continue;
                }
                st->sessions = value;
                continue;
            }

            /* "<id> <box>" */
            n = parse_u32(p, &value, &overflow);
            if (n == 0 || overflow) {
                st->bad_lines++;
                continue;
            }
            {
                char *after = p + n;
                uint32_t box = 0;
                bool box_overflow = false;
                int m;

                while (is_blank(*after))
                    after++;
                m = parse_u32(after, &box, &box_overflow);
                if (m == 0 || box_overflow || box < 1 ||
                    box > LEITNER_BOXES) {
                    st->bad_lines++;
                    continue;
                }
                if (leitner_state_set(st, value, (uint8_t)box) != 0) {
                    st->bad_lines++;
                    continue;
                }
                loaded++;
            }
        }
    }
    return loaded;
}

/* Writes an unsigned decimal without printf.  Returns characters written, or
 * -1 when the buffer is too small. */
static int put_u32(char *buf, int size, uint32_t v)
{
    char tmp[12];
    int n = 0, i, written = 0;

    do {
        tmp[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    } while (v);

    if (size < n)
        return -1;
    for (i = n - 1; i >= 0; i--)
        buf[written++] = tmp[i];
    return written;
}

static int put_str(char *buf, int size, char const *s)
{
    int n = (int)strlen(s);

    if (size < n)
        return -1;
    memcpy(buf, s, (size_t)n);
    return n;
}

int leitner_state_format(leitner_state_t const *st, char *buf, int size)
{
    int written = 0, i, n;

    if (!st || !buf || size <= 0)
        return -1;

    n = put_str(buf, size, "# Bench cards state v");
    if (n < 0)
        return -1;
    written += n;

    n = put_u32(buf + written, size - written, LEITNER_STATE_VERSION);
    if (n < 0)
        return -1;
    written += n;

    n = put_str(buf + written, size - written, "\nsession ");
    if (n < 0)
        return -1;
    written += n;

    n = put_u32(buf + written, size - written, st->sessions);
    if (n < 0)
        return -1;
    written += n;

    n = put_str(buf + written, size - written, "\n");
    if (n < 0)
        return -1;
    written += n;

    for (i = 0; i < st->count; i++) {
        n = put_u32(buf + written, size - written, st->entry[i].id);
        if (n < 0)
            return -1;
        written += n;
        n = put_str(buf + written, size - written, " ");
        if (n < 0)
            return -1;
        written += n;
        n = put_u32(buf + written, size - written, st->entry[i].box);
        if (n < 0)
            return -1;
        written += n;
        n = put_str(buf + written, size - written, "\n");
        if (n < 0)
            return -1;
        written += n;
    }
    buf[written] = '\0';
    return written;
}
