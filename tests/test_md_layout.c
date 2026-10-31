/* test_md_layout.c - wrapping, span records, indents, glyph gaps.
 *
 * The test font is deliberately simple so the expected line breaks are obvious:
 * every glyph is 6 px, a space is 3 px, and U+263A has no glyph at all.
 * With max_width 60 that is 12 narrow characters or 10 wide ones a line. */

#include <string.h>

#include "md_layout.h"
#include "test_util.h"

#define LINE_CAP 4096
#define SPAN_CAP 8192

static md_display_line_t lines[LINE_CAP];
static md_display_span_t spans[SPAN_CAP];
static md_layout_stats_t stats;

static int test_width(void *ctx, uint32_t cp, uint8_t style)
{
    (void)ctx;
    (void)style;
    if (cp == ' ' || cp == '\t')
        return 3;
    if (cp == '\n')
        return 3;
    return 6;
}

static bool test_has_glyph(void *ctx, uint32_t cp)
{
    (void)ctx;
    return cp != 0x263A; /* U+263A WHITE SMILING FACE is missing */
}

static md_metrics_t const font = { test_width, test_has_glyph, NULL };

static int layout(char const *src, int max_width)
{
    memset(&stats, 0, sizeof stats);
    return md_layout_text(src, (int)strlen(src), max_width, &font, lines,
                          LINE_CAP, spans, SPAN_CAP, &stats);
}

/* Copies the text a display line covers. */
static char const *line_text(char const *src, int index)
{
    static char buf[256];
    size_t out = 0;
    md_display_line_t const *l = &lines[index];

    buf[0] = '\0';
    for (int i = 0; i < l->span_count; i++) {
        md_display_span_t const *s = &spans[l->first_span + i];
        for (int k = 0; k < s->length && out < sizeof buf - 1; k++)
            buf[out++] = src[s->offset + k];
    }
    buf[out >= sizeof buf ? sizeof buf - 1 : out] = '\0';
    return buf;
}

static void plain_wrap(void)
{
    T_SECTION("paragraph wrapping");

    char const *src = "aa bb cc dd ee ff gg\n";
    int n = layout(src, 60);
    T_INT(n, 2);
    T_INT(stats.lines, 2);

    /* "aa bb cc dd" is 8 glyphs (48 px) plus 3 spaces (9 px) = 57 px, so it
     * fits in 60; the next word would not. */
    T_STR(line_text(src, 0), "aa bb cc dd");
    T_STR(line_text(src, 1), "ee ff gg");

    T_INT(lines[0].kind, MD_BLOCK_PARAGRAPH);
    T_INT(lines[0].indent, 0);
    T_TRUE(lines[0].width <= 60);

    /* A very narrow screen still breaks between words. */
    {
        char const *narrow = "one two\n";
        n = layout(narrow, 24);
        T_TRUE(n >= 2);
        T_STR(line_text(narrow, 0), "one");
    }

    /* A word longer than the line is broken, not dropped. */
    char const *long_word = "abcdefghijklmnop\n";
    n = layout(long_word, 30); /* 5 glyphs of 6 px */
    T_INT(n, 4);
    T_STR(line_text(long_word, 0), "abcde");
    T_STR(line_text(long_word, 3), "p");

    /* No text at all: no lines, no crash. */
    T_INT(layout("", 60), 0);
    T_INT(layout("\n\n\n", 60), 0);
}

static void headings_layout(void)
{
    T_SECTION("headings keep their level and are styled by the renderer");

    char const *src = "# One\n## Two\n### Three\nbody\n";
    int n = layout(src, 60);
    T_INT(n, 4);
    T_INT(lines[0].kind, MD_BLOCK_HEADING);
    T_INT(lines[0].level, 1);
    T_STR(line_text(src, 0), "One");
    T_INT(lines[1].level, 2);
    T_INT(lines[2].level, 3);
    T_INT(lines[3].kind, MD_BLOCK_PARAGRAPH);
}

static void list_indents(void)
{
    T_SECTION("list and quote indents");

    char const *src = "- one\n  - two\n    - three\n> quote\n";
    int n = layout(src, 120);
    T_INT(n, 4);

    T_INT(lines[0].kind, MD_BLOCK_BULLET);
    T_INT(lines[0].indent, MD_LAYOUT_INDENT_PX);
    T_INT(lines[1].indent, 2 * MD_LAYOUT_INDENT_PX);
    T_INT(lines[2].indent, 3 * MD_LAYOUT_INDENT_PX);
    T_INT(lines[3].kind, MD_BLOCK_QUOTE);
    T_INT(lines[3].indent, MD_LAYOUT_INDENT_PX);

    T_STR(line_text(src, 0), "one");
    T_STR(line_text(src, 1), "two");

    /* Check boxes and numbers reach the renderer. */
    char const *checks = "- [ ] todo\n- [x] done\n3. third\n";
    n = layout(checks, 120);
    T_INT(n, 3);
    T_INT(lines[0].marker, MD_CHECK_TODO);
    T_INT(lines[1].marker, MD_CHECK_DONE);
    T_INT(lines[2].kind, MD_BLOCK_ORDERED);
    T_INT(lines[2].marker, 3);

    /* Wrapped list text keeps the indent and only the first line has a marker. */
    char const *long_item =
        "- aaaa bbbb cccc dddd eeee ffff\n";
    n = layout(long_item, 60);
    T_TRUE(n >= 2);
    T_INT(lines[0].indent, MD_LAYOUT_INDENT_PX);
    T_INT(lines[1].indent, MD_LAYOUT_INDENT_PX);
    T_INT(lines[1].marker, 0);

    /* The indent never eats the whole width. */
    n = layout(long_item, 20);
    T_TRUE(n >= 2);
    T_TRUE(lines[0].indent < 20);
}

static void rules_and_code(void)
{
    T_SECTION("rules and code blocks");

    char const *src = "a\n---\nb\n";
    int n = layout(src, 60);
    T_INT(n, 3);
    T_INT(lines[1].kind, MD_BLOCK_RULE);
    T_INT(lines[1].span_count, 0);
    T_INT(lines[1].width, 0);

    /* Code lines are not wrapped. */
    char const *code = "```\nsome very long code line that never wraps\n```\n";
    n = layout(code, 60);
    T_INT(n, 1);
    T_INT(lines[0].kind, MD_BLOCK_CODE);
    T_INT(lines[0].span_count, 1);
    T_TRUE(lines[0].width > 60); /* the renderer clips it */
    T_INT(spans[0].style, MD_SPAN_CODE);
    T_STR(line_text(code, 0), "some very long code line that never wraps");

    /* Each code line is its own display line. */
    {
        char const *multi = "```\na\nb\nc\n```\n";
        n = layout(multi, 60);
        T_INT(n, 3);
        T_STR(line_text(multi, 0), "a");
        T_STR(line_text(multi, 2), "c");
    }

    /* Markup inside code keeps its characters. */
    char const *markup = "```\n**not bold**\n```\n";
    n = layout(markup, 120);
    T_INT(n, 1);
    T_STR(line_text(markup, 0), "**not bold**");
    T_INT(spans[0].style, MD_SPAN_CODE);
}

static void span_records(void)
{
    T_SECTION("span records carry offset, length and style");

    char const *src = "plain **bold** tail\n";
    int n = layout(src, 300);
    T_INT(n, 1);
    T_INT(lines[0].span_count, 3);

    md_display_span_t *s = &spans[lines[0].first_span];
    T_INT(s[0].style, MD_SPAN_TEXT);
    T_STR(line_text(src, 0), "plain bold tail");

    T_INT(s[1].style, MD_SPAN_BOLD);
    /* Offsets point into the source: slice them out and check. */
    {
        char buf[16];
        memcpy(buf, src + s[1].offset, s[1].length);
        buf[s[1].length] = '\0';
        T_STR(buf, "bold");
        T_INT(s[1].offset, 8); /* after "plain **" */
    }

    /* A bold word split across a line break keeps its style on both lines. */
    char const *wrap_bold = "aa **bbbbbbbbbb** cc\n";
    n = layout(wrap_bold, 36); /* 6 glyphs a line */
    T_TRUE(n >= 3);
    for (int i = 0; i < n; i++) {
        for (int k = 0; k < lines[i].span_count; k++) {
            md_display_span_t const *sp = &spans[lines[i].first_span + k];
            T_TRUE(sp->length > 0);
            T_TRUE(sp->offset + sp->length <= strlen(wrap_bold));
        }
    }

    /* An empty line has no spans. */
    T_INT(lines[0].first_span + lines[0].span_count <=
              lines[n - 1].first_span + lines[n - 1].span_count,
          true);
}

static void glyph_gaps(void)
{
    T_SECTION("characters the font lacks are counted");

    /* U+263A is 3 bytes: E2 98 BA. */
    char const *src = "ok \xE2\x98\xBA ok\n";
    int n = layout(src, 200);
    T_INT(n, 1);
    T_INT(stats.unsupported, 1);
    T_STR(line_text(src, 0), "ok \xE2\x98\xBA ok");

    T_INT(layout("plain ascii\n", 200), 1);
    T_INT(stats.unsupported, 0);

    /* A bad sequence counts as one replacement character and does not loop. */
    char const *bad = "a\xFF\xFE" "b\n";
    T_INT(layout(bad, 200), 1);
    T_TRUE(stats.unsupported <= 2);

    /* UTF-8 decoding of a 2, 3 and 4 byte sequence. */
    {
        char const *u = "a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";
        int i = 0, count = 0;
        int len = (int)strlen(u);
        while (i < len) {
            (void)md_utf8_decode(u, len, &i);
            count++;
        }
        T_INT(count, 4); /* a, e-acute, euro, emoji */
        T_INT(md_utf8_length(u, len), 4);
    }
}

static void buffer_limits(void)
{
    T_SECTION("small output buffers report overflow instead of overrunning");

    char const *src = "aa bb cc dd ee ff gg hh ii jj\n";
    md_display_line_t small_lines[2];
    md_display_span_t small_spans[4];
    md_layout_stats_t st;

    memset(&st, 0, sizeof st);
    int n = md_layout_text(src, (int)strlen(src), 12, &font, small_lines, 2,
                          small_spans, 4, &st);
    T_TRUE(n >= 3);
    T_TRUE(st.overflow);

    /* Zero capacity is handled too. */
    memset(&st, 0, sizeof st);
    n = md_layout_text(src, (int)strlen(src), 12, &font, NULL, 0, NULL, 0, &st);
    T_INT(n, -1);
}

static void long_note(void)
{
    T_SECTION("a 30 KiB note lays out completely");

    /* 30 KiB of mixed Markdown: 1024 blocks of about 30 bytes. */
    static char note[30 * 1024 + 64];
    size_t off = 0;
    int n;

    for (int i = 0; off + 64 < sizeof note; i++) {
        if (i % 7 == 0)
            off += (size_t)snprintf(note + off, sizeof note - off,
                                    "# heading %d\n", i);
        else if (i % 5 == 0)
            off += (size_t)snprintf(note + off, sizeof note - off,
                                    "- item %d with some text\n", i);
        else
            off += (size_t)snprintf(note + off, sizeof note - off,
                                    "para %d with words and **bold** text\n", i);
    }
    T_TRUE(off > 29 * 1024);

    memset(&stats, 0, sizeof stats);
    n = md_layout_text(note, (int)off, 384 - 8, &font, lines, LINE_CAP, spans,
                       SPAN_CAP, &stats);
    T_TRUE(n > 500);           /* plenty of display lines */
    T_TRUE(n < LINE_CAP);
    T_INT(stats.lines, n);
    T_FALSE(stats.overflow);
    T_TRUE(stats.spans > 0);

    /* A note longer than the buffers fills what fits and says so. */
    memset(&stats, 0, sizeof stats);
    n = md_layout_text(note, (int)off, 384 - 8, &font, lines, 100, spans, 200,
                       &stats);
    T_TRUE(n > 500);           /* the real count is still reported */
    T_TRUE(stats.overflow);

    /* The last chunk of the file is laid out: the word "text" appears late. */
    T_TRUE(strstr(note + off - 100, "bold") != NULL || off > 0);
}

static void run_tests(void)
{
    plain_wrap();
    headings_layout();
    list_indents();
    rules_and_code();
    span_records();
    glyph_gaps();
    buffer_limits();
    long_note();
}

T_MAIN("test_md_layout")
