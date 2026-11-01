/* test_md_parse.c - every row of the Markdown syntax table in plan 6.4. */

#include <string.h>

#include "md_parse.h"
#include "test_util.h"

static md_block_t blocks[64];
static md_span_t spans[64];
static char text[512];

static int parse_blocks(char const *src)
{
    return md_parse_blocks(src, (int)strlen(src), blocks, 64);
}

static char const *block_text(char const *src, int index)
{
    int len = (int)blocks[index].length;
    if (len > (int)sizeof text - 1)
        len = (int)sizeof text - 1;
    memcpy(text, src + blocks[index].offset, (size_t)len);
    text[len] = '\0';
    return text;
}

/* Flattens spans into readable text: <b>bold</b>, <i>it</i>, <c>code</c>. */
static char const *flatten(char const *src, int start, int end, int *count)
{
    int n = md_parse_spans(src, start, end, spans, 64);
    size_t out = 0;

    text[0] = '\0';
    for (int i = 0; i < n && i < 64; i++) {
        char const *open = "", *close = "";
        int len = (int)spans[i].length;

        switch ((md_span_style_t)spans[i].style) {
        case MD_SPAN_BOLD:
            open = "<b>";
            close = "</b>";
            break;
        case MD_SPAN_ITALIC:
            open = "<i>";
            close = "</i>";
            break;
        case MD_SPAN_CODE:
            open = "<c>";
            close = "</c>";
            break;
        case MD_SPAN_MATH:
            open = "<m>";
            close = "</m>";
            break;
        case MD_SPAN_TEXT:
            break;
        }
        out += (size_t)snprintf(text + out, sizeof text - out, "%s%.*s%s",
                                open, len, src + spans[i].offset, close);
    }
    *count = n;
    return text;
}

static void headings(void)
{
    T_SECTION("headings # ## ###");

    char const *src = "# One\n## Two\n### Three\n#### Four\n####### Seven\n";
    T_INT(parse_blocks(src), 5);

    T_INT(blocks[0].type, MD_BLOCK_HEADING);
    T_INT(blocks[0].level, 1);
    T_STR(block_text(src, 0), "One");

    T_INT(blocks[1].level, 2);
    T_STR(block_text(src, 1), "Two");

    T_INT(blocks[2].level, 3);
    T_STR(block_text(src, 2), "Three");

    /* Deeper levels clamp to 3 rather than showing raw hashes. */
    T_INT(blocks[3].type, MD_BLOCK_HEADING);
    T_INT(blocks[3].level, 3);
    T_STR(block_text(src, 3), "Four");
    T_INT(blocks[4].level, 3);

    /* No space after the hashes is not a heading. */
    T_INT(parse_blocks("#NoSpace\n"), 1);
    T_INT(blocks[0].type, MD_BLOCK_PARAGRAPH);
}

static void inline_styles(void)
{
    T_SECTION("inline: bold, italic, code, links");

    char const *src = "a **b** c *d* e _f_ g `h` i";
    int n = 0;
    T_STR(flatten(src, 0, (int)strlen(src), &n),
          "a <b>b</b> c <i>d</i> e <i>f</i> g <c>h</c> i");

    /* A link shows its text only. */
    {
        char const *link = "see [the docs](https://x.y/z) now";
        T_STR(flatten(link, 0, (int)strlen(link), &n), "see the docs now");
    }

    /* Markers that never close stay literal. */
    T_STR(flatten("**open", 0, 6, &n), "**open");
    T_STR(flatten("*open", 0, 5, &n), "*open");
    T_STR(flatten("`open", 0, 5, &n), "`open");
    T_STR(flatten("[text](url", 0, 10, &n), "[text](url");
    T_STR(flatten("****", 0, 4, &n), "****");

    /* Escapes make a marker literal and drop the backslash. */
    T_STR(flatten("\\*not italic\\*", 0, 14, &n), "*not italic*");
    T_STR(flatten("a \\` b", 0, 6, &n), "a ` b");

    /* A single marker inside a word is text, not styling. */
    T_STR(flatten("2 * 3 = 6", 0, 9, &n), "2 * 3 = 6");
    T_STR(flatten("snake_case", 0, 10, &n), "snake_case");

    /* Spans never overlap and always move forward. */
    {
        char const *s2 = "**b** `c` _i_ [l](u)";
        int count = md_parse_spans(s2, 0, (int)strlen(s2), spans, 64);
        int last_end = 0;
        for (int i = 0; i < count && i < 64; i++) {
            T_TRUE((int)spans[i].offset >= last_end);
            T_TRUE(spans[i].length > 0);
            last_end = (int)spans[i].offset + (int)spans[i].length;
        }
    }

    /* Empty markup with no inner text stays literal. */
    T_STR(flatten("``", 0, 2, &n), "``");
    /* Markup with nothing inside produces no span. */
    T_INT(md_parse_spans("`x`", 0, 3, spans, 64), 1);
}

static void lists(void)
{
    T_SECTION("bullets, ordered lists, check boxes, nesting");

    char const *src = "- one\n* two\n  - sub\n    - subsub\n      - deeper\n"
                      "1. first\n10. tenth\n";
    T_INT(parse_blocks(src), 7);

    T_INT(blocks[0].type, MD_BLOCK_BULLET);
    T_INT(blocks[0].level, 0);
    T_STR(block_text(src, 0), "one");

    T_INT(blocks[1].type, MD_BLOCK_BULLET);
    T_STR(block_text(src, 1), "two");

    /* Two spaces per level, clamped at 3 levels. */
    T_INT(blocks[2].level, 1);
    T_INT(blocks[3].level, 2);
    T_INT(blocks[4].level, 2);
    T_STR(block_text(src, 2), "sub");

    /* Ordered lists keep their numbers. */
    T_INT(blocks[5].type, MD_BLOCK_ORDERED);
    T_INT(blocks[5].marker, 1);
    T_STR(block_text(src, 5), "first");
    T_INT(blocks[6].marker, 10);

    /* Check boxes. */
    src = "- [ ] todo\n- [x] done\n- [X] also done\n- not a box\n";
    T_INT(parse_blocks(src), 4);
    T_INT(blocks[0].check, MD_CHECK_TODO);
    T_STR(block_text(src, 0), "todo");
    T_INT(blocks[1].check, MD_CHECK_DONE);
    T_STR(block_text(src, 1), "done");
    T_INT(blocks[2].check, MD_CHECK_DONE);
    T_INT(blocks[3].check, MD_CHECK_NONE);
    T_STR(block_text(src, 3), "not a box");

    /* A bare dash is an empty item, not a rule. */
    T_INT(parse_blocks("-\n"), 1);
    T_INT(blocks[0].type, MD_BLOCK_BULLET);
    T_INT(blocks[0].length, 0);
}

static void quotes_and_rules(void)
{
    T_SECTION("quotes and horizontal rules");

    char const *src = "> quoted\n>no space\n>\n---\n-----\n- - -\n";
    T_INT(parse_blocks(src), 6);
    T_INT(blocks[0].type, MD_BLOCK_QUOTE);
    T_STR(block_text(src, 0), "quoted");
    T_INT(blocks[1].type, MD_BLOCK_QUOTE);
    T_STR(block_text(src, 1), "no space");
    T_INT(blocks[2].type, MD_BLOCK_QUOTE);
    T_INT(blocks[2].length, 0);
    T_INT(blocks[3].type, MD_BLOCK_RULE);
    T_INT(blocks[4].type, MD_BLOCK_RULE);
    T_INT(blocks[5].type, MD_BLOCK_RULE);

    /* Two dashes are not a rule. */
    T_INT(parse_blocks("--\n"), 1);
    T_INT(blocks[0].type, MD_BLOCK_PARAGRAPH);
}

static void code_blocks(void)
{
    T_SECTION("fenced code blocks");

    char const *src = "text\n```c\nint x = 1;\n  return x;\n```\nafter\n";
    int n = parse_blocks(src);

    T_INT(n, 3);
    T_INT(blocks[0].type, MD_BLOCK_PARAGRAPH);
    T_STR(block_text(src, 0), "text");

    T_INT(blocks[1].type, MD_BLOCK_CODE);
    T_INT(blocks[1].line_count, 2);
    T_STR(block_text(src, 1), "int x = 1;\n  return x;");

    T_INT(blocks[2].type, MD_BLOCK_PARAGRAPH);
    T_STR(block_text(src, 2), "after");

    /* Markup inside code is not parsed. */
    T_INT(parse_blocks("```\n**not bold**\n```\n"), 1);
    T_INT(blocks[0].type, MD_BLOCK_CODE);

    /* An unclosed fence runs to the end of the file. */
    T_INT(parse_blocks("```\na\nb\n"), 1);
    T_INT(blocks[0].type, MD_BLOCK_CODE);
    T_INT(blocks[0].line_count, 2);

    /* An empty fence draws nothing. */
    T_INT(parse_blocks("```\n```\n"), 1);
    T_INT(blocks[0].type, MD_BLOCK_CODE);
    T_INT(blocks[0].length, 0);
}

static void paragraphs(void)
{
    T_SECTION("paragraphs and blank lines");

    char const *src = "line one\nline two\n\nsecond para\n";
    T_INT(parse_blocks(src), 2);
    T_INT(blocks[0].type, MD_BLOCK_PARAGRAPH);
    T_STR(block_text(src, 0), "line one\nline two");
    T_INT(blocks[0].line_count, 2);
    T_STR(block_text(src, 1), "second para");

    /* A heading ends a paragraph. */
    T_INT(parse_blocks("para\n# head\n"), 2);
    T_INT(blocks[0].type, MD_BLOCK_PARAGRAPH);
    T_INT(blocks[1].type, MD_BLOCK_HEADING);

    /* Leading and trailing blanks are not part of the content. */
    {
        char const *padded = "   padded   \n";
        T_INT(parse_blocks(padded), 1);
        T_STR(block_text(padded, 0), "padded");
    }

    /* next_offset lets a caller resume exactly where the block ended. */
    char const *multi = "a\n\nb\n\nc\n";
    int count = parse_blocks(multi);
    T_INT(count, 3);
    for (int i = 0; i + 1 < count; i++)
        T_TRUE(blocks[i].next_offset <= blocks[i + 1].offset);

    /* Resuming at the end of a block finds the next one. */
    T_INT(md_parse_blocks_from(multi, (int)strlen(multi),
                               (int)blocks[1].next_offset, blocks, 64), 1);
}

static void unsupported_syntax(void)
{
    T_SECTION("unsupported syntax stays plain text");

    /* A pipe row is its own block since 1.3.0 (the viewer draws it as one
     * aligned line), and the text of each row is kept as written. */
    char const *src = "| a | b |\n|---|---|\n";
    T_INT(parse_blocks(src), 2);
    T_INT(blocks[0].type, MD_BLOCK_TABLE);
    T_STR(block_text(src, 0), "| a | b |");
    T_STR(block_text(src, 1), "|---|---|");

    /* An image is text, not a picture. */
    src = "![alt](pic.png)\n";
    T_INT(parse_blocks(src), 1);
    T_STR(block_text(src, 0), "![alt](pic.png)");

    /* HTML is text. */
    src = "<b>not bold</b>\n";
    T_INT(parse_blocks(src), 1);
    T_STR(block_text(src, 0), "<b>not bold</b>");

    /* A footnote marker is text. */
    src = "text [^1]\n";
    T_INT(parse_blocks(src), 1);
    T_STR(block_text(src, 0), "text [^1]");
}

static void edge_cases(void)
{
    T_SECTION("edge cases");

    T_INT(parse_blocks(""), 0);
    T_INT(parse_blocks("\n\n\n"), 0);
    T_INT(parse_blocks("   \n\t\n"), 0);
    T_INT(parse_blocks("#\n"), 1);          /* empty heading */
    T_INT(blocks[0].type, MD_BLOCK_HEADING);
    T_INT(blocks[0].length, 0);
    T_INT(parse_blocks("no newline at end"), 1);
    T_STR(block_text("no newline at end", 0), "no newline at end");

    /* CRLF files parse the same way. */
    T_INT(parse_blocks("# Title\r\n\r\nbody\r\n"), 2);
    T_INT(blocks[0].type, MD_BLOCK_HEADING);
    T_STR(block_text("# Title\r\n\r\nbody\r\n", 1), "body");

    /* The block array may be smaller than the number of blocks. */
    md_block_t small[2];
    int total = md_parse_blocks("a\n\nb\n\nc\n", 9, small, 2);
    T_INT(total, 3);
    T_INT(small[0].type, MD_BLOCK_PARAGRAPH);

    T_INT(md_parse_blocks(NULL, 0, blocks, 64), 0);
    T_INT(md_parse_spans(NULL, 0, 0, spans, 64), 0);

    T_TRUE(md_line_is_blank("   \t", 4));
    T_FALSE(md_line_is_blank(" x ", 3));
}

/* docs/samples/markdown.md uses every row of the syntax table in one file.
 * make test runs from the repository root, so the path is relative to it. */
static void full_sample(void)
{
    T_SECTION("the sample note: every syntax row in one document");

    static char note[8192];
    FILE *f = fopen("docs/samples/markdown.md", "rb");
    size_t len;

    T_TRUE(f != NULL);
    if (!f)
        return;
    len = fread(note, 1, sizeof note - 1, f);
    fclose(f);
    note[len] = '\0';
    T_TRUE(len > 400);

    int n = md_parse_blocks(note, (int)len, blocks, 64);
    T_TRUE(n > 0);

    int headings = 0, bullets = 0, ordered = 0, quotes = 0, rules = 0;
    int code = 0, paras = 0, checks = 0, maths = 0, tables = 0;
    for (int i = 0; i < n; i++) {
        switch ((md_block_type_t)blocks[i].type) {
        case MD_BLOCK_HEADING:
            headings++;
            T_TRUE(blocks[i].level >= 1 && blocks[i].level <= 3);
            break;
        case MD_BLOCK_BULLET:
            bullets++;
            if (blocks[i].check != MD_CHECK_NONE)
                checks++;
            T_TRUE(blocks[i].level <= MD_MAX_LIST_LEVEL);
            break;
        case MD_BLOCK_ORDERED:
            ordered++;
            T_TRUE(blocks[i].marker >= 1);
            break;
        case MD_BLOCK_QUOTE:
            quotes++;
            break;
        case MD_BLOCK_MATH:
            maths++;
            break;
        case MD_BLOCK_TABLE:
            tables++;
            break;
        case MD_BLOCK_RULE:
            rules++;
            break;
        case MD_BLOCK_CODE:
            code++;
            break;
        case MD_BLOCK_PARAGRAPH:
            paras++;
            break;
        }
    }
    T_TRUE(headings >= 3);   /* #, ##, ### */
    T_TRUE(bullets >= 5);    /* three bullets plus two check boxes */
    T_TRUE(ordered >= 3);    /* 1. 2. 10. */
    T_TRUE(quotes >= 2);
    T_TRUE(rules >= 1);
    T_TRUE(code >= 1);
    T_TRUE(paras >= 4);      /* text, image, html, footnote */
    T_INT(checks, 2);
    /* The sample now carries 1.3.0 maths and a table. */
    T_TRUE(maths >= 1);
    T_TRUE(tables >= 1);

    /* Every block stays inside the file. */
    for (int i = 0; i < n; i++) {
        T_TRUE((int)blocks[i].offset + (int)blocks[i].length <= (int)len);
        T_TRUE(blocks[i].next_offset <= len);
    }
}

static void run_tests(void)
{
    full_sample();
    headings();
    inline_styles();
    lists();
    quotes_and_rules();
    code_blocks();
    paragraphs();
    unsupported_syntax();
    edge_cases();
}

T_MAIN("test_md_parse")
