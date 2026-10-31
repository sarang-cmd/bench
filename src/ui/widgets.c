/* widgets.c - see widgets.h.  Drawing only, except w_keyboard(). */

#include "widgets.h"

#include <string.h>

#include "platform/input.h"
#include "platform/screen.h"
#include "theme.h"

/* ---- title and softkeys ------------------------------------------------- */

void w_title(char const *title, char const *hint)
{
    scr_fill(0, 0, SCR_W, TH_TITLE_H, TH_INK);
    /* Cut the title so it can never run into the hint or the red square: a
     * 57-character file name used to cross the whole bar. */
    int const reserve = (hint && *hint ? scr_text_w(hint) + TH_PAD : 0) +
                        TH_PAD + 8;
    int const room = SCR_W - 2 * TH_PAD - reserve;

    scr_text_ellipsis_as(SCR_TR_TITLE, TH_PAD, (TH_TITLE_H - scr_text_h()) / 2,
                         title, TH_PAPER, room);
    if (hint && *hint)
    {
        int const y = (TH_TITLE_H - scr_text_h()) / 2;

        scr_text_as(SCR_TR_TITLE, SCR_W - TH_PAD - scr_text_w(hint), y, hint,
                    TH_YELLOW);
    }
    /* Bauhaus: a 4-pixel red square after the title, always in the same place. */
    scr_fill(SCR_W - TH_PAD - 4, TH_TITLE_H - 6, 4, 4, TH_RED);
}

void w_softkeys(char const *const labels[6], int active)
{
    int y = SCR_H - TH_SOFT_H;

    scr_fill(0, y, SCR_W, TH_SOFT_H, TH_SHADE);
    scr_hline(0, y, SCR_W, TH_INK);
    for (int i = 0; i < 6; i++)
    {
        int x = i * TH_SOFT_W;

        if (i > 0)
            scr_vline(x, y + 1, TH_SOFT_H - 1, TH_PAPER);
        if (active == i)
            scr_fill(x + 1, y + 1, TH_SOFT_W - 1, TH_SOFT_H - 1, TH_INK);
        if (!labels[i] || !*labels[i])
            continue;
        scr_text_as(SCR_TR_SOFTKEY,
                    x + 1 + (TH_SOFT_W - 2 - scr_text_w(labels[i])) / 2,
                    y + (TH_SOFT_H - scr_text_h()) / 2, labels[i],
                    active == i ? TH_PAPER : TH_INK);
    }
}

/* ---- panels, messages, modals ------------------------------------------ */

void w_panel(int x, int y, int w, int h)
{
    scr_fill(x, y, w, h, TH_PAPER);
    scr_frame(x, y, w, h, TH_INK, 1);
    /* A black corner tab: the guide's blocky look in two pixels of geometry. */
    scr_fill(x + 1, y + 1, 3, 3, TH_RED);
}

void w_message(char const *title, char const *line)
{
    int y = TH_CONTENT_Y + TH_CONTENT_H / 2 - TH_LINE;

    scr_fill(0, TH_CONTENT_Y, SCR_W, TH_CONTENT_H, TH_PAPER);
    if (title && *title)
        scr_text_center(0, SCR_W, y, title, TH_INK);
    if (line && *line)
        scr_text_center(0, SCR_W, y + TH_LINE, line, TH_INK);
}

void w_modal(char const *title, char const *const *lines, int count,
             char const *hint)
{
    int w = SCR_W - 2 * TH_PAD;
    int h = TH_LINE * (count + 2) + 2 * TH_PAD;
    int x = TH_PAD;
    int y = TH_CONTENT_Y + (TH_CONTENT_H - h) / 2;

    scr_fill(x, y, w, h, TH_PAPER);
    scr_frame(x, y, w, h, TH_INK, 2);
    scr_fill(x + 2, y + 2, w - 4, TH_TITLE_H - 4, TH_INK);
    scr_text(x + TH_PAD, y + 3, title, TH_PAPER);
    for (int i = 0; i < count; i++)
        scr_text(x + TH_PAD, y + TH_TITLE_H + TH_PAD - 2 + i * TH_LINE,
                 lines[i], TH_INK);
    if (hint && *hint)
        scr_text(x + TH_PAD, y + h - TH_PAD - scr_text_h(), hint, TH_INK);
}

/* ---- lists -------------------------------------------------------------- */

int w_rows_in(int h)
{
    int rows = h / TH_LINE;

    return rows > 0 ? rows : 1;
}

void w_row(int x, int y, int w, char const *text, char const *right, bool sel)
{
    if (sel)
        scr_fill(x, y, w, TH_LINE, TH_INK);
    scr_text_ellipsis(x + 4, y + 1, text, sel ? TH_PAPER : TH_INK,
                      w - 8 - (right ? scr_text_w(right) + 6 : 0));
    if (right && *right)
        scr_text(x + w - 4 - scr_text_w(right), y + 1, right,
                 sel ? TH_YELLOW : TH_RED);
}

int w_scroll_follow(int sel, int top, int visible)
{
    if (visible <= 0)
        return 0;
    if (sel < top)
        return sel;
    if (sel >= top + visible)
        return sel - visible + 1;
    return top;
}

void w_scrollbar(int x, int y, int h, int count, int visible, int top)
{
    int bar;
    int off;

    if (count <= visible || visible <= 0)
        return;
    scr_fill(x, y, 2, h, TH_SHADE);
    bar = h * visible / count;
    if (bar < 6)
        bar = 6;
    off = (h - bar) * top / (count - visible);
    scr_fill(x, y + off, 2, bar, TH_RED);
}

/* ---- fields ------------------------------------------------------------- */

void w_field(int x, int y, int w, int h, char const *label, char const *value,
             bool active)
{
    int lw = scr_text_w(label);

    scr_fill(x, y, w, h, active ? TH_YELLOW : TH_PAPER);
    scr_frame(x, y, w, h, TH_INK, active ? 2 : 1);
    scr_text(x + TH_PAD, y + (h - scr_text_h()) / 2, label, TH_INK);
    scr_text_ellipsis(x + TH_PAD + lw + TH_PAD, y + (h - scr_text_h()) / 2,
                      value, TH_INK,
                      w - lw - 3 * TH_PAD - (active ? scr_text_w("_") : 0));
    if (active)
    {
        int vw = scr_text_w(value);

        /* Ink, not red: red on the yellow active field is 2.9:1, which the
         * contrast check in tools/sim rejects. */
        scr_text(x + TH_PAD + lw + TH_PAD + vw, y + (h - scr_text_h()) / 2, "_",
                 TH_INK);
    }
}

/* ---- on-screen keyboard ------------------------------------------------- */

/* A-Z, space, delete, OK in a 7x5 grid. */
static char const keyboard_cells[35] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U',
    'V', 'W', 'X', 'Y', 'Z', ' ', '<',     /* '<' deletes */
    '>', 0, 0, 0, 0, 0, 0,                 /* '>' accepts */
};

#define KB_COLS 7
#define KB_ROWS 5

int w_keyboard(char *buf, int size, char const *title)
{
    int sel = 0;
    int len = (int)strlen(buf);

    for (;;)
    {
        int box_w = SCR_W - 2 * TH_PAD;
        int box_h = 2 * TH_PAD + TH_LINE + KB_ROWS * (TH_LINE + 2) + TH_LINE;
        int x = TH_PAD;
        int y = TH_CONTENT_Y + (TH_CONTENT_H - box_h) / 2;

        /* Modal: erase everything behind it.  Half a line of the note
         * showing above and below the panel read as a drawing bug. */
        scr_fill(0, TH_CONTENT_Y, SCR_W, TH_CONTENT_H, TH_PAPER);
        scr_fill(x, y, box_w, box_h, TH_PAPER);
        scr_frame(x, y, box_w, box_h, TH_INK, 2);
        scr_fill(x + 2, y + 2, box_w - 4, TH_TITLE_H - 4, TH_INK);
        scr_text(x + TH_PAD, y + 3, title ? title : "Search", TH_PAPER);

        /* The text so far, with a cursor. */
        {
            int ty = y + TH_TITLE_H + 2;
            int avail = box_w - 2 * TH_PAD - scr_text_w("_");

            scr_fill(x + 3, ty, box_w - 6, TH_LINE, TH_SHADE);
            scr_text_ellipsis(x + TH_PAD, ty + 1, len ? buf : "", TH_INK,
                              avail);
            scr_text(x + TH_PAD + (len ? scr_text_w(buf) : 0), ty + 1, "_",
                     TH_INK);
        }

        /* The grid. */
        {
            int gy = y + TH_TITLE_H + TH_LINE + 4;
            int cell_w = (box_w - 8) / KB_COLS;
            int cell_h = TH_LINE;

            for (int i = 0; i < KB_ROWS * KB_COLS; i++)
            {
                int r = i / KB_COLS;
                int c = i % KB_COLS;
                int cx = x + 4 + c * cell_w;
                int cy = gy + r * (cell_h + 2);
                char label[2] = { keyboard_cells[i], 0 };
                bool empty = keyboard_cells[i] == 0;

                if (empty)
                    continue;
                if (i == sel)
                    scr_fill(cx, cy, cell_w - 1, cell_h, TH_INK);
                /* The space key has no glyph to show; mark it with a bar so
                 * it does not read as an empty, broken cell. */
                if (keyboard_cells[i] == ' ')
                {
                    scr_hline(cx + cell_w / 4,
                              cy + cell_h - 2,
                              cell_w / 2 - 1,
                              i == sel ? TH_PAPER : TH_INK);
                    continue;
                }
                scr_text_center(cx, cell_w - 1, cy + 1,
                                keyboard_cells[i] == '<' ? "DEL"
                                : keyboard_cells[i] == '>' ? "OK" : label,
                                i == sel ? TH_PAPER : TH_INK);
            }
        }

        scr_text_center(0, SCR_W, y + box_h - TH_PAD - scr_text_h(),
                        "arrows move  EXE picks  EXIT back", TH_INK);
        scr_present();

        {
            bench_event_t ev = input_next();

            if (ev.key == BK_EXIT || ev.key == BK_MENU)
                return 0;
            if (ev.key == BK_LEFT)
                sel = (sel / KB_COLS) * KB_COLS + ((sel % KB_COLS) + KB_COLS - 1) % KB_COLS;
            else if (ev.key == BK_RIGHT)
                sel = (sel / KB_COLS) * KB_COLS + ((sel % KB_COLS) + 1) % KB_COLS;
            else if (ev.key == BK_UP)
                sel = (sel + KB_ROWS * KB_COLS - KB_COLS) % (KB_ROWS * KB_COLS);
            else if (ev.key == BK_DOWN)
                sel = (sel + KB_COLS) % (KB_ROWS * KB_COLS);
            else if (ev.key == BK_DEL)
            {
                if (len > 0)
                    buf[--len] = 0;
            }
            else if (ev.key == BK_EXE || ev.key == BK_DIGIT)
            {
                char c = ev.key == BK_DIGIT ? ev.ch : keyboard_cells[sel];

                if (c == '>')
                    return 1;
                if (c == '<')
                {
                    if (len > 0)
                        buf[--len] = 0;
                }
                else if (len < size - 1)
                {
                    buf[len++] = c;
                    buf[len] = 0;
                }
            }
        }
    }
}

/* ---- progress ----------------------------------------------------------- */

void w_progress(int x, int y, int w, int h, char const *label, int permille)
{
    int fill;

    if (permille < 0)
        permille = 0;
    if (permille > 1000)
        permille = 1000;
    fill = w * permille / 1000;

    scr_fill(x, y, w, h, TH_PAPER);
    scr_frame(x, y, w, h, TH_INK, 1);
    if (fill > 2)
        scr_fill(x + 1, y + 1, fill - 2, h - 2, TH_RED);
    if (label && *label)
        scr_text(x, y - scr_text_h() - 2, label, TH_INK);
}
