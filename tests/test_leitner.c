/* test_leitner.c - box moves, session draw rules, progress file, identity. */

#include <string.h>

#include "cards_parse.h"
#include "leitner.h"
#include "test_util.h"

static void box_moves(void)
{
    T_SECTION("new / right / wrong");

    uint8_t box = 0;
    leitner_new(&box);
    T_INT(box, 1);
    leitner_new(NULL); /* no crash */

    /* Right answers walk up one box at a time, capped at 3. */
    leitner_right(&box);
    T_INT(box, 2);
    leitner_right(&box);
    T_INT(box, 3);
    leitner_right(&box);
    T_INT(box, 3);

    /* A wrong answer resets to box 1. */
    leitner_wrong(&box);
    T_INT(box, 1);
    leitner_wrong(&box);
    T_INT(box, 1);
    leitner_wrong(NULL);

    /* Corrupted values are clamped, not trusted. */
    T_INT(leitner_clamp(0), 1);
    T_INT(leitner_clamp(4), 3);
    T_INT(leitner_clamp(2), 2);
    box = 0;
    leitner_right(&box);
    T_INT(box, 2);
    box = 200;
    leitner_right(&box);
    T_INT(box, 3);
}

static void due_rules(void)
{
    T_SECTION("which boxes a session draws");

    /* Box 1 every session. */
    for (uint32_t s = 0; s < 8; s++)
        T_TRUE(leitner_due(1, s));

    /* Box 2 every second session: 1, 3, 5, 7. */
    T_FALSE(leitner_due(2, 0));
    T_TRUE(leitner_due(2, 1));
    T_FALSE(leitner_due(2, 2));
    T_TRUE(leitner_due(2, 3));
    T_FALSE(leitner_due(2, 4));
    T_TRUE(leitner_due(2, 5));

    /* Box 3 every fourth session: 3, 7, 11. */
    T_FALSE(leitner_due(3, 0));
    T_FALSE(leitner_due(3, 1));
    T_FALSE(leitner_due(3, 2));
    T_TRUE(leitner_due(3, 3));
    T_FALSE(leitner_due(3, 4));
    T_FALSE(leitner_due(3, 5));
    T_FALSE(leitner_due(3, 6));
    T_TRUE(leitner_due(3, 7));

    /* Out-of-range boxes are clamped, so they behave like 1..3. */
    T_TRUE(leitner_due(0, 0));
    T_TRUE(leitner_due(9, 3));
}

static void session_build(void)
{
    T_SECTION("session order: box 1, then box 2, then box 3");

    /*  cards:  0=b1 1=b2 2=b3 3=b1 4=b2 5=b3 */
    uint8_t boxes[6] = { 1, 2, 3, 1, 2, 3 };
    int order[8];
    int n;

    /* Session 0: box 1 only, in deck order. */
    n = leitner_session_build(boxes, 6, 0, order, 8);
    T_INT(n, 2);
    T_INT(order[0], 0);
    T_INT(order[1], 3);

    /* Session 1: box 1 then box 2. */
    n = leitner_session_build(boxes, 6, 1, order, 8);
    T_INT(n, 4);
    T_INT(order[0], 0);
    T_INT(order[1], 3);
    T_INT(order[2], 1);
    T_INT(order[3], 4);

    /* Session 3: all three boxes. */
    n = leitner_session_build(boxes, 6, 3, order, 8);
    T_INT(n, 6);
    T_INT(order[0], 0);
    T_INT(order[1], 3);
    T_INT(order[2], 1);
    T_INT(order[3], 4);
    T_INT(order[4], 2);
    T_INT(order[5], 5);

    /* A session list that is too small is cut, not overrun. */
    n = leitner_session_build(boxes, 6, 3, order, 3);
    T_INT(n, 3);
    T_INT(order[2], 1);

    /* Empty deck. */
    T_INT(leitner_session_build(boxes, 0, 3, order, 8), 0);

    /* Bad arguments. */
    T_INT(leitner_session_build(NULL, 6, 0, order, 8), -1);
    T_INT(leitner_session_build(boxes, 6, 0, NULL, 8), -1);
    T_INT(leitner_session_build(boxes, 6, 0, order, 0), -1);
}

static void state_rules(void)
{
    T_SECTION("progress state: boxes by card id");

    leitner_state_t st;
    leitner_state_init(&st);
    T_INT(st.count, 0);
    T_INT(st.sessions, 0);

    /* An unknown card is in box 1. */
    T_INT(leitner_state_box(&st, 12345u), 1);

    T_INT(leitner_state_set(&st, 12345u, 1), 0);
    T_INT(st.count, 1);
    T_INT(leitner_state_box(&st, 12345u), 1);

    /* A right answer moves it up, a wrong answer resets it. */
    T_INT(leitner_apply_result(&st, 12345u, true), 0);
    T_INT(leitner_state_box(&st, 12345u), 2);
    T_INT(leitner_apply_result(&st, 12345u, true), 0);
    T_INT(leitner_state_box(&st, 12345u), 3);
    T_INT(leitner_apply_result(&st, 12345u, true), 0);
    T_INT(leitner_state_box(&st, 12345u), 3); /* capped */
    T_INT(leitner_apply_result(&st, 12345u, false), 0);
    T_INT(leitner_state_box(&st, 12345u), 1);

    /* A brand new id appears on the first result. */
    T_INT(leitner_apply_result(&st, 777u, true), 0);
    T_INT(leitner_state_box(&st, 777u), 2);
    T_INT(st.count, 2);

    /* Setting an out-of-range box clamps it. */
    T_INT(leitner_state_set(&st, 777u, 99), 0);
    T_INT(leitner_state_box(&st, 777u), 3);
    T_INT(leitner_state_set(&st, 777u, 0), 0);
    T_INT(leitner_state_box(&st, 777u), 1);

    /* Session counter drives the draw rules. */
    leitner_end_session(&st);
    T_INT(st.sessions, 1);
    leitner_end_session(&st);
    T_INT(st.sessions, 2);
    leitner_end_session(NULL);

    /* Bad arguments. */
    T_INT(leitner_state_set(NULL, 1u, 1), -1);
    T_INT(leitner_apply_result(NULL, 1u, true), -1);
    T_INT(leitner_state_box(NULL, 1u), 1);
}

static void state_file(void)
{
    T_SECTION("progress file: write, read back, relaunch");

    leitner_state_t st, back;
    static char text[4096];
    int len;

    leitner_state_init(&st);
    (void)leitner_state_set(&st, 111u, 1);
    (void)leitner_state_set(&st, 222u, 2);
    (void)leitner_state_set(&st, 333u, 3);
    st.sessions = 7;

    len = leitner_state_format(&st, text, (int)sizeof text);
    T_TRUE(len > 0);
    T_STR(text, "# Bench cards state v1\nsession 7\n111 1\n222 2\n333 3\n");
    T_TRUE(text[len] == '\0');

    /* Reading it back gives the same boxes: this is "quit and relaunch". */
    T_INT(leitner_state_parse(text, len, &back), 3);
    T_INT(back.count, 3);
    T_INT(back.sessions, 7);
    T_INT(leitner_state_box(&back, 111u), 1);
    T_INT(leitner_state_box(&back, 222u), 2);
    T_INT(leitner_state_box(&back, 333u), 3);
    T_INT(back.bad_lines, 0);

    /* A buffer that is too small fails instead of truncating the state. */
    char small[16];
    T_INT(leitner_state_format(&st, small, (int)sizeof small), -1);
    T_INT(leitner_state_format(&st, small, 0), -1);
    T_INT(leitner_state_format(NULL, text, (int)sizeof text), -1);

    /* Corrupt and partial files are tolerated. */
    char const *messy = "# v1\n"
                        "session x\n"          /* bad session line */
                        "111 2\n"
                        "not a number\n"       /* bad entry */
                        "222 9\n"              /* box out of range */
                        "333\n"                /* missing box */
                        "444 0\n"              /* box out of range */
                        "\n"
                        "# comment\n"
                        "555 3\n";
    T_INT(leitner_state_parse(messy, (int)strlen(messy), &back), 2);
    T_INT(back.count, 2);
    T_INT(back.bad_lines, 5);
    T_INT(leitner_state_box(&back, 111u), 2);
    T_INT(leitner_state_box(&back, 555u), 3);
    T_INT(back.sessions, 0);

    /* Numbers that overflow 32 bits are bad lines, not wraps. */
    char const *huge = "4294967296 1\n";
    T_INT(leitner_state_parse(huge, (int)strlen(huge), &back), 0);
    T_INT(back.bad_lines, 1);

    /* An empty file is an empty state. */
    T_INT(leitner_state_parse("", 0, &back), 0);
    T_INT(back.count, 0);
    T_INT(back.sessions, 0);
    T_INT(back.bad_lines, 0);

    T_INT(leitner_state_parse(NULL, 0, &back), -1);
    T_INT(leitner_state_parse("x", 1, NULL), -1);
}

static void deck_identity(void)
{
    T_SECTION("identity: answer edits keep a box, question edits reset it");

    static char text[512];
    deck_t deck_a, deck_b;
    leitner_state_t st;
    int issues = 0;

    leitner_state_init(&st);

    /* Session one: learn the card, get it right -> box 2. */
    (void)cards_parse("What is g? | 9.81\n", 19, &deck_a, NULL, 0, &issues);
    T_INT(deck_a.count, 1);
    T_INT(leitner_apply_result(&st, deck_a.card[0].id, true), 0);
    T_INT(leitner_state_box(&st, deck_a.card[0].id), 2);

    /* The user edits the answer only: same question hash, box still 2. */
    char const *edited_answer = "What is g? | 9.8 m/s^2\n";
    (void)cards_parse(edited_answer, (int)strlen(edited_answer), &deck_b, NULL,
                      0, &issues);
    T_UINT(deck_b.card[0].id, deck_a.card[0].id);
    T_INT(leitner_state_box(&st, deck_b.card[0].id), 2);

    /* The user edits the question: new hash, so the card is new (box 1). */
    char const *edited_question = "What is g on Earth? | 9.81\n";
    (void)cards_parse(edited_question, (int)strlen(edited_question), &deck_b,
                      NULL, 0, &issues);
    T_TRUE(deck_b.card[0].id != deck_a.card[0].id);
    T_INT(leitner_state_box(&st, deck_b.card[0].id), 1);

    /* Progress entries for cards no longer in the deck are harmless. */
    (void)leitner_state_set(&st, 424242u, 3);
    T_INT(leitner_state_box(&st, 424242u), 3);
    (void)snprintf(text, sizeof text, "x");
}

static void five_hundred_cards(void)
{
    T_SECTION("a 500 card deck runs through a whole session");

    static char text[CARDS_MAX * 24 + 64];
    static deck_t deck;
    static uint8_t boxes[CARDS_MAX];
    static int order[CARDS_MAX];
    size_t off = 0;
    int issues = 0, n, i;
    leitner_state_t st;

    for (i = 0; i < CARDS_MAX; i++)
        off += (size_t)snprintf(text + off, sizeof text - off, "q%d | a%d\n", i,
                                i);

    n = cards_parse(text, (int)off, &deck, NULL, 0, &issues);
    T_INT(n, CARDS_MAX);
    T_INT(issues, 0);

    leitner_state_init(&st);
    for (i = 0; i < deck.count; i++) {
        uint8_t box = leitner_state_box(&st, deck.card[i].id);
        boxes[i] = box;
    }

    /* First session draws every card once (all are new, box 1). */
    n = leitner_session_build(boxes, deck.count, st.sessions, order,
                              CARDS_MAX);
    T_INT(n, CARDS_MAX);

    /* Answer them all right, end the session; session 1 draws box 2 too. */
    for (i = 0; i < deck.count; i++)
        (void)leitner_apply_result(&st, deck.card[i].id, true);
    leitner_end_session(&st);

    for (i = 0; i < deck.count; i++)
        boxes[i] = leitner_state_box(&st, deck.card[i].id);
    n = leitner_session_build(boxes, deck.count, st.sessions, order,
                              CARDS_MAX);
    T_INT(n, CARDS_MAX);
    T_INT(leitner_state_box(&st, deck.card[0].id), 2);

    /* Now the state file holds 500 entries and reads back identically. */
    static char saved[LEITNER_MAX_ENTRIES * 16 + 64];
    leitner_state_t back;
    int len = leitner_state_format(&st, saved, (int)sizeof saved);
    T_TRUE(len > 0);
    T_INT(leitner_state_parse(saved, len, &back), CARDS_MAX);
    T_INT(back.sessions, 1);
    T_INT(leitner_state_box(&back, deck.card[499].id), 2);
}

static void run_tests(void)
{
    box_moves();
    due_rules();
    session_build();
    state_rules();
    state_file();
    deck_identity();
    five_hundred_cards();
}

T_MAIN("test_leitner")
