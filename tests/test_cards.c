/* test_cards.c - cards_parse: every rule in plan 6.3. */

#include <stdio.h>
#include <string.h>

#include "cards_parse.h"
#include "test_util.h"

static deck_t deck;
static cards_issue_t issues[16];
static int nissues = 0;

static int parse(char const *text)
{
    nissues = 0;
    memset(issues, 0, sizeof issues);
    return cards_parse(text, (int)strlen(text), &deck, issues,
                       (int)(sizeof issues / sizeof *issues), &nissues);
}

static void basics(void)
{
    T_SECTION("a normal deck");

    int n = parse("What is the unit of impulse? | N s\n"
                  "Name the force on a falling body | weight\n");
    T_INT(n, 2);
    T_INT(deck.count, 2);
    T_STR(deck.card[0].q, "What is the unit of impulse?");
    T_STR(deck.card[0].a, "N s");
    T_STR(deck.card[1].a, "weight");
    T_INT(deck.card[0].source_line, 1);
    T_INT(deck.card[1].source_line, 2);
    T_INT(nissues, 0);

    /* Spaces around both sides are trimmed. */
    n = parse("   spaced question   |   spaced answer   \n");
    T_INT(n, 1);
    T_STR(deck.card[0].q, "spaced question");
    T_STR(deck.card[0].a, "spaced answer");

    /* Only the first pipe splits: the answer may contain pipes. */
    n = parse("a | b | c\n");
    T_INT(n, 1);
    T_STR(deck.card[0].q, "a");
    T_STR(deck.card[0].a, "b | c");

    /* No trailing newline on the last line. */
    n = parse("last | card");
    T_INT(n, 1);
    T_STR(deck.card[0].a, "card");
}

static void skipping(void)
{
    T_SECTION("blank lines and comments are skipped silently");

    int n = parse("\n"
                  "# a comment\n"
                  "   \t \n"
                  "   # indented comment\n"
                  "real | card\n"
                  "\n");
    T_INT(n, 1);
    T_INT(nissues, 0);
    T_INT(deck.card[0].source_line, 5);
}

static void bad_lines(void)
{
    T_SECTION("bad lines: line number, skip, keep going");

    int n = parse("good one | a\n"
                  "no pipe here\n"
                  " | empty question\n"
                  "empty answer | \n"
                  "good two | b\n");
    T_INT(n, 2);
    T_INT(deck.count, 2);
    T_STR(deck.card[0].q, "good one");
    T_STR(deck.card[1].q, "good two");
    T_INT(nissues, 3);
    T_INT(issues[0].line, 2);
    T_INT(issues[0].kind, CARDS_ISSUE_NO_PIPE);
    T_INT(issues[1].line, 3);
    T_INT(issues[1].kind, CARDS_ISSUE_EMPTY_Q);
    T_INT(issues[2].line, 4);
    T_INT(issues[2].kind, CARDS_ISSUE_EMPTY_A);
    T_STR(cards_issue_text(CARDS_ISSUE_NO_PIPE), "no | on this line");

    /* Only the first line numbers survive a leading comment. */
    n = parse("# header\n"
              "\n"
              "[ ] no pipe\n");
    T_INT(n, 0);
    T_INT(nissues, 1);
    T_INT(issues[0].line, 3);
}

static void long_lines(void)
{
    T_SECTION("120 character limit: warning, truncated, card kept");

    char line[CARDS_SIDE_MAX * 2 + 8];
    char text[CARDS_SIDE_MAX * 4 + 16];

    memset(line, 'q', sizeof line - 1);
    line[sizeof line - 1] = '\0';
    snprintf(text, sizeof text, "%s | short\n", line);
    T_INT(parse(text), 1);
    T_INT(nissues, 1);
    T_INT(issues[0].kind, CARDS_ISSUE_TOO_LONG);
    T_INT((int)strlen(deck.card[0].q), CARDS_SIDE_MAX);

    /* Exactly 120 characters is fine. */
    {
        char q[CARDS_SIDE_MAX + 1];
        memset(q, 'x', CARDS_SIDE_MAX);
        q[CARDS_SIDE_MAX] = '\0';
        snprintf(text, sizeof text, "%s | a\n", q);
        T_INT(parse(text), 1);
        T_INT(nissues, 0);
        T_INT((int)strlen(deck.card[0].q), CARDS_SIDE_MAX);
    }

    /* 121 characters warns and cuts to 120. */
    {
        char q[CARDS_SIDE_MAX + 2];
        memset(q, 'y', CARDS_SIDE_MAX + 1);
        q[CARDS_SIDE_MAX + 1] = '\0';
        snprintf(text, sizeof text, "%s | a\n", q);
        T_INT(parse(text), 1);
        T_INT(nissues, 1);
        T_INT(issues[0].kind, CARDS_ISSUE_TOO_LONG);
        T_INT((int)strlen(deck.card[0].q), CARDS_SIDE_MAX);
    }

    /* A line far longer than the internal buffer still parses safely. */
    {
        size_t n = sizeof text;
        memset(line, 'z', sizeof line - 1);
        line[sizeof line - 1] = '\0';
        snprintf(text, sizeof text, "%.*s | a", (int)(n - 8), line);
        int got = parse(text);
        T_TRUE(got >= 0);
        T_TRUE(nissues >= 1);
        T_TRUE((int)strlen(deck.card[0].q) <= CARDS_SIDE_MAX);
    }
}

static void limits(void)
{
    T_SECTION("500 card limit");

    static char big[CARDS_MAX * 24 + 64];
    static deck_t big_deck;
    size_t off = 0;

    for (int i = 0; i < CARDS_MAX; i++)
        off += (size_t)snprintf(big + off, sizeof big - off, "q%d | a%d\n", i,
                                i);

    int nissues_big = 0;
    int n = cards_parse(big, (int)off, &big_deck, NULL, 0, &nissues_big);
    T_INT(n, CARDS_MAX);
    T_INT(big_deck.count, CARDS_MAX);
    T_INT(nissues_big, 0);
    T_STR(big_deck.card[0].q, "q0");
    T_STR(big_deck.card[CARDS_MAX - 1].q, "q499");

    /* A 501st card is reported and dropped. */
    off += (size_t)snprintf(big + off, sizeof big - off, "q500 | a500\n");
    cards_issue_t one[1];
    int issues_big = 0;
    n = cards_parse(big, (int)off, &big_deck, one, 1, &issues_big);
    T_INT(n, CARDS_MAX);
    T_INT(issues_big, 1);
    T_INT(one[0].kind, CARDS_ISSUE_DECK_FULL);
    T_INT(one[0].line, CARDS_MAX + 1);
}

static void hashes(void)
{
    T_SECTION("question hash: identity that survives an answer edit");

    T_INT(parse("What is g? | 9.81\n"), 1);
    uint32_t id_one = deck.card[0].id;
    T_TRUE(id_one != 0);

    /* Same question, different answer: same id, so the box is kept. */
    T_INT(parse("What is g? | 9.8\n"), 1);
    T_UINT(deck.card[0].id, id_one);

    /* Same question with extra spaces is the same card after trimming. */
    T_INT(parse("  What is g?  | 9.81 \n"), 1);
    T_UINT(deck.card[0].id, id_one);

    /* Edited question: different id, so the card starts again in box 1. */
    T_INT(parse("What is g on Earth? | 9.81\n"), 1);
    T_TRUE(deck.card[0].id != id_one);

    /* The hash is stable across calls and never zero. */
    T_UINT(cards_hash("What is g?"), id_one);
    T_UINT(cards_hash("What is g?"), cards_hash("What is g?"));
    T_TRUE(cards_hash("") != 0);
    T_UINT(cards_hash(NULL), 0);

    /* Different questions almost certainly differ. */
    T_TRUE(cards_hash("a") != cards_hash("b"));
    T_TRUE(cards_hash("abc") != cards_hash("acb"));

    /* Duplicate questions in one deck give the same id, which is intended:
     * they share one box. */
    T_INT(parse("dup | 1\ndup | 2\n"), 2);
    T_UINT(deck.card[0].id, deck.card[1].id);
}

static void edges(void)
{
    T_SECTION("edge cases: empty file, CRLF, BOM, control characters");

    T_INT(parse(""), 0);
    T_INT(deck.count, 0);
    T_INT(nissues, 0);

    /* Windows line endings. */
    T_INT(parse("one | 1\r\ntwo | 2\r\n"), 2);
    T_STR(deck.card[0].a, "1");
    T_STR(deck.card[1].q, "two");

    /* UTF-8 byte order mark at the start of the file. */
    T_INT(parse("\xEF\xBB\xBFone | 1\n"), 1);
    T_STR(deck.card[0].q, "one");

    /* A control character makes the line unusable. */
    T_INT(parse("bad \x01 line | x\ngood | y\n"), 1);
    T_INT(nissues, 1);
    T_INT(issues[0].kind, CARDS_ISSUE_BAD_CHAR);
    T_STR(deck.card[0].q, "good");

    /* UTF-8 text is kept as bytes; the font layer decides how to draw it. */
    T_INT(parse("pi is 3.14 \xC3\xB1 | ok\n"), 1);
    T_INT(nissues, 0);

    /* Bad arguments. */
    T_INT(cards_parse(NULL, 0, &deck, NULL, 0, &nissues), -1);
    T_INT(cards_parse("a | b\n", 6, NULL, NULL, 0, &nissues), -1);
    T_INT(cards_parse("a | b\n", -1, &deck, NULL, 0, &nissues), -1);

    /* An issue array that is too small still reports the total. */
    cards_issue_t tiny_issues[1];
    int total = 0;
    T_INT(cards_parse("x\n\ny\n", 5, &deck, tiny_issues, 1, &total), 0);
    T_INT(total, 2);
    T_INT(tiny_issues[0].line, 1);

    /* Trimming helper. */
    char buf[] = "  hi there \t ";
    T_STR(cards_trim(buf), "hi there");
    T_TRUE(cards_trim(NULL) == NULL);
}

static void run_tests(void)
{
    basics();
    skipping();
    bad_lines();
    long_lines();
    limits();
    hashes();
    edges();
}

T_MAIN("test_cards")
