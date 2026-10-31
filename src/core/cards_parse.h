/* cards_parse.h - parse "question | answer" deck files.
 *
 * Spec: docs/plan.md section 6.3.
 *   - one card per line, split at the first pipe, trimmed
 *   - blank lines and lines starting with '#' are skipped
 *   - at most CARDS_MAX cards per deck, at most CARDS_SIDE_MAX characters a side
 *   - a line that cannot be used is reported with its line number and skipped
 *   - a side that is too long is truncated and reported as a warning
 *
 * Plain C11, no gint, no allocation. */

#ifndef BENCH_CARDS_PARSE_H
#define BENCH_CARDS_PARSE_H

#include <stdbool.h>
#include <stdint.h>

#define CARDS_MAX 500
#define CARDS_SIDE_MAX 120

typedef struct {
    char q[CARDS_SIDE_MAX + 1];
    char a[CARDS_SIDE_MAX + 1];
    uint32_t id;             /* hash of the question, stable across sessions */
    uint16_t source_line;    /* 1-based line in the file, for messages */
} card_t;

typedef struct {
    card_t card[CARDS_MAX];
    int count;
} deck_t;

typedef enum {
    CARDS_ISSUE_NONE = 0,
    CARDS_ISSUE_NO_PIPE,      /* the line has no '|' */
    CARDS_ISSUE_EMPTY_Q,      /* nothing before the pipe */
    CARDS_ISSUE_EMPTY_A,      /* nothing after the pipe */
    CARDS_ISSUE_TOO_LONG,     /* a side was longer than CARDS_SIDE_MAX */
    CARDS_ISSUE_DECK_FULL,    /* more than CARDS_MAX cards: the rest is dropped */
    CARDS_ISSUE_BAD_CHAR,     /* non-printable byte in the line */
} cards_issue_kind_t;

typedef struct {
    int line;                 /* 1-based line number */
    cards_issue_kind_t kind;
} cards_issue_t;

/* Parse `len` bytes of `text` into `deck`.
 * Issues are appended to `issues` (up to `max_issues`); `*nissues` always gets
 * the total number of issues found, even when they do not all fit.
 * Returns the number of cards loaded (>= 0) or -1 on a bad argument. */
int cards_parse(char const *text, int len, deck_t *deck, cards_issue_t *issues,
                int max_issues, int *nissues);

/* Short text for an issue kind, for the deck screen footer.  Never NULL. */
char const *cards_issue_text(cards_issue_kind_t kind);

/* FNV-1a 32-bit hash of the question text.  Card identity is the question, so
 * editing an answer keeps the box and editing a question resets it. */
uint32_t cards_hash(char const *text);

/* Trim leading and trailing spaces/tabs in place.  Returns the new start. */
char *cards_trim(char *s);

#endif /* BENCH_CARDS_PARSE_H */
