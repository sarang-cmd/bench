/* app_formula.c - the formula booklets.
 *
 * Spec: the request to put the IB AA HL, Physics 2025 and Chemistry 2025
 * booklet content into Bench.  The booklets ship as plain text in
 * Bench/formulas/, one file per booklet, so a teacher can add their own and the
 * add-in never needs a rebuild for it.
 *
 * List screen: every .txt in the folder, with the first heading of each as its
 * description.  Viewer: the file as lines, headings in colour, UP/DOWN scrolls,
 * LEFT/RIGHT jump a section at a time (the way a reader turns to a topic), F1
 * to the top.  The section arithmetic is core/formula.c, tested there.
 *
 * Keys: EXE opens, F1 top, LEFT/RIGHT section, EXIT back.
 * Files: Bench/formulas/*.txt, read only. */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/formula.h"
#include "platform/screen.h"
#include "platform/store.h"
#include "theme.h"
#include "ui/widgets.h"

#define BOOK_MAX STORE_FILE_MAX

static store_entry_t books[STORE_LIST_MAX];
static int book_count;
static int book_sel;
static int book_top;
static int shown;              /* a booklet is open */
static char text[BOOK_MAX];
static int too_big;
static char message[64];
static int text_len;
static int first;              /* first line on screen */

static int rows(void)
{
    return (SCR_H - TH_SOFT_H - TH_CONTENT_Y - TH_PAD - 4) / TH_LINE;
}

static void load_list(void)
{
    book_count = store_list("formulas", ".txt", books, STORE_LIST_MAX);
    if (book_count < 0)
        book_count = 0;
    if (book_sel >= book_count)
        book_sel = book_count > 0 ? book_count - 1 : 0;
}

void app_formula_init(void)
{
    shown = 0;
    text_len = 0;
    first = 0;
    too_big = 0;
    message[0] = 0;
    load_list();
}

int app_formula_timeout(void)
{
    return 0;
}

static void open_book(int index)
{
    int len = 0;
    char path[80];

    if (index < 0 || index >= book_count)
        return;
    (void)path;
    /* A booklet bigger than the buffer is refused with a message, not read
     * half-way and shown as if it were whole. */
    if (books[index].size > BOOK_MAX - 1)
    {
        too_big = 1;
        return;
    }
    too_big = 0;
    if (store_read_at(books[index].at, "formulas", books[index].name, text,
                      BOOK_MAX - 1, &len) != STORE_OK)
        return;
    text[len] = 0;
    text_len = len;
    first = 0;
    shown = 1;
}

/* The first heading of a booklet, used as the description in the list. */
static char const *book_subtitle(int index)
{
    static char buf[40];
    int len = 0;
    char path[80];

    (void)path;
    if (store_read_at(books[index].at, "formulas", books[index].name, text,
                      BOOK_MAX - 1, &len) != STORE_OK)
        return "";
    text[len] = 0;
    for (int i = 0; i < formula_line_count(text, len); i++)
    {
        if (formula_heading_level(text, len, i) == 1)
        {
            int const start = formula_line_start(text, len, i) + 2;
            int const end = formula_line_end(text, len, i);
            int n = 0;

            for (int q = start; q < end && n < (int)sizeof buf - 1; q++)
                buf[n++] = text[q];
            buf[n] = 0;
            return buf;
        }
    }
    return "";
}

static void draw_list(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int const w = SCR_W - 2 * TH_PAD;
    int const list_rows = (SCR_H - TH_SOFT_H - y - 2 * TH_LINE - 8) / TH_LINE;

    if (book_count == 0)
    {
        if (store_elsewhere_exists("formulas"))
            w_message("Booklets are in SAVE-F",
                      "Move SAVE-F/Bench/formulas to Bench/formulas");
        else
            w_message("No booklets yet",
                      "Put .txt booklets in Bench/formulas on the card root");
        w_strip(NULL, NULL);
        return;
    }

    /* Which folder the booklets came from, since it can be one of three. */
    w_source_line("Booklets", (int)books[0].at, "Bench/formulas");
    y += TH_LINE;

    if (book_sel < book_top)
        book_top = book_sel;
    if (book_sel >= book_top + list_rows)
        book_top = book_sel - list_rows + 1;

    for (int i = 0; i < list_rows && book_top + i < book_count; i++)
    {
        int const idx = book_top + i;

        /* The booklet's own title line, so the list says "Mathematics AA HL"
         * rather than "aahl.txt". */
        w_row(TH_PAD, y + i * TH_LINE, w, books[idx].name,
              book_subtitle(idx), idx == book_sel);
    }
    w_scrollbar(SCR_W - TH_PAD - 2, y, list_rows * TH_LINE, book_count,
                list_rows, book_top);

    w_strip("UP/DOWN pick", "EXE open");
}

static void draw_book(void)
{
    int const fit = rows();
    int const lines = formula_line_count(text, text_len);
    int const y0 = TH_CONTENT_Y + TH_PAD;
    char buf[64];

    if (first > lines - fit)
        first = lines - fit;
    if (first < 0)
        first = 0;

    for (int i = 0; i < fit && first + i < lines; i++)
    {
        int const n = first + i;
        int const level = formula_heading_level(text, text_len, n);
        int const start = formula_line_start(text, text_len, n);
        int const end = formula_line_end(text, text_len, n);
        int k = 0;
        int color = TH_INK;

        for (int q = (level > 0 ? start + level + 1 : start);
             q < end && k < (int)sizeof buf - 1; q++)
            buf[k++] = text[q];
        buf[k] = 0;
        /* A heading has to be findable by eye in a page of formulae: level 1
         * red, level 2 blue, level 3 ink, all at the same 4.5:1 floor. */
        if (level == 1)
            color = TH_RED;
        else if (level == 2)
            color = TH_BLUE;
        if (k > 0)
            scr_text_ellipsis(TH_PAD, y0 + i * TH_LINE, buf, color,
                              SCR_W - 2 * TH_PAD);
        if (level == 1)
            scr_hline(TH_PAD, y0 + i * TH_LINE + TH_LINE - 3,
                      SCR_W - 2 * TH_PAD, TH_SHADE);
    }

    w_scrollbar(SCR_W - TH_PAD + 2, y0, fit * TH_LINE - 4, lines, fit, first);

    w_strip("LEFT/RIGHT section", "EXIT back");
}

void app_formula_draw(void)
{
    scr_clear(TH_PAPER);
    if (shown)
    {
        w_title("Formula book", "");
        draw_book();
    }
    else
    {
        w_title("Formula book", "Booklets");
        draw_list();
        if (too_big)
            w_strip("that booklet is too big to show", NULL);
    }
    scr_present();
}

app_result_t app_formula_key(bench_event_t ev)
{
    int const lines = formula_line_count(text, text_len);
    int const fit = rows();

    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("formulas"));
        return APP_CONTINUE;
    }
    if (ev.key == BK_MENU)
        return APP_QUIT;

    if (!shown)
    {
        if (ev.key == BK_EXIT)
            return APP_BACK;
        if (ev.key == BK_UP && book_sel > 0)
            book_sel--;
        else if (ev.key == BK_DOWN && book_sel + 1 < book_count)
            book_sel++;
        else if (ev.key == BK_EXE)
            open_book(book_sel);
        return APP_CONTINUE;
    }

    switch (ev.key)
    {
    case BK_EXIT:
        shown = 0;
        break;
    case BK_UP:
        if (first > 0)
            first--;
        break;
    case BK_DOWN:
        if (first + fit < lines)
            first++;
        break;
    case BK_LEFT:
        first = formula_prev_heading(text, text_len, first, 2);
        break;
    case BK_RIGHT:
        first = formula_next_heading(text, text_len, first + 1, 2);
        break;
    case BK_F1:
        first = 0;
        break;
    default:
        break;
    }
    return APP_CONTINUE;
}
