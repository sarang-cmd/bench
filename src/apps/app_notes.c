/* app_notes.c - the Notes viewer (plain text and Markdown).
 *
 * Spec: docs/plan.md section 6.4, TASK-M6 and TASK-M7.
 *
 * Screens
 *   list   files in Bench/notes ending in .md or .txt
 *   view   the note, wrapped to the window; a footer shows position, the
 *          search hit count and how many characters the font does not have
 *   heads  the heading list of a Markdown note (F3), pick one to jump
 *
 * Keys
 *   list   UP/DOWN pick, EXE open, EXIT back
 *   view   UP/DOWN line, LEFT/RIGHT page, F1 search, F2 next hit, F3 headings,
 *          F4 top, EXIT back to the list
 *   heads  UP/DOWN pick, EXE jump, EXIT back
 *
 * Files read: notes/<name> (at most 32 KiB, see BENCH_NOTE_MAX)
 * Files written: none.  Search text lives in memory only. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/md_layout.h"
#include "core/md_parse.h"
#include "core/textsearch.h"
#include "platform/mem.h"
#include "platform/screen.h"
#include "platform/store.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

#define NOTE_LINES 2048
#define NOTE_SPANS 4096
#define NOTE_HEADS 32
#define FOOTER_LINES 2

typedef enum { SC_LIST, SC_VIEW, SC_HEADS } screen_t;

static screen_t screen;
static store_entry_t files[STORE_LIST_MAX];
static int file_count;
static int file_sel;
static int file_top;

static char *text;
static int text_len;
static bool is_markdown;
static md_display_line_t *lines;
static md_display_span_t *spans;
static int line_count;
static int span_count;
static int unsupported;
static int top;
static bool overflow;

static char search[24];
static bool has_search;
static int hits;
static int hit_at;
static int hit_offset;
static char note_title[STORE_NAME_MAX];

static int heading_line[NOTE_HEADS];
static int heading_count;
static int head_sel;
static char message[64];

/* ---- layout ------------------------------------------------------------- */

static int metric_width(void *ctx, uint32_t cp, uint8_t style)
{
    (void)ctx;
    (void)style;
    if (!scr_has_glyph(cp))
        cp = '?';
    return scr_glyph_w(cp);
}

static bool metric_has(void *ctx, uint32_t cp)
{
    (void)ctx;
    return scr_has_glyph(cp);
}

static int content_px(void)
{
    int pct = app_cfg()->notes_wrap;

    if (pct == 0)
        return SCR_W - 2 * TH_PAD;
    return (SCR_W - 2 * TH_PAD) * pct / 100;
}

/* Wrap plain text into display lines: one span per line, greedy on words. */
static int wrap_plain(void)
{
    int n = 0;
    int p = 0;
    int max_px = content_px();

    while (p < text_len && n < NOTE_LINES)
    {
        int line_start = p;
        int line_end = p;
        int last_space = -1;
        int width = 0;

        while (p < text_len && text[p] != '\n')
        {
            int start = p;
            uint32_t cp = md_utf8_decode(text, text_len, &p);
            int w = metric_width(NULL, cp, MD_SPAN_TEXT);

            if (width + w > max_px && last_space > line_start)
            {
                line_end = last_space;
                p = last_space + 1;
                break;
            }
            width += w;
            line_end = p;
            if (text[start] == ' ')
                last_space = start;
        }
        if (p < text_len && text[p] == '\n')
            p++;
        if (line_end < line_start)
            line_end = line_start;

        lines[n].first_span = (uint16_t)n;
        lines[n].span_count = 1;
        lines[n].width = (uint16_t)width;
        lines[n].indent = 0;
        lines[n].kind = MD_BLOCK_PARAGRAPH;
        lines[n].level = 0;
        lines[n].marker = 0;
        spans[n].offset = (uint32_t)line_start;
        spans[n].length = (uint16_t)(line_end - line_start);
        spans[n].style = MD_SPAN_TEXT;
        n++;
        if (p == line_start && p < text_len)
            p++;                       /* never loop on a zero-length line */
    }
    span_count = n;
    return n;
}

static void find_headings(void)
{
    md_block_t blocks[128];
    int n = md_parse_blocks(text, text_len, blocks, 128);

    heading_count = 0;
    for (int i = 0; i < n && heading_count < NOTE_HEADS; i++)
    {
        if (blocks[i].type != MD_BLOCK_HEADING)
            continue;
        /* The display line holding this block's offset. */
        for (int l = 0; l < line_count; l++)
        {
            md_display_span_t const *s = &spans[lines[l].first_span];

            if (lines[l].span_count > 0 && s->offset >= blocks[i].offset)
            {
                heading_line[heading_count++] = l;
                break;
            }
        }
    }
}

static bool load_note(void)
{
    char path[STORE_NAME_MAX + 8];
    int len = 0;
    md_metrics_t metrics = { metric_width, metric_has, NULL };
    md_layout_stats_t stats;

    text = mem_alloc(BENCH_NOTE_MAX);
    lines = mem_alloc(sizeof(md_display_line_t) * NOTE_LINES);
    spans = mem_alloc(sizeof(md_display_span_t) * NOTE_SPANS);
    if (!text || !lines || !spans)
    {
        strcpy(message, "Not enough memory for this note");
        return false;
    }

    if (store_join(path, (int)sizeof path, "notes", files[file_sel].name) !=
        STORE_OK)
        return false;

    int rc = store_read(path, text, BENCH_NOTE_MAX, &len);
    if (rc == STORE_ERR_TOO_BIG)
    {
        strcpy(message, "Note is larger than 32 KiB");
        return false;
    }
    if (rc != STORE_OK)
    {
        strcpy(message, "Could not read that note");
        return false;
    }

    text_len = len;
    strncpy(note_title, files[file_sel].name, sizeof note_title - 1);
    note_title[sizeof note_title - 1] = 0;
    is_markdown = textsearch_find(note_title, (int)strlen(note_title), 0,
                                  ".md", 3) >= 0;

    memset(&stats, 0, sizeof stats);
    if (is_markdown)
    {
        line_count = md_layout_text(text, text_len, content_px(), &metrics,
                                    lines, NOTE_LINES, spans, NOTE_SPANS,
                                    &stats);
        unsupported = stats.unsupported;
        overflow = stats.overflow;
        span_count = stats.spans;
    }
    else
    {
        line_count = wrap_plain();
        unsupported = 0;
        overflow = line_count >= NOTE_LINES;
    }

    top = 0;
    has_search = false;
    hits = 0;
    hit_at = 0;
    hit_offset = -1;
    heading_count = 0;
    head_sel = 0;
    find_headings();
    return true;
}

static void free_note(void)
{
    mem_free(text);
    mem_free(lines);
    mem_free(spans);
    text = NULL;
    lines = NULL;
    spans = NULL;
    line_count = 0;
}

int app_notes_timeout(void)
{
    return 0;
}

void app_notes_init(void)
{
    free_note();
    screen = SC_LIST;
    file_sel = 0;
    file_top = 0;
    message[0] = 0;
    search[0] = 0;
    file_count = store_list("notes", ".md", files, STORE_LIST_MAX);
    if (file_count < 0)
        file_count = 0;
    {
        store_entry_t txt[STORE_LIST_MAX];
        int n = store_list("notes", ".txt", txt, STORE_LIST_MAX);

        for (int i = 0; i < n && file_count < STORE_LIST_MAX; i++)
            files[file_count++] = txt[i];
    }
}

/* ---- drawing ------------------------------------------------------------ */

static int view_rows(void)
{
    return w_rows_in(SCR_H - TH_TITLE_H - TH_SOFT_H - FOOTER_LINES * TH_LINE -
                     4);
}

static void draw_list(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int rows = w_rows_in(TH_CONTENT_H - 3 * TH_LINE);
    char num[16];
    char line[32];

    if (file_count == 0)
    {
        w_message("No notes yet", "Put .md or .txt files in Bench/notes");
        w_strip(NULL, NULL);
        scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                        "EXIT back", TH_INK);
        return;
    }

    scr_text(TH_PAD, y, "Notes in Bench/notes", TH_INK);
    y += TH_LINE + 2;
    file_top = w_scroll_follow(file_sel, file_top, rows);
    for (int i = 0; i < rows && file_top + i < file_count; i++)
    {
        int idx = file_top + i;

        nt_u64((uint64_t)(files[idx].size + 512) / 1024u, num, (int)sizeof num);
        strcpy(line, num);
        nt_append(line, (int)sizeof line, " K");
        w_row(TH_PAD, y + i * TH_LINE, SCR_W - 2 * TH_PAD, files[idx].name,
              line, idx == file_sel);
    }
    w_scrollbar(SCR_W - TH_PAD - 2, y, rows * TH_LINE, file_count, rows,
                file_top);

    if (message[0])
        scr_text_ellipsis(TH_PAD, SCR_H - TH_SOFT_H - TH_LINE - 2, message,
                          TH_RED, SCR_W - 2 * TH_PAD);

    w_softkeys((char const *const[6]){ "Open", "", "", "", "", "" }, -1);
}

/* Draw one span's text, clipped to `max_x`.  Returns the width drawn. */
static int draw_span(int x, int y, md_display_span_t const *s, int color,
                     int max_x)
{
    static char buf[96];
    int from = (int)s->offset;
    int to = from + (int)s->length;
    int n = 0;

    if (from < 0 || to > text_len)
        return 0;
    for (int i = from; i < to && n < (int)sizeof buf - 1; i++)
        buf[n++] = text[i];
    buf[n] = 0;
    if (n == 0)
        return 0;
    if (scr_text_w(buf) > max_x)
    {
        int fit = scr_text_fit(buf, max_x);

        buf[fit] = 0;
    }
    scr_text(x, y, buf, color);
    return scr_text_w(buf);
}

static int span_color(uint8_t kind, uint8_t style, uint8_t level)
{
    if (kind == MD_BLOCK_CODE || style == MD_SPAN_CODE)
        return TH_MD_CODE;
    if (style == MD_SPAN_BOLD)
        return TH_MD_BOLD;
    if (style == MD_SPAN_ITALIC)
        return TH_MD_ITALIC;
    if (kind == MD_BLOCK_HEADING)
        return level == 1 ? TH_MD_H1 : level == 2 ? TH_MD_H2 : TH_MD_H3;
    return TH_MD_INK;
}

static void draw_view(void)
{
    int rows = view_rows();
    int y0 = TH_CONTENT_Y + 1;
    int bottom = y0 + rows * TH_LINE;

    if (top > line_count - rows)
        top = line_count - rows;
    if (top < 0)
        top = 0;

    if (line_count == 0)
    {
        w_message("Empty note", "Nothing to show in this file");
        w_strip(NULL, NULL);
        return;
    }

    scr_fill(0, y0, SCR_W, bottom - y0, TH_PAPER);

    for (int i = 0; i < rows && top + i < line_count; i++)
    {
        md_display_line_t const *ln = &lines[top + i];
        int y = y0 + i * TH_LINE;
        int x = TH_PAD + ln->indent;

        /* Marks for the block kind, drawn in the indent gutter. */
        switch (ln->kind)
        {
        case MD_BLOCK_HEADING:
            if (ln->level == 1)
                scr_hline(TH_PAD, y + TH_LINE - 2, SCR_W - 2 * TH_PAD,
                          TH_MD_H1);
            break;
        case MD_BLOCK_BULLET:
            if (ln->marker == MD_CHECK_TODO)
            {
                scr_frame(TH_PAD, y + 2, 7, 7, TH_MD_BULLET, 1);
            }
            else if (ln->marker == MD_CHECK_DONE)
            {
                scr_fill(TH_PAD, y + 2, 7, 7, TH_MD_BULLET);
            }
            else
                scr_fill(TH_PAD, y + TH_LINE / 2 - 2, 4, 4, TH_MD_BULLET);
            break;
        case MD_BLOCK_ORDERED:
            if (ln->marker > 0 && ln->level == 0)
            {
                char num[8];

                nt_u64((uint64_t)ln->marker, num, (int)sizeof num);
                scr_text(TH_PAD - 2, y, num, TH_MD_BULLET);
            }
            break;
        case MD_BLOCK_QUOTE:
            scr_fill(TH_PAD - 3, y, 2, TH_LINE - 1, TH_MD_QUOTE_BAR);
            break;
        case MD_BLOCK_RULE:
            scr_hline(TH_PAD, y + TH_LINE / 2, SCR_W - 2 * TH_PAD, TH_MD_RULE);
            break;
        default:
            break;
        }

        /* The text, span by span. */
        {
            int max_x = SCR_W - TH_PAD - x;

            for (int k = 0; k < ln->span_count; k++)
            {
                md_display_span_t const *sp = &spans[ln->first_span + k];
                int color = span_color(ln->kind, sp->style, ln->level);
                bool code = ln->kind == MD_BLOCK_CODE ||
                            sp->style == MD_SPAN_CODE;
                int w;

                if (code)
                {
                    /* Measure first so the highlight sits behind the text. */
                    static char tmp[96];
                    int n = 0;

                    for (int q = (int)sp->offset;
                         q < (int)(sp->offset + sp->length) &&
                         n < (int)sizeof tmp - 1; q++)
                        tmp[n++] = text[q];
                    tmp[n] = 0;
                    w = scr_text_w(tmp);
                    scr_fill(x, y, w + 2, scr_text_h() + 1, TH_MD_CODE_BG);
                }
                w = draw_span(x, y, sp, color, max_x);
                if (sp->style == MD_SPAN_ITALIC)
                    scr_hline(x, y + scr_text_h() - 1, w, color);
                x += w;
            }
        }
    }
    scr_vline(SCR_W - 3, y0, rows * TH_LINE, TH_SHADE);

    /* Footer: position, search state, missing glyphs. */
    {
        int fy = bottom + 1;
        char num[16];
        char line[64];

        scr_hline(0, fy - 1, SCR_W, TH_SHADE);
        nt_u64((uint64_t)(top + 1), num, (int)sizeof num);
        strcpy(line, num);
        nt_append(line, (int)sizeof line, "/");
        nt_u64((uint64_t)line_count, num, (int)sizeof num);
        nt_append(line, (int)sizeof line, num);
        nt_append(line, (int)sizeof line, " lines");
        nt_u64((uint64_t)textsearch_words(text, text_len), num, (int)sizeof num);
        nt_append(line, (int)sizeof line, "  ");
        nt_append(line, (int)sizeof line, num);
        nt_append(line, (int)sizeof line, "w");
        scr_text(TH_PAD, fy, line, TH_INK);

        if (has_search)
        {
            strcpy(line, "\"");
            nt_append(line, (int)sizeof line, search);
            nt_append(line, (int)sizeof line, "\" ");
            if (hits > 0)
            {
                nt_u64((uint64_t)(hit_at + 1), num, (int)sizeof num);
                nt_append(line, (int)sizeof line, num);
                nt_append(line, (int)sizeof line, "/");
                nt_u64((uint64_t)hits, num, (int)sizeof num);
                nt_append(line, (int)sizeof line, num);
            }
            else
                nt_append(line, (int)sizeof line, "none");
            scr_text_right(SCR_W - TH_PAD, fy, line, TH_INK);
        }

        fy += TH_LINE;
        if (unsupported > 0)
        {
            nt_u64((uint64_t)unsupported, num, (int)sizeof num);
            strcpy(line, "? ");
            nt_append(line, (int)sizeof line, num);
            nt_append(line, (int)sizeof line, " character(s) not in the font");
            scr_text_ellipsis(TH_PAD, fy, line, TH_INK, SCR_W - 2 * TH_PAD);
        }
        else if (overflow)
            scr_text(TH_PAD, fy, "Note truncated: too long for the viewer",
                     TH_RED);
        else if (is_markdown)
            scr_text(TH_PAD, fy, "Markdown: colour and underline", TH_INK);
        else
            scr_text(TH_PAD, fy, "Plain text", TH_INK);
    }

    w_softkeys((char const *const[6]){ "Find", "Next", "Heads", "Top", "", "" },
               -1);
}

static void draw_heads(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int rows = w_rows_in(TH_CONTENT_H - 3 * TH_LINE);
    char num[12];

    if (heading_count == 0)
    {
        w_message("No headings", "This note has no # lines");
        w_strip(NULL, NULL);
        return;
    }

    scr_text(TH_PAD, y, "Headings", TH_INK);
    y += TH_LINE + 2;
    for (int i = 0; i < rows && i < heading_count; i++)
    {
        int l = heading_line[i];
        md_display_span_t const *sp = &spans[lines[l].first_span];
        char label[64];
        int n = 0;

        for (int q = (int)sp->offset;
             q < (int)(sp->offset + sp->length) && n < (int)sizeof label - 1;
             q++)
            label[n++] = text[q];
        label[n] = 0;
        nt_u64((uint64_t)(l + 1), num, (int)sizeof num);
        w_row(TH_PAD, y + i * TH_LINE, SCR_W - 2 * TH_PAD, label, num,
              i == head_sel);
    }
    w_strip(NULL, NULL);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "UP/DOWN pick   EXE jump   EXIT back", TH_INK);
}

void app_notes_draw(void)
{
    scr_clear(TH_PAPER);
    w_title(screen == SC_LIST ? "Notes" : note_title, "");

    switch (screen)
    {
    case SC_LIST: draw_list(); break;
    case SC_VIEW: draw_view(); break;
    default: draw_heads(); break;
    }
    scr_present();
}

/* ---- search ------------------------------------------------------------ */

/* Move the view so the display line holding `offset` is visible. */
static void jump_to_offset(int offset);

static void run_search(void)
{
    char term[24];
    int nlen;

    term[0] = 0;
    nt_append(term, (int)sizeof term, search);
    if (!w_keyboard(term, (int)sizeof term, "Find text"))
        return;
    if (!term[0])
    {
        has_search = false;
        return;
    }
    strcpy(search, term);
    nlen = (int)strlen(search);
    has_search = true;
    hits = textsearch_count(text, text_len, search, nlen);
    hit_at = 0;
    hit_offset = -1;
    if (hits > 0)
    {
        hit_offset = textsearch_find_wrap(text, text_len, 0, search, nlen);
        jump_to_offset(hit_offset);
    }
}

static void next_hit(void)
{
    int nlen = (int)strlen(search);

    if (!has_search || hits == 0)
        return;
    hit_offset = textsearch_find_wrap(text, text_len, hit_offset + nlen, search,
                                      nlen);
    hit_at = (hit_at + 1) % hits;
    jump_to_offset(hit_offset);
}

static void jump_to_offset(int offset)
{
    int line = 0;

    if (offset < 0)
        return;
    for (int i = 0; i < line_count; i++)
    {
        md_display_span_t const *s = &spans[lines[i].first_span];

        if (lines[i].span_count == 0)
            continue;
        if ((int)s->offset > offset)
            break;
        line = i;
    }
    top = line - view_rows() / 3;
    if (top < 0)
        top = 0;
}

app_result_t app_notes_key(bench_event_t ev)
{
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("notes"));
        return APP_CONTINUE;
    }
    if (ev.key == BK_MENU)
        return APP_QUIT;

    switch (screen)
    {
    case SC_LIST:
        if (ev.key == BK_EXIT)
            return APP_BACK;
        if (ev.key == BK_UP && file_sel > 0)
            file_sel--;
        else if (ev.key == BK_DOWN && file_sel + 1 < file_count)
            file_sel++;
        else if (ev.key == BK_EXE && file_count > 0)
        {
            if (load_note())
                screen = SC_VIEW;
            else if (!message[0])
                strcpy(message, "Could not open that note");
        }
        break;

    case SC_VIEW:
        if (ev.key == BK_EXIT || ev.key == BK_EXE)
        {
            free_note();
            app_notes_init();
            screen = SC_LIST;
        }
        else if (ev.key == BK_DOWN && top + 1 < line_count)
            top++;
        else if (ev.key == BK_UP && top > 0)
            top--;
        else if (ev.key == BK_RIGHT)
        {
            top += view_rows();
            if (top > line_count - 1)
                top = line_count > 0 ? line_count - 1 : 0;
        }
        else if (ev.key == BK_LEFT)
        {
            top -= view_rows();
            if (top < 0)
                top = 0;
        }
        else if (ev.key == BK_F1)
            run_search();
        else if (ev.key == BK_F2)
            next_hit();
        else if (ev.key == BK_F3)
            screen = SC_HEADS;
        else if (ev.key == BK_F4)
            top = 0;
        break;

    default:
        if (ev.key == BK_EXIT)
            screen = SC_VIEW;
        else if (ev.key == BK_UP && head_sel > 0)
            head_sel--;
        else if (ev.key == BK_DOWN && head_sel + 1 < heading_count)
            head_sel++;
        else if (ev.key == BK_EXE)
        {
            top = heading_line[head_sel];
            screen = SC_VIEW;
        }
        break;
    }
    return APP_CONTINUE;
}
