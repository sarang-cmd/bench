/* leitner.h - three-box Leitner schedule and its progress file.
 *
 * Spec: docs/plan.md section 6.3.
 *   - new cards start in box 1
 *   - right answer: box + 1, capped at 3
 *   - wrong answer: back to box 1
 *   - a session draws box 1 always, box 2 every second session, box 3 every
 *     fourth session
 *
 * Card identity is the hash of the question text (cards_parse.h), so editing an
 * answer keeps the box and editing a question resets the card to box 1.
 *
 * The progress file is plain text:
 *     # Bench cards state v1
 *     session 7
 *     3141592653 2
 * Plain C11, no gint, no allocation. */

#ifndef BENCH_LEITNER_H
#define BENCH_LEITNER_H

#include <stdbool.h>
#include <stdint.h>

#define LEITNER_BOXES 3
#define LEITNER_MAX_ENTRIES 2000
#define LEITNER_STATE_VERSION 1

/* ---- box rules ---------------------------------------------------------- */

/* Sets *box to 1.  Safe with a NULL pointer. */
void leitner_new(uint8_t *box);

/* Right answer: one box further, never past box 3. */
void leitner_right(uint8_t *box);

/* Wrong answer: back to box 1. */
void leitner_wrong(uint8_t *box);

/* Clamp an arbitrary value into 1..3 (a corrupted state file, say). */
uint8_t leitner_clamp(uint8_t box);

/* Is a card in `box` drawn in `session` (0-based)? */
bool leitner_due(uint8_t box, uint32_t session);

/* Fill `order` with the indices of the cards to draw this session, box 1
 * first, then box 2 when due, then box 3 when due.  Order inside a box follows
 * the deck order.  Returns the number of entries written, which is at most
 * `max_order`. */
int leitner_session_build(uint8_t const *boxes, int n, uint32_t session,
                          int *order, int max_order);

/* Shuffles `order[0..n)` in place, deterministically for a given seed: the same
 * seed is the same session order, so a test can pin it and two runs on the same
 * day agree.  Cards v2 uses this for the "mixed" session. */
void leitner_shuffle(int *order, int n, uint32_t seed);

/* ---- progress state ----------------------------------------------------- */

typedef struct {
    uint32_t id;
    uint8_t box;
} leitner_entry_t;

typedef struct {
    leitner_entry_t entry[LEITNER_MAX_ENTRIES];
    int count;
    uint32_t sessions;       /* sessions completed so far */
    int bad_lines;           /* lines skipped while parsing */
} leitner_state_t;

void leitner_state_init(leitner_state_t *st);

/* Parse a progress file.  Never fails on content: bad lines are counted in
 * st->bad_lines and skipped.  Returns the number of entries loaded, or -1 on a
 * bad argument. */
int leitner_state_parse(char const *text, int len, leitner_state_t *st);

/* Serialise to text.  Returns the length written, or -1 if `size` is too
 * small.  The output always ends with a newline. */
int leitner_state_format(leitner_state_t const *st, char *buf, int size);

/* Box of a card, or 1 when the card has no entry yet. */
uint8_t leitner_state_box(leitner_state_t const *st, uint32_t id);

/* Set the box of a card, adding an entry when needed.
 * Returns 0 on success, -1 when the table is full or the argument is bad. */
int leitner_state_set(leitner_state_t *st, uint32_t id, uint8_t box);

/* Apply one answer to a card: right moves it up, wrong resets it. */
int leitner_apply_result(leitner_state_t *st, uint32_t id, bool right);

/* Called once when a session ends: bumps the session counter, which drives the
 * every-second / every-fourth-session rules. */
void leitner_end_session(leitner_state_t *st);

#endif /* BENCH_LEITNER_H */
