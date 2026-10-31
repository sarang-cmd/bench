/* cards_parse.c - see cards_parse.h. */

#include "cards_parse.h"

#include <string.h>

static bool is_space(char c)
{
    return c == ' ' || c == '\t';
}

static bool is_printable_ascii(char c)
{
    unsigned char u = (unsigned char)c;
    /* Tabs are handled as separators; the rest must be plain printable ASCII
     * or UTF-8 continuation bytes (>= 0x80), which the font layer maps. */
    return (u >= 0x20 && u <= 0x7E) || u >= 0x80 || c == '\t';
}

char *cards_trim(char *s)
{
    char *end;

    if (!s)
        return NULL;
    while (is_space(*s))
        s++;
    end = s + strlen(s);
    while (end > s && is_space(end[-1]))
        *--end = '\0';
    return s;
}

uint32_t cards_hash(char const *text)
{
    uint32_t h = 2166136261u;

    if (!text)
        return 0;
    while (*text) {
        h ^= (uint32_t)(unsigned char)*text++;
        h *= 16777619u;
    }
    /* 0 is reserved for "no progress entry yet". */
    return h == 0 ? 1u : h;
}

char const *cards_issue_text(cards_issue_kind_t kind)
{
    switch (kind) {
    case CARDS_ISSUE_NONE:
        return "ok";
    case CARDS_ISSUE_NO_PIPE:
        return "no | on this line";
    case CARDS_ISSUE_EMPTY_Q:
        return "empty question";
    case CARDS_ISSUE_EMPTY_A:
        return "empty answer";
    case CARDS_ISSUE_TOO_LONG:
        return "line too long, cut";
    case CARDS_ISSUE_DECK_FULL:
        return "deck full, rest skipped";
    case CARDS_ISSUE_BAD_CHAR:
        return "odd character, skipped";
    }
    return "problem";
}

static void add_issue(cards_issue_t *issues, int max_issues, int *nissues,
                      int line, cards_issue_kind_t kind)
{
    if (issues && *nissues < max_issues)
        issues[*nissues] = (cards_issue_t){ line, kind };
    (*nissues)++;
}

/* Copies a range into a fixed buffer, truncating.  Returns true when it had to
 * cut characters off. */
static bool copy_side(char *dst, char const *src, int len)
{
    bool cut = len > CARDS_SIDE_MAX;

    if (cut)
        len = CARDS_SIDE_MAX;
    memcpy(dst, src, (size_t)len);
    dst[len] = '\0';
    /* Trailing spaces left over from the cut are not worth keeping. */
    while (len > 0 && is_space(dst[len - 1]))
        dst[--len] = '\0';
    return cut;
}

/* Trims [*start, *end) in place (blanks only) and returns the new length. */
static int trim_range(char const *text, int *start, int *end)
{
    while (*start < *end && is_space(text[*start]))
        (*start)++;
    while (*end > *start && is_space(text[*end - 1]))
        (*end)--;
    return *end - *start;
}

int cards_parse(char const *text, int len, deck_t *deck, cards_issue_t *issues,
                int max_issues, int *nissues)
{
    int pos = 0, line_no = 0, loaded = 0, issues_found = 0;

    if (!text || len < 0 || !deck)
        return -1;

    deck->count = 0;
    if (nissues)
        *nissues = 0;
    if (max_issues < 0)
        max_issues = 0;

    while (pos < len) {
        int start = pos, end, q_start, q_end, a_start, a_end, pipe = -1;

        while (pos < len && text[pos] != '\n')
            pos++;
        end = pos;
        if (pos < len && text[pos] == '\n')
            pos++;

        line_no++;

        /* Drop a trailing CR so Windows line endings parse. */
        if (end > start && text[end - 1] == '\r')
            end--;
        /* Strip a UTF-8 byte order mark on the very first line. */
        if (line_no == 1 && end - start >= 3 &&
            (unsigned char)text[start] == 0xEF &&
            (unsigned char)text[start + 1] == 0xBB &&
            (unsigned char)text[start + 2] == 0xBF)
            start += 3;

        if (trim_range(text, &start, &end) == 0)
            continue;
        if (text[start] == '#')
            continue;

        /* Reject control characters early: they would render as blanks on the
         * calculator and hide a broken line. */
        {
            bool bad = false;
            for (int i = start; i < end; i++) {
                if (!is_printable_ascii(text[i])) {
                    bad = true;
                    break;
                }
            }
            if (bad) {
                add_issue(issues, max_issues, &issues_found, line_no,
                          CARDS_ISSUE_BAD_CHAR);
                continue;
            }
        }

        for (int i = start; i < end; i++) {
            if (text[i] == '|') {
                pipe = i;
                break;
            }
        }
        if (pipe < 0) {
            add_issue(issues, max_issues, &issues_found, line_no,
                      CARDS_ISSUE_NO_PIPE);
            continue;
        }

        q_start = start;
        q_end = pipe;
        a_start = pipe + 1;
        a_end = end;
        (void)trim_range(text, &q_start, &q_end);
        (void)trim_range(text, &a_start, &a_end);

        if (q_end == q_start) {
            add_issue(issues, max_issues, &issues_found, line_no,
                      CARDS_ISSUE_EMPTY_Q);
            continue;
        }
        if (a_end == a_start) {
            add_issue(issues, max_issues, &issues_found, line_no,
                      CARDS_ISSUE_EMPTY_A);
            continue;
        }
        if (deck->count >= CARDS_MAX) {
            add_issue(issues, max_issues, &issues_found, line_no,
                      CARDS_ISSUE_DECK_FULL);
            continue;
        }

        {
            card_t *card = &deck->card[deck->count];
            bool q_cut = copy_side(card->q, text + q_start, q_end - q_start);
            bool a_cut = copy_side(card->a, text + a_start, a_end - a_start);

            if (q_cut || a_cut)
                add_issue(issues, max_issues, &issues_found, line_no,
                          CARDS_ISSUE_TOO_LONG);

            card->id = cards_hash(card->q);
            card->source_line =
                (uint16_t)(line_no > 65535 ? 65535 : line_no);
            deck->count++;
            loaded++;
        }
    }

    if (nissues)
        *nissues = issues_found;
    return loaded;
}
