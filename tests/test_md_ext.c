/* test_md_ext.c - math spans, tables and the LaTeX-to-text converter.
 *
 * Spec: 1.3.0 deliverable (a) "real LaTeX" and (b) "real Markdown".  The core
 * cannot draw, so "real" means: the parser recognises the constructs and the
 * converter turns them into characters the font can actually show.
 *
 * Vectors are the smallest input that proves each rule; the expected strings
 * are plain ASCII or single-byte-free UTF-8 sequences copied from the table in
 * md_parse.c, so a mismatch names the construct. */

#include <string.h>

#include "md_parse.h"
#include "test_util.h"

static md_span_t spans[16];
static md_cell_t cells[8];
static char out[128];

/* ---- inline math -------------------------------------------------------- */

static void test_math_span(void)
{
    char const *src = "area $\\pi r^2$ here";
    int n = md_parse_spans(src, 0, (int)strlen(src), spans, 16);

    T_INT(n, 3);
    T_TRUE(spans[0].style == MD_SPAN_TEXT);
    T_TRUE(spans[1].style == MD_SPAN_MATH);
    T_TRUE(spans[1].length == 7);           /* "\pi r^2" without the dollars */
    T_TRUE(memcmp(src + spans[1].offset, "\\pi r^2", 7) == 0);
    T_TRUE(spans[2].style == MD_SPAN_TEXT);

    /* A lone dollar is text, not an unterminated formula. */
    n = md_parse_spans("costs $5", 0, 9, spans, 16);
    T_INT(n, 1);
    T_TRUE(spans[0].style == MD_SPAN_TEXT);
    T_INT(spans[0].length, 9);

    /* Escaped dollar stays text. */
    n = md_parse_spans("\\$5 and \\$6", 0, 11, spans, 16);
    for (int i = 0; i < n; i++)
        T_TRUE(spans[i].style == MD_SPAN_TEXT);
}

/* ---- block math --------------------------------------------------------- */

static void test_math_block(void)
{
    char const *src = "text\n$$\n\\frac{a}{b}\n$$\nmore";
    md_block_t blocks[8];
    int n = md_parse_blocks(src, (int)strlen(src), blocks, 8);

    T_INT(n, 3);
    T_TRUE(blocks[1].type == MD_BLOCK_MATH);
    T_TRUE(blocks[1].length == 11);        /* "\frac{a}{b}" only */
    T_TRUE(blocks[2].type == MD_BLOCK_PARAGRAPH);
}

/* ---- LaTeX to text ------------------------------------------------------ */

static char const *tex(char const *src)
{
    int n = md_latex_to_text(src, (int)strlen(src), out, (int)sizeof out);
    out[n >= 0 && n < (int)sizeof out ? n : 0] = '\0';
    return out;
}

static void test_tex_symbols(void)
{
    T_TRUE(strcmp(tex("\\pi"), "\xcf" "\x80") == 0);            /* pi */
    T_INT(strcmp(tex("\\alpha + \\beta"), "\xce" "\xb1 + \xce" "\xb2"), 0);
    T_TRUE(strcmp(tex("a \\times b"), "a \xc3" "\x97 b") == 0); /* times */
    T_TRUE(strcmp(tex("x \\le 3"), "x \xe2" "\x89" "\xa4 3") == 0); /* le */
    T_INT(strcmp(tex("x \\ge 3"), "x \xe2" "\x89" "\xa5 3"), 0);
    T_INT(strcmp(tex("a \\ne b"), "a \xe2" "\x89" "\xa0 b"), 0);
    T_INT(strcmp(tex("5 \\pm 2"), "5 \xc2" "\xb1 2"), 0);
    T_INT(strcmp(tex("90\\degree"), "90\xc2" "\xb0"), 0);
    T_INT(strcmp(tex("a \\cdot b"), "a \xc2" "\xb7 b"), 0);
    T_INT(strcmp(tex("\\infty"), "\xe2" "\x88" "\x9e"), 0);
    T_INT(strcmp(tex("x \\to 0"), "x \xe2" "\x86" "\x92 0"), 0);
    T_INT(strcmp(tex("A \\cap B"), "A \xe2" "\x88" "\xa9 B"), 0);
}

static void test_tex_fractions_roots(void)
{
    /* \frac{a}{b} becomes a/b: the font has no stacked fraction glyph. */
    T_INT(strcmp(tex("\\frac{1}{2}"), "1/2"), 0);
    T_INT(strcmp(tex("\\frac{x+1}{x-1}"), "x+1/x-1"), 0);
    T_INT(strcmp(tex("\\sqrt{9}"), "\xe2" "\x88" "\x9a" "9"), 0);
    T_INT(strcmp(tex("\\sqrt[3]{8}"), "\xc2" "\xb3\xe2" "\x88" "\x9a" "8"), 0);
}

static void test_tex_scripts(void)
{
    /* Superscript digits and signs map to their Unicode forms; an exponent the
     * table does not cover keeps the caret so nothing is lost. */
    T_INT(strcmp(tex("x^2"), "x\xc2" "\xb2"), 0);
    T_INT(strcmp(tex("x^{12}"), "x\xc2" "\xb9\xc2" "\xb2"), 0);
    T_INT(strcmp(tex("2^{-1}"), "2\xe2" "\x81" "\xbb\xc2" "\xb9"), 0);
    T_INT(strcmp(tex("H_2O"), "H\xe2" "\x82" "\x82O"), 0);
    T_INT(strcmp(tex("x^n"), "x^n"), 0);
    T_INT(strcmp(tex("x^{n+1}"), "x^n+1"), 0);
}

static void test_tex_escapes(void)
{
    T_TRUE(strcmp(tex("a\\,b"), "ab") == 0);      /* spacing commands vanish */
    T_INT(strcmp(tex("100\\%"), "100%"), 0);
    T_INT(strcmp(tex("\\left( x \\right)"), "( x )"), 0);
    T_INT(strcmp(tex("\\text{speed} = 3"), "speed = 3"), 0);
    T_TRUE(strcmp(tex("\\unknowncmd x"), "unknowncmd x") == 0); /* no loss */
    T_INT(strcmp(tex("plain"), "plain"), 0);

    /* Buffer safety: an output buffer too small truncates and reports length. */
    char small[8];
    int n = md_latex_to_text("\\alpha\\beta\\gamma", 20, small, (int)sizeof small);
    T_TRUE(n <= (int)sizeof small - 1);
    T_TRUE(small[n] == '\0');
}

/* ---- tables ------------------------------------------------------------- */

static void test_table_rows(void)
{
    char const *line = "| Name | Value |";
    int n = md_parse_table_row(line, (int)strlen(line), cells, 8);

    T_INT(n, 2);
    T_TRUE(cells[0].length == 4);                 /* "Name" */
    T_TRUE(memcmp(line + cells[0].offset, "Name", 4) == 0);
    T_TRUE(cells[1].length == 5);                 /* "Value" */

    /* Missing outer pipes are fine, and so are empty cells. */
    line = "a | | c";
    n = md_parse_table_row(line, (int)strlen(line), cells, 8);
    T_INT(n, 3);
    T_INT(cells[1].length, 0);
}

static void test_table_detect(void)
{
    char const *src = "before\n| a | b |\n|:--|--:|\n| 1 | 2 |\nafter\n";
    md_table_t t;

    T_TRUE(md_parse_table(src, (int)strlen(src), &t) == 2);   /* two data rows */
    T_INT(t.cols, 2);
    T_TRUE(t.header_offset == 7);                 /* "| a | b |" */
    T_TRUE(t.align[0] == MD_ALIGN_LEFT);
    T_TRUE(t.align[1] == MD_ALIGN_RIGHT);

    /* Centre alignment. */
    T_INT(md_parse_table("| a |\n|:-:|\n| 1 |\n", 18, &t), 1);
    T_TRUE(t.align[0] == MD_ALIGN_CENTER);

    /* A pipe line without a separator underneath is not a table. */
    T_INT(md_parse_table("| a | b |\ntext\n", 14, &t), 0);
    T_INT(md_parse_table("no table here\n", 14, &t), 0);
}

static void test_table_without_outer_pipes(void)
{
    md_table_t t;
    T_INT(md_parse_table("a | b\n---|---\n1 | 2\n", 18, &t), 1);
    T_INT(t.cols, 2);
}

static char prepared[512];

static char const *prep(char const *src)
{
    int n = md_prepare(src, (int)strlen(src), prepared, (int)sizeof prepared, 80);

    (void)n;
    return prepared;
}

static void test_prepare_math(void)
{
    T_SECTION("math is flattened for the viewer");

    T_STR(prep("Area $\\pi r^2$ done\n"), "Area pi r^2 done\n");
    T_STR(prep("$\\frac{1}{2}$ off\n"), "1/2 off\n");
    T_STR(prep("$\\sqrt{2}$\n"), "sqrt2\n");
    T_STR(prep("$\\sqrt[3]{8}$\n"), "sqrt[3]8\n");
    T_STR(prep("x^{12}\n"), "x^{12}\n");     /* outside math: untouched */
    T_STR(prep("$5.00 lost$\n"), "5.00 lost\n");

    /* Block math is flattened line by line. */
    T_STR(prep("$$\n\\frac{a}{b}\n$$\n"), "$$\na/b\n$$\n");
    T_STR(prep("$$\n\\alpha = \\frac{v}{t}\n$$\n"), "$$\nalpha = v/t\n$$\n");

    /* Bad arguments. */
    T_INT(md_prepare(NULL, 1, prepared, 8, 80), -1);
    T_INT(md_prepare("x", 1, NULL, 8, 80), -1);
    T_INT(md_prepare("x", 1, prepared, 0, 80), -1);
}

static void test_prepare_table(void)
{
    T_SECTION("tables become aligned rows");

    T_STR(prep("| Name | Qty |\n|:--|--:|\n| x | 42 |\n"),
          "| Name | Qty |\n"
          "| ---- | --- |\n"
          "| x    |  42 |\n");

    /* Text before and after the table is kept. */
    T_STR(prep("before\n| a | b |\n|---|---|\n| 1 | 2 |\nafter\n"),
          "before\n"
          "| a | b |\n"
          "| - | - |\n"
          "| 1 | 2 |\n"
          "after\n");
}

static void test_table_block(void)
{
    char const *src = "text\n| a | b |\n| - | - |\nmore\n";
    md_block_t blocks[8];
    int n = md_parse_blocks(src, (int)strlen(src), blocks, 8);

    T_SECTION("a padded row is its own block");

    /* The rows are not merged into the paragraph around them. */
    T_INT(n, 4);
    T_INT(blocks[1].type, MD_BLOCK_TABLE);
    T_INT(blocks[2].type, MD_BLOCK_TABLE);
    T_INT(blocks[3].type, MD_BLOCK_PARAGRAPH);
}

static void test_prepare_width(void)
{
    T_SECTION("a table shrinks to the screen");

    /* 20 columns cannot hold "| acceleration | value |", so the cells shrink
     * until the longest row fits. */
    char const *src = "| acceleration | value |\n|---|---|\n| a | 1 |\n";
    int n = md_prepare(src, (int)strlen(src), prepared, (int)sizeof prepared,
                       20);
    char const *line = prepared;
    int longest = 0;

    (void)n;
    while (*line) {
        int len = 0;

        while (line[len] && line[len] != '\n')
            len++;
        if (len > longest)
            longest = len;
        line += len;
        if (*line)
            line++;
    }
    T_CHECK(longest <= 20, "longest row is %d columns", longest);
    /* And the text is still there, just narrower. */
    T_CHECK(strstr(prepared, "ac") != NULL, "cells keep their text");
}

static void run_tests(void)
{
    test_math_span();
    test_math_block();
    test_tex_symbols();
    test_tex_fractions_roots();
    test_tex_scripts();
    test_tex_escapes();
    test_table_rows();
    test_table_detect();
    test_table_without_outer_pipes();
    test_prepare_math();
    test_prepare_table();
    test_table_block();
    test_prepare_width();
}

T_MAIN("test_md_ext")
