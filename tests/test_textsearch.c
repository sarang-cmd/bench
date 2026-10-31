/* test_textsearch.c - case-insensitive search and line helpers (plan 6.4). */

#include <string.h>

#include "test_util.h"
#include "textsearch.h"

/* Small helper so a test reads like the call it is checking. */
static int find(char const *hay, char const *needle)
{
    return textsearch_find(hay, (int)strlen(hay), 0, needle,
                           (int)strlen(needle));
}

static void case_folding(void)
{
    T_SECTION("ASCII case folding");

    T_INT(textsearch_lower('A'), 'a');
    T_INT(textsearch_lower('Z'), 'z');
    T_INT(textsearch_lower('a'), 'a');
    T_INT(textsearch_lower('7'), '7');
    T_INT(textsearch_lower('.'), '.');
    T_INT((unsigned char)textsearch_lower((char)0xC3),
          (unsigned char)0xC3); /* high bytes are untouched */

    /* Mixed case in both directions. */
    T_INT(find("Hello World", "world"), 6);
    T_INT(find("Hello World", "WORLD"), 6);
    T_INT(find("hello world", "Hello"), 0);
    T_INT(find("The QUICK brown FOX", "quick"), 4);
}

static void find_rules(void)
{
    T_SECTION("find: offsets, misses and edge cases");

    char const *text = "one two three two one";
    int len = (int)strlen(text);

    T_INT(textsearch_find(text, len, 0, "two", 3), 4);
    T_INT(textsearch_find(text, len, 5, "two", 3), 14);      /* next one */
    T_INT(textsearch_find(text, len, 15, "two", 3), -1);     /* past the end */
    T_INT(textsearch_find(text, len, 0, "four", 4), -1);

    /* A needle longer than the text never matches. */
    T_INT(textsearch_find("ab", 2, 0, "abc", 3), -1);

    /* Empty needles never match: that would be infinitely many matches. */
    T_INT(textsearch_find(text, len, 0, "", 0), -1);
    T_INT(textsearch_find(text, len, 0, NULL, 0), -1);
    T_INT(textsearch_find(NULL, len, 0, "two", 3), -1);

    /* A negative start is clamped to 0. */
    T_INT(textsearch_find(text, len, -5, "one", 3), 0);

    /* The needle can sit at the very end. */
    T_INT(textsearch_find(text, len, 0, "one", 3), 0);
    T_INT(textsearch_find(text, len, 1, "one", 3), 18);

    /* A match of the same length as the rest of the text. */
    T_INT(textsearch_find("xxab", 4, 2, "ab", 2), 2);

    /* Bytes, not code points: a UTF-8 search still matches on bytes. */
    T_INT(find("caf\xC3\xA9 latte", "\xC3\xA9"), 3);
}

static void find_back_rules(void)
{
    T_SECTION("find_back: previous match");

    char const *text = "one two three two one";
    int len = (int)strlen(text);

    T_INT(textsearch_find_back(text, len, len, "two", 3), 14);
    T_INT(textsearch_find_back(text, len, 14, "two", 3), 4);
    T_INT(textsearch_find_back(text, len, 4, "two", 3), -1);
    T_INT(textsearch_find_back(text, len, 0, "two", 3), -1);

    /* `before` beyond the end is clamped. */
    T_INT(textsearch_find_back(text, len, len + 100, "one", 3), 18);

    T_INT(textsearch_find_back(text, len, len, "", 0), -1);
    T_INT(textsearch_find_back(NULL, len, len, "two", 3), -1);
}

static void counting(void)
{
    T_SECTION("count: non-overlapping matches");

    T_INT(textsearch_count("one two three two one", 19, "two", 3), 2);
    T_INT(textsearch_count("aaaa", 4, "aa", 2), 2);      /* overlapping */
    T_INT(textsearch_count("Banana", 6, "ana", 3), 1);   /* overlap ignored */
    T_INT(textsearch_count("abc", 3, "x", 1), 0);
    T_INT(textsearch_count("", 0, "a", 1), 0);
    T_INT(textsearch_count("abc", 3, "", 0), 0);

    /* Matching down to the last byte terminates. */
    T_INT(textsearch_count("xax", 3, "x", 1), 2);
}

static void wrapping(void)
{
    T_SECTION("find_wrap: jump to the next match, from anywhere");

    /* The plan's case: the word is on the last line. */
    char const *text = "line one\nline two\nthe last line has zebra\n";
    int len = (int)strlen(text);

    T_INT(textsearch_find_wrap(text, len, 0, "zebra", 5), 36);
    /* Wraps back to the first match when searching from the end. */
    T_INT(textsearch_find_wrap(text, len, 31, "line", 4), 0);
    T_INT(textsearch_find_wrap(text, len, len, "LINE", 4), 0);
    /* No match at all. */
    T_INT(textsearch_find_wrap(text, len, 0, "giraffe", 7), -1);
    T_INT(textsearch_find_wrap(text, len, 0, "", 0), -1);
}

static void lines(void)
{
    T_SECTION("line count, starts, ends");

    char const *t1 = "a\nb\nc";
    char const *t2 = "a\nb\n";
    char const *t3 = "";
    char const *t4 = "\n\n";
    char const *t5 = "single";
    char const *t6 = "crlf\r\nnext\r\n";

    T_INT(textsearch_line_count(t1, 5), 3);
    T_INT(textsearch_line_count(t2, 4), 2);
    T_INT(textsearch_line_count(t3, 0), 0);
    T_INT(textsearch_line_count(t4, 2), 2);
    T_INT(textsearch_line_count(t5, 6), 1);
    T_INT(textsearch_line_count(t6, 12), 2);
    T_INT(textsearch_line_count(NULL, 0), 0);

    /* Starts. */
    T_INT(textsearch_line_start(t1, 5, 0), 0);
    T_INT(textsearch_line_start(t1, 5, 1), 2);
    T_INT(textsearch_line_start(t1, 5, 2), 4);
    T_INT(textsearch_line_start(t1, 5, 3), 5); /* past the end -> len */

    /* Ends, with CR stripped. */
    T_INT(textsearch_line_end(t1, 5, 0), 1);
    T_INT(textsearch_line_end(t1, 5, 1), 3);
    T_INT(textsearch_line_end(t1, 5, 2), 5);
    T_INT(textsearch_line_end(t1, 5, 3), -1);
    T_INT(textsearch_line_end(t6, 12, 0), 4); /* "crlf", no CR */
    T_INT(textsearch_line_end(t6, 12, 1), 10);
    T_INT(textsearch_line_end(t2, 4, 1), 3);
    T_INT(textsearch_line_end(t3, 0, 0), -1);

    /* Empty lines have an empty range. */
    T_INT(textsearch_line_end(t4, 2, 0), 0);
    T_INT(textsearch_line_start(t4, 2, 1), 1);
    T_INT(textsearch_line_end(t4, 2, 1), 1);
}

static void line_of_position(void)
{
    T_SECTION("line_of: which line holds a match");

    char const *text = "line one\nline two\nthe last line has zebra\n";
    int len = (int)strlen(text);

    T_INT(textsearch_line_of(text, len, 0), 0);
    T_INT(textsearch_line_of(text, len, 8), 0);   /* the newline itself */
    T_INT(textsearch_line_of(text, len, 9), 1);   /* first byte of line 1 */
    T_INT(textsearch_line_of(text, len, 36), 2);  /* "zebra" */
    T_INT(textsearch_line_of(text, len, len), 2); /* end = last line */
    T_INT(textsearch_line_of(text, len, -1), 0);
    T_INT(textsearch_line_of(text, len, 999), 2);

    /* The match maps to a line whose range contains it: this is the jump the
     * viewer does from a search hit. */
    int hit = textsearch_find_wrap(text, len, 0, "zebra", 5);
    int line = textsearch_line_of(text, len, hit);
    T_INT(line, 2);
    T_TRUE(hit >= textsearch_line_start(text, len, line));
    T_TRUE(hit <= textsearch_line_end(text, len, line));

    T_TRUE(textsearch_at_line_end(text, len, 8));
    T_TRUE(textsearch_at_line_end(text, len, len));
    T_FALSE(textsearch_at_line_end(text, len, 7));
    T_TRUE(textsearch_at_line_end(NULL, 0, 0));
}

static void empty_and_short(void)
{
    T_SECTION("empty and one byte texts");

    T_INT(textsearch_find("", 0, 0, "a", 1), -1);
    T_INT(textsearch_find("a", 1, 0, "a", 1), 0);
    T_INT(textsearch_find("a", 0, 0, "a", 1), -1); /* zero length hides it */
    T_INT(textsearch_line_of("", 0, 0), 0);
    T_INT(textsearch_line_start("", 0, 0), 0);
    T_INT(textsearch_line_end("", 0, 0), -1);
}

static void run_tests(void)
{
    case_folding();
    find_rules();
    find_back_rules();
    counting();
    wrapping();
    lines();
    line_of_position();
    empty_and_short();
}

T_MAIN("test_textsearch")
