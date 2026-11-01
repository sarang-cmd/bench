/* test_starter_files.c - the starter pack folder, read by the add-in's own parsers.
 *
 * starterpack/ ships notes, decks and a study log so a new install has something to
 * look at.  Those files are documentation, and documentation rots: a line the
 * parser skips is a line the student never sees.  This test reads every starter pack
 * file with the same code the calculator runs and fails on a single skipped
 * line.
 *
 * make test runs from the repository root, so the paths are relative to it. */

#include <stdio.h>
#include <string.h>

#include "test_util.h"

#include "cards_parse.h"
#include "logbook.h"
#include "md_parse.h"

/* Wide enough for the longest starter pack file, with room to spare. */
static char text[16384];

/* Reads a whole file.  Returns the length, or -1 when it cannot be opened. */
static int slurp(char const *path)
{
    FILE *f = fopen(path, "rb");
    size_t len;

    if (!f)
        return -1;
    len = fread(text, 1, sizeof text - 1, f);
    fclose(f);
    text[len] = '\0';
    return (int)len;
}

static void decks(void)
{
    static deck_t deck;
    static cards_issue_t issues[8];
    static char const *const files[] = {
        "starterpack/cards/mechanics.txt",
        "starterpack/cards/moles-and-concentration.txt",
        "starterpack/cards/quadratics.txt",
    };
    static int const wanted[] = { 10, 8, 8 };

    T_SECTION("starter pack decks parse with nothing skipped");

    for (int i = 0; i < 3; i++) {
        int len = slurp(files[i]);
        int nissues = -1;
        int n;

        T_TRUE(len > 0);
        if (len <= 0)
            continue;
        n = cards_parse(text, len, &deck, issues, 8, &nissues);
        T_INT(nissues, 0);
        T_INT(n, wanted[i]);
        T_INT(deck.count, wanted[i]);
        /* An id of 0 would mean the hash failed; every card has one. */
        T_TRUE(deck.card[0].id != 0);
        T_TRUE(strlen(deck.card[0].q) > 0);
        T_TRUE(strlen(deck.card[0].a) > 0);
    }
}

static void notes(void)
{
    static md_block_t blocks[256];
    static char const *const files[] = {
        "starterpack/notes/photosynthesis.md",
        "starterpack/notes/constant-acceleration.md",
        "starterpack/notes/uncertainty-rules.md",
    };

    T_SECTION("starter pack notes have blocks the reader can draw");

    for (int i = 0; i < 3; i++) {
        int len = slurp(files[i]);
        int n, headings = 0, fenced = 0;

        T_TRUE(len > 0);
        if (len <= 0)
            continue;
        n = md_parse_blocks(text, len, blocks, 256);
        T_TRUE(n > 4);
        for (int b = 0; b < n; b++) {
            T_TRUE((int)blocks[b].offset + (int)blocks[b].length <= len);
            if (blocks[b].type == MD_BLOCK_HEADING)
                headings++;
            if (blocks[b].type == MD_BLOCK_CODE)
                fenced++;
        }
        T_TRUE(headings >= 1);
        T_TRUE(fenced >= 1);
    }
}

static void study_log(void)
{
    static log_entry_t entry[LOG_MAX];
    int len = slurp("starterpack/study/log.txt");
    int skipped = -1;
    int n;

    T_SECTION("the starter pack study log adds up");

    T_TRUE(len > 0);
    if (len <= 0)
        return;
    n = log_parse_all(text, entry, LOG_MAX, &skipped);
    T_INT(n, 16);
    T_INT(skipped, 0);
    /* 25+15+30+20+10+40+15+35+10+30+45+20+25+15+40+30 */
    T_INT(log_total(entry, n), 405);
    T_INT(log_days_logged(entry, n), 12);
    T_INT(log_tool_total(entry, n, "pacer"), 240);
    T_INT(log_tool_total(entry, n, "cards"), 125);
    T_INT(log_longest_streak(entry, n), 6);
}

static void run_tests(void)
{
    decks();
    notes();
    study_log();
}

T_MAIN("test_starter_files")
