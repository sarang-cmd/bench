/* app_base.c - bases and bits (V2 feature F12).
 *
 * Spec: the V2 master prompt, F12.  One number in every base, and the same
 * number as a two's complement pattern; a second tab for the bitwise operators.
 *
 *      Base and bits                entry base 10
 *      value 255_
 *      dec 255   hex FF   oct 377
 *      8-bit   1111 1111  = FF
 *      16-bit  0000 0000 1111 1111 = 00FF
 *      32-bit  0000 ... 1111 1111 = 000000FF
 *
 * Keys
 *   digits      type the number in the entry base; A-F only above base 10
 *   LEFT/RIGHT  pick a field (bits tab: a or b)
 *   F1          convert / bits
 *   F3          entry base (2, 8, 10, 16, 36) or the operator
 *   F5 / EXIT   help / home
 *
 * Nothing is written to the card. */

#include "app.h"
#include "app_help.h"

#include <stdio.h>
#include <string.h>

#include "core/base.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum { BS_CONVERT = 0, BS_BITS, BS_STAGES } stage_t;

static stage_t stage = BS_CONVERT;
static char value[24] = "255";
static char left[24] = "12";
static char right[24] = "10";
static int entry_base = 10;
static int field;               /* bits tab: 0 = a, 1 = b */
static bit_op_t op = BIT_AND;

static int const BASES[] = { 2, 8, 10, 16, 36 };
#define BASE_COUNT ((int)(sizeof BASES / sizeof BASES[0]))

void app_base_init(void)
{
    stage = BS_CONVERT;
    strcpy(value, "255");
    strcpy(left, "12");
    strcpy(right, "10");
    entry_base = 10;
    field = 0;
    op = BIT_AND;
}

int app_base_timeout(void)
{
    return 0;
}

/* ---- drawing ------------------------------------------------------------ */

static void line(int x, int y, char const *label, char const *text)
{
    scr_text(x, y, label, TH_INK);
    scr_text(x + 48, y, text, TH_INK);
}

static void draw_convert(void)
{
    char buf[80], hex[24];
    int64_t v;
    int y = TH_CONTENT_Y + 6;
    bool ok = base_parse(value, entry_base, &v);

    scr_text(TH_PAD, y, "value", TH_INK);
    scr_text(TH_PAD + 48, y, value[0] ? value : "_", TH_INK);
    scr_fill(TH_PAD + 48, y + TH_LINE - 2,
             scr_text_w(value[0] ? value : "_"), 2, TH_RED);
    y += TH_LINE + 2;

    if (!ok)
    {
        scr_text(TH_PAD, y, "Not a number in this base. Digits: ", TH_INK);
        scr_text(TH_PAD, y + TH_LINE, base_digits(entry_base), TH_INK);
        return;
    }

    /* The same value in the bases a paper uses, entry base first. */
    {
        int const order[] = { 10, 16, 8, 2, 36 };
        char const *const names[] = { "dec", "hex", "oct", "bin", "b36" };

        for (int i = 0; i < 5; i++)
        {
            int const col = (i % 2) * (SCR_W / 2);

            if (!base_format(v, order[i], buf, (int)sizeof buf)) continue;
            line(TH_PAD + col, y + (i / 2) * (TH_LINE + 1), names[i], buf);
        }
        y += 3 * (TH_LINE + 1) + 4;
    }

    /* Two's complement, at the three widths the paper uses. */
    for (int bits = 8; bits <= 32; bits *= 2)
    {
        char tag[16];

        snprintf(tag, sizeof tag, "%d-bit", bits);
        scr_text(TH_PAD, y, tag, TH_INK);
        if (!base_twos(v, bits, buf, (int)sizeof buf))
        {
            scr_text(TH_PAD + 48, y, "does not fit", TH_RED);
        }
        else
        {
            base_twos_hex(v, bits, hex, (int)sizeof hex);
            snprintf(buf + strlen(buf), sizeof buf - strlen(buf), " = %s", hex);
            scr_text(TH_PAD + 48, y, buf, TH_INK);
        }
        y += TH_LINE;
    }

    {
        char one[48];

        int const n = base_popcount((uint64_t)v);

        snprintf(one, sizeof one, "%d bit%s set", n, n == 1 ? "" : "s");
        scr_text(TH_PAD, y + 2, one, TH_INK);
    }
}

static void draw_bits(void)
{
    char buf[80];
    int64_t a = 0, b = 0, r = 0;
    int y = TH_CONTENT_Y + 6;
    bool oka = base_parse(left, entry_base, &a);
    bool okb = base_parse(right, entry_base, &b);

    scr_text(TH_PAD, y, "a", TH_INK);
    scr_text(TH_PAD + 24, y, left[0] ? left : "_", TH_INK);
    if (field == 0)
        scr_fill(TH_PAD + 24, y + TH_LINE - 2,
                 scr_text_w(left[0] ? left : "_"), 2, TH_RED);
    scr_text(TH_PAD + SCR_W / 2, y, "b", TH_INK);
    scr_text(TH_PAD + SCR_W / 2 + 24, y, right[0] ? right : "_", TH_INK);
    if (field == 1)
        scr_fill(TH_PAD + SCR_W / 2 + 24, y + TH_LINE - 2,
                 scr_text_w(right[0] ? right : "_"), 2, TH_RED);
    y += TH_LINE + 4;

    snprintf(buf, sizeof buf, "operator  %s  (F3 changes it)", bit_op_name(op));
    scr_text(TH_PAD, y, buf, TH_INK);
    y += TH_LINE + 6;

    if (!oka || !okb)
    {
        scr_text(TH_PAD, y, "Type both numbers.", TH_INK);
        return;
    }

    if (!base_op(a, b, op, &r))
    {
        scr_text(TH_PAD, y, "A shift needs a count under 63.", TH_RED);
        return;
    }

    w_panel(TH_PAD, y, SCR_W - 2 * TH_PAD, 4 * TH_LINE + 12);
    base_format(a, 2, buf, (int)sizeof buf);
    line(TH_PAD + 8, y + 6, "a", buf);
    base_format(b, 2, buf, (int)sizeof buf);
    line(TH_PAD + 8, y + 6 + TH_LINE, "b", buf);

    {
        char hex[24];

        base_format(r, 10, buf, (int)sizeof buf);
        base_format(r, 16, hex, (int)sizeof hex);
        snprintf(buf + strlen(buf), sizeof buf - strlen(buf), "   hex %s", hex);
        line(TH_PAD + 8, y + 6 + 2 * TH_LINE, "=", buf);
        {
            int const n = base_popcount((uint64_t)r);

            snprintf(buf, sizeof buf, "%d bit%s set", n, n == 1 ? "" : "s");
        }
        scr_text(TH_PAD + 8, y + 6 + 3 * TH_LINE, buf, TH_INK);
    }
}

void app_base_draw(void)
{
    static char hint[24];

    scr_clear(TH_PAPER);
    snprintf(hint, sizeof hint, "entry base %d", entry_base);
    w_title("Base and bits", stage == BS_CONVERT ? hint : "operators");
    if (stage == BS_CONVERT)
        draw_convert();
    else
        draw_bits();

    w_softkeys((char const *const[6]){
                   stage == BS_CONVERT ? "Bits" : "Convert", "Type",
                   stage == BS_CONVERT ? "Base" : "Op",
                   stage == BS_CONVERT ? "" : "Swap", "", "" },
               -1);
    scr_present();
}

/* ---- keys --------------------------------------------------------------- */

static char *current_field(void)
{
    if (stage == BS_CONVERT) return value;
    return field == 0 ? left : right;
}

static int base_value_of(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'Z') return ch - 'A' + 10;
    return -1;
}

app_result_t app_base_key(bench_event_t ev)
{
    char *buf = current_field();

    switch (ev.key)
    {
    case BK_MENU:
        return APP_QUIT;
    case BK_F5:
        app_help_open(tool_by_id("base"));
        return APP_CONTINUE;
    case BK_F2:
        /* Letters need the on-screen keyboard: the calculator has no A-F keys
         * this add-in may rely on. */
        w_keyboard(buf, 24, "value");
        break;
    case BK_DIGIT:
        if (base_value_of(ev.ch) >= 0 && base_value_of(ev.ch) < entry_base
            && strlen(buf) < 20)
        {
            size_t const n = strlen(buf);

            buf[n] = ev.ch >= 'a' && ev.ch <= 'z' ? (char)(ev.ch - 32) : ev.ch;
            buf[n + 1] = 0;
        }
        break;
    case BK_MINUS:
        /* Two's complement of a negative number is half the point of the
         * screen, so a sign has to be typeable. */
        if (buf[0] == '-')
            memmove(buf, buf + 1, strlen(buf));
        else if (strlen(buf) < 20)
        {
            memmove(buf + 1, buf, strlen(buf) + 1);
            buf[0] = '-';
        }
        break;
    case BK_DEL:
        if (buf[0]) buf[strlen(buf) - 1] = 0;
        break;
    case BK_LEFT:
    case BK_RIGHT:
        if (stage == BS_BITS) field = 1 - field;
        break;
    case BK_UP:
    case BK_DOWN:
        if (stage == BS_BITS) field = 1 - field;
        break;
    case BK_F1:
        stage = stage == BS_CONVERT ? BS_BITS : BS_CONVERT;
        break;
    case BK_F3:
        if (stage == BS_CONVERT)
        {
            int i = 0;

            while (i < BASE_COUNT && BASES[i] != entry_base) i++;
            entry_base = BASES[(i + 1) % BASE_COUNT];
        }
        else
            op = (bit_op_t)((op + 1) % BIT_OPS);
        break;
    case BK_F4:
        if (stage == BS_BITS)
        {
            char tmp[24];

            strcpy(tmp, left);
            strcpy(left, right);
            strcpy(right, tmp);
        }
        break;
    case BK_EXIT:
        return APP_BACK;
    default:
        break;
    }
    return APP_CONTINUE;
}
