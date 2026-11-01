/* app_cards.c - the Cards tool (Leitner drill).
 *
 * Spec: docs/plan.md section 6.3, TASK-M5.
 *
 * Screens
 *   list     decks found in Bench/cards, with card counts and file sizes
 *   deck     issues in the deck file (bad lines, truncated sides), F1 start
 *   session  question, EXE reveals the answer, F1 right, F2 wrong
 *   summary  right/wrong counts, F2 shows the box statistics
 *   stats    cards per box for this deck
 *
 * Keys
 *   list     UP/DOWN pick, EXE open, EXIT back
 *   deck     F1 start, EXIT back
 *   session  EXE reveal, F1 right, F2 wrong, EXIT abandon the session
 *   summary  F2 stats, EXE back to the list
 *
 * Files read:   cards/<name>.txt, state/cards.sav
 * Files written: state/cards.sav (once, when a session ends) */

#include "app.h"
#include "app_help.h"

#include <string.h>

#include "core/cards_parse.h"
#include "core/leitner.h"
#include "platform/mem.h"
#include "platform/screen.h"
#include "platform/store.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

#define CARDS_STATE_PATH "state/cards.sav"
#define DECK_FILE_MAX 65536
#define ISSUE_MAX 32

typedef enum { SC_LIST, SC_DECK, SC_SESSION, SC_SUMMARY, SC_STATS } screen_t;

static screen_t screen;
static store_entry_t decks[STORE_LIST_MAX];
static int deck_count;
static int deck_sel;
static int deck_top;

static deck_t *deck;
static uint8_t *boxes;
static int *order;
static leitner_state_t *state;
static int session_cards;
static int session_at;
static bool revealed;
static int right_count;
static int wrong_count;
static int issue_count;
static char issues[ISSUE_MAX][40];
static int issue_lines;
static bool shuffle;   /* mixed order inside a box (Cards v2) */
static uint32_t session_id;
static char message[64];

static void free_deck(void)
{
    mem_free(deck);
    mem_free(boxes);
    mem_free(order);
    mem_free(state);
    deck = NULL;
    boxes = NULL;
    order = NULL;
    state = NULL;
}

void app_cards_init(void)
{
    free_deck();
    screen = SC_LIST;
    deck_sel = 0;
    deck_top = 0;
    issue_count = 0;
    message[0] = 0;
    deck_count = store_list("cards", ".txt", decks, STORE_LIST_MAX);
    if (deck_count < 0)
        deck_count = 0;
}

int app_cards_timeout(void)
{
    return 0;
}

/* ---- loading ------------------------------------------------------------ */

static bool load_deck(void)
{
    char path[STORE_NAME_MAX + 8];
    char *text;
    int len = 0;
    int issues_total = 0;
    cards_issue_t found[ISSUE_MAX];
    int n;

    free_deck();
    issue_count = 0;
    issue_lines = 0;

    if (store_join(path, (int)sizeof path, "cards", decks[deck_sel].name) !=
        STORE_OK)
        return false;

    text = mem_alloc(DECK_FILE_MAX);
    if (!text)
    {
        strcpy(message, "Not enough memory for this deck");
        return false;
    }

    n = store_read(path, text, DECK_FILE_MAX - 1, &len);
    if (n != STORE_OK)
    {
        mem_free(text);
        strcpy(message, n == STORE_ERR_TOO_BIG ? "Deck file is over 64 KiB"
                                               : "Could not read the deck");
        return false;
    }

    deck = mem_alloc(sizeof(deck_t));
    boxes = mem_alloc(CARDS_MAX);
    order = mem_alloc(sizeof(int) * (size_t)CARDS_MAX);
    state = mem_alloc(sizeof(leitner_state_t));

    if (!deck || !boxes || !order || !state)
    {
        mem_free(text);
        free_deck();
        strcpy(message, "Not enough memory for this deck");
        return false;
    }

    cards_parse(text, len, deck, found, ISSUE_MAX, &issues_total);
    mem_free(text);

    for (int i = 0; i < deck->count; i++)
        boxes[i] = 1;

    /* Progress file. */
    leitner_state_init(state);
    {
        static char sbuf[4096];
        int slen = 0;

        if (store_read(CARDS_STATE_PATH, sbuf, (int)sizeof sbuf - 1, &slen) ==
            STORE_OK)
        {
            sbuf[slen] = 0;
            leitner_state_parse(sbuf, slen, state);
        }
    }
    for (int i = 0; i < deck->count; i++)
        boxes[i] = leitner_state_box(state, deck->card[i].id);
    session_id = state->sessions;

    /* Issues, at most what the panel can show. */
    issue_count = issues_total < ISSUE_MAX ? issues_total : ISSUE_MAX;
    for (int i = 0; i < issue_count; i++)
    {
        char num[12];

        nt_u64((uint64_t)found[i].line, num, (int)sizeof num);
        strcpy(issues[i], "line ");
        nt_append(issues[i], (int)sizeof issues[i], num);
        nt_append(issues[i], (int)sizeof issues[i], ": ");
        nt_append(issues[i], (int)sizeof issues[i],
                  cards_issue_text(found[i].kind));
    }
    issue_lines = issues_total;

    message[0] = 0;
    return deck->count > 0;
}

/* ---- saving ------------------------------------------------------------- */

static void save_state(void)
{
    static char buf[LEITNER_MAX_ENTRIES * 24 + 64];
    int n;

    if (!state)
        return;
    leitner_end_session(state);
    n = leitner_state_format(state, buf, (int)sizeof buf);
    if (n > 0)
        store_write(CARDS_STATE_PATH, buf, n);
}

/* ---- drawing ------------------------------------------------------------ */

static void draw_list(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int rows = w_rows_in(TH_CONTENT_H - 3 * TH_LINE);
    char line[64];
    char num[16];

    if (deck_count == 0)
    {
        if (store_elsewhere_exists("cards"))
            w_message("Decks are in SAVE-F",
                      "Move SAVE-F/Bench/cards to Bench/cards");
        else
            w_message("No decks yet",
                      "Put .txt decks in Bench/cards on the card root");
        scr_text_center(0, SCR_W, TH_CONTENT_Y + TH_CONTENT_H / 2 + TH_LINE,
                        "One card per line: question | answer", TH_INK);
        w_strip(NULL, NULL);
        scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                        "EXIT back", TH_INK);
        return;
    }

    scr_text(TH_PAD, y, "Decks in Bench/cards", TH_INK);
    y += TH_LINE + 2;
    deck_top = w_scroll_follow(deck_sel, deck_top, rows);
    for (int i = 0; i < rows && deck_top + i < deck_count; i++)
    {
        int idx = deck_top + i;

        nt_u64((uint64_t)decks[idx].size / 1024u, num, (int)sizeof num);
        strcpy(line, num);
        nt_append(line, (int)sizeof line, " KiB");
        w_row(TH_PAD, y + i * TH_LINE, SCR_W - 2 * TH_PAD, decks[idx].name,
              line, idx == deck_sel);
    }
    w_scrollbar(SCR_W - TH_PAD - 2, y, rows * TH_LINE, deck_count, rows,
                deck_top);

    if (message[0])
        scr_text_ellipsis(TH_PAD, SCR_H - TH_SOFT_H - TH_LINE - 2, message,
                          TH_RED, SCR_W - 2 * TH_PAD);

    w_softkeys((char const *const[6]){ "Open", "", "", "", "", "" }, -1);
}

static void draw_deck(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    char line[64];
    char num[16];
    int rows = w_rows_in(TH_CONTENT_H - 8 * TH_LINE);

    nt_u64((uint64_t)deck->count, num, (int)sizeof num);
    strcpy(line, "Cards: ");
    nt_append(line, (int)sizeof line, num);
    scr_text(TH_PAD, y, line, TH_INK);
    y += TH_LINE;

    nt_u64((uint64_t)issue_lines, num, (int)sizeof num);
    strcpy(line, "Problem lines: ");
    nt_append(line, (int)sizeof line, num);
    scr_text(TH_PAD, y, line, issue_lines ? TH_INK : TH_INK);
    y += TH_LINE + 2;

    for (int i = 0; i < issue_count && i < rows; i++)
        scr_text_ellipsis(TH_PAD + 6, y + i * TH_LINE, issues[i], TH_INK,
                          SCR_W - 2 * TH_PAD - 6);
    y += rows * TH_LINE;

    if (deck->count == 0)
        scr_text(TH_PAD, y, "Nothing to drill in this deck.", TH_INK);
    else
        scr_text(TH_PAD, y, shuffle ? "Order: mixed inside each box"
                                    : "Order: deck order, box by box", TH_INK);

    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "F1 start   F2 mixed order   EXIT back", TH_INK);
    w_softkeys((char const *const[6]){ "Start", "Mix", "", "", "", "" }, -1);
}

static void draw_session(void)
{
    int y = TH_CONTENT_Y + TH_PAD;
    int w = SCR_W - 2 * TH_PAD;
    char num[16];
    char line[32];
    card_t const *c;
    int idx;

    if (session_at >= session_cards)
        return;
    idx = order[session_at];
    c = &deck->card[idx];

    nt_u64((uint64_t)(session_at + 1), num, (int)sizeof num);
    strcpy(line, num);
    nt_append(line, (int)sizeof line, "/");
    nt_u64((uint64_t)session_cards, num, (int)sizeof num);
    nt_append(line, (int)sizeof line, num);
    scr_text_right(SCR_W - TH_PAD, y - 2, line, TH_INK);

    scr_text(TH_PAD, y, "Question", TH_INK);
    y += TH_LINE + 2;
    scr_fill(TH_PAD, y, w, TH_LINE * 3 + 8, TH_PAPER);
    scr_frame(TH_PAD, y, w, TH_LINE * 3 + 8, TH_INK, 2);
    scr_text_ellipsis(TH_PAD + 4, y + 4, c->q, TH_INK, w - 8);
    y += TH_LINE * 3 + 14;

    if (revealed)
    {
        scr_text(TH_PAD, y, "Answer", TH_INK);
        y += TH_LINE + 2;
        scr_fill(TH_PAD, y, w, TH_LINE * 2 + 8, TH_YELLOW);
        scr_frame(TH_PAD, y, w, TH_LINE * 2 + 8, TH_INK, 2);
        scr_text_ellipsis(TH_PAD + 4, y + 4, c->a, TH_INK, w - 8);
    }
    else
    {
        scr_text(TH_PAD, y, "EXE to reveal the answer", TH_INK);
    }

    w_softkeys((char const *const[6]){ revealed ? "Right" : "", "",
                                       revealed ? "Wrong" : "", "", "", "" },
               -1);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    revealed ? "F1 right   F2 wrong   EXIT stop"
                             : "EXE reveal   EXIT stop",
                    TH_INK);
}

static void draw_summary(void)
{
    int y = TH_CONTENT_Y + TH_PAD + 2;
    char line[64];
    char num[16];

    scr_text(TH_PAD, y, "Session finished", TH_INK);
    y += TH_LINE + 4;

    nt_u64((uint64_t)right_count, num, (int)sizeof num);
    strcpy(line, "Right  ");
    nt_append(line, (int)sizeof line, num);
    scr_text(TH_PAD, y, line, TH_BLUE);
    y += TH_LINE;

    nt_u64((uint64_t)wrong_count, num, (int)sizeof num);
    strcpy(line, "Wrong  ");
    nt_append(line, (int)sizeof line, num);
    scr_text(TH_PAD, y, line, TH_INK);
    y += TH_LINE + 2;

    scr_text(TH_PAD, y, "Progress saved to state/cards.sav", TH_INK);
    y += TH_LINE;
    scr_text(TH_PAD, y, "Boxes follow the Leitner rules: right moves", TH_INK);
    scr_text(TH_PAD, y + TH_LINE, "up one box, wrong returns to box 1.",
             TH_INK);

    w_softkeys((char const *const[6]){ "", "Boxes", "", "", "", "" }, -1);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "F2 boxes   EXE back to the list", TH_INK);
}

static void draw_stats(void)
{
    int y = TH_CONTENT_Y + TH_PAD + 2;
    int counts[LEITNER_BOXES + 1] = { 0, 0, 0, 0 };
    char line[64];
    char num[16];

    for (int i = 0; i < deck->count; i++)
    {
        uint8_t b = leitner_clamp(boxes[i]);

        counts[b]++;
    }

    scr_text(TH_PAD, y, "Boxes in this deck", TH_INK);
    y += TH_LINE + 4;
    for (int b = 1; b <= LEITNER_BOXES; b++)
    {
        nt_u64((uint64_t)counts[b], num, (int)sizeof num);
        strcpy(line, "Box ");
        nt_u64((uint64_t)b, num, (int)sizeof num);
        nt_append(line, (int)sizeof line, num);
        nt_append(line, (int)sizeof line, ": ");
        nt_u64((uint64_t)counts[b], num, (int)sizeof num);
        nt_append(line, (int)sizeof line, num);
        nt_append(line, (int)sizeof line, " cards");
        scr_text(TH_PAD, y, line, TH_INK);
        y += TH_LINE;
    }
    y += 4;
    nt_u64((uint64_t)state->sessions, num, (int)sizeof num);
    strcpy(line, "Sessions run: ");
    nt_append(line, (int)sizeof line, num);
    scr_text(TH_PAD, y, line, TH_INK);
    y += TH_LINE;
    scr_text(TH_PAD, y, "Box 2 is drawn on odd sessions, box 3 every", TH_INK);
    scr_text(TH_PAD, y + TH_LINE, "fourth session.", TH_INK);

    w_strip(NULL, NULL);
    scr_text_center(0, SCR_W, SCR_H - TH_SOFT_H - TH_LINE - 2,
                    "EXIT back to the summary", TH_INK);
}

/* ---- entry points ------------------------------------------------------- */

void app_cards_draw(void)
{
    scr_clear(TH_PAPER);
    w_title("Cards", screen == SC_SESSION ? "F1 right" : "");

    switch (screen)
    {
    case SC_LIST: draw_list(); break;
    case SC_DECK: draw_deck(); break;
    case SC_SESSION: draw_session(); break;
    case SC_SUMMARY: draw_summary(); break;
    default: draw_stats(); break;
    }
    scr_present();
}

static void start_session(void)
{
    session_cards = leitner_session_build(boxes, deck->count, session_id,
                                          order, CARDS_MAX);
    session_at = 0;
    revealed = false;
    right_count = 0;
    wrong_count = 0;
    if (session_cards <= 0)
    {
        /* Nothing due: drill the whole deck so the screen is never empty. */
        for (int i = 0; i < deck->count && i < CARDS_MAX; i++)
            order[i] = i;
        session_cards = deck->count;
    }
    if (shuffle)
        leitner_shuffle(order, session_cards, session_id * 2654435761u + 1u);
    screen = SC_SESSION;
}

static void answer(bool right)
{
    uint32_t id = deck->card[order[session_at]].id;

    leitner_apply_result(state, id, right);
    boxes[order[session_at]] = leitner_state_box(state, id);
    if (right)
        right_count++;
    else
        wrong_count++;

    if (++session_at >= session_cards)
    {
        save_state();
        screen = SC_SUMMARY;
    }
    else
        revealed = false;
}

app_result_t app_cards_key(bench_event_t ev)
{
    if (ev.key == BK_F5)
    {
        app_help_open(tool_by_id("cards"));
        return APP_CONTINUE;
    }
    if (ev.key == BK_MENU)
        return APP_QUIT;

    switch (screen)
    {
    case SC_LIST:
        if (ev.key == BK_EXIT)
            return APP_BACK;
        if (ev.key == BK_UP && deck_sel > 0)
            deck_sel--;
        else if (ev.key == BK_DOWN && deck_sel + 1 < deck_count)
            deck_sel++;
        else if (ev.key == BK_EXE && deck_count > 0)
        {
            if (load_deck())
                screen = SC_DECK;
            else if (!message[0])
                strcpy(message, "No usable cards in that file");
        }
        break;

    case SC_DECK:
        if (ev.key == BK_EXIT)
        {
            free_deck();
            screen = SC_LIST;
        }
        else if (ev.key == BK_F1)
            start_session();
        else if (ev.key == BK_F2)
            shuffle = !shuffle;
        break;

    case SC_SESSION:
        if (ev.key == BK_EXIT)
        {
            if (right_count + wrong_count > 0)
                save_state();
            screen = SC_LIST;
            free_deck();
            app_cards_init();
        }
        else if (ev.key == BK_EXE)
            revealed = true;
        else if (ev.key == BK_F1 && revealed)
            answer(true);
        else if (ev.key == BK_F2 && revealed)
            answer(false);
        break;

    case SC_SUMMARY:
        if (ev.key == BK_F2)
            screen = SC_STATS;
        else if (ev.key == BK_EXE || ev.key == BK_EXIT)
        {
            free_deck();
            app_cards_init();
            screen = SC_LIST;
        }
        break;

    default:
        if (ev.key == BK_EXIT || ev.key == BK_EXE)
            screen = SC_SUMMARY;
        break;
    }
    return APP_CONTINUE;
}
