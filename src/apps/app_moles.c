/* app_moles.c - moles, solutions and gases (V2 feature F11).
 *
 * Spec: the V2 master prompt, F11.  Five calculations a chemistry paper asks
 * for, one per mode, each with the numbers typed and the working written out:
 *
 *      Moles                       n = m / Mr
 *      mass  4 g   Mr  40          n = 0.1 mol
 *      ------------------------------
 *      n = m / Mr = 4 / 40 = 0.1 mol
 *
 * Modes (F3 / F4, also shown in the title bar): moles, concentration, the gas
 * equation, dilution, yield and atom economy.
 *
 * Keys
 *   UP / DOWN   pick a field
 *   digits .    type the number, DEL takes a digit back
 *   F1          the mode's worked example again
 *   F3 / F4     previous / next mode
 *   F5 / EXIT   help / home
 *
 * Nothing is written to the card. */

#include "app.h"
#include "app_help.h"

#include <stdio.h>
#include <string.h>

#include "core/chem.h"
#include "core/numparse.h"
#include "platform/screen.h"
#include "theme.h"
#include "ui/numtext.h"
#include "ui/widgets.h"

typedef enum {
    MO_MOLES = 0,
    MO_CONC,
    MO_GAS,
    MO_DILUTE,
    MO_YIELD,
    MO_COUNT
} mole_mode_t;

static mole_mode_t mode = MO_MOLES;
static int field;
static char buf[4][12];

typedef struct {
    char const *label;
    char const *unit;
    char const *seed;
} slot_t;

typedef struct {
    char const *name;       /* title bar hint */
    char const *formula;    /* the equation, as text */
    slot_t slot[4];
    int slots;
} mode_info_t;

static mode_info_t const modes[MO_COUNT] = {
    [MO_MOLES] = {
        "moles", "n = m / Mr",
        { { "mass", "g", "4" }, { "Mr", "g/mol", "40" } }, 2
    },
    [MO_CONC] = {
        "concentration", "c = n / V",
        { { "n", "mol", "0.1" }, { "V", "dm3", "0.25" } }, 2
    },
    [MO_GAS] = {
        "gas equation", "n = p V / R T",
        { { "p", "kPa", "100" }, { "V", "dm3", "24" }, { "T", "K", "298" } }, 3
    },
    [MO_DILUTE] = {
        "dilution", "c2 = c1 V1 / V2",
        { { "c1", "mol/dm3", "0.5" }, { "V1", "cm3", "20" },
          { "V2", "cm3", "250" } }, 3
    },
    [MO_YIELD] = {
        "yield", "% = actual / theory",
        { { "actual", "g", "3.5" }, { "theory", "g", "5" } }, 2
    },
};

static void load_defaults(void)
{
    for (int i = 0; i < 4; i++)
        buf[i][0] = 0;
    for (int i = 0; i < modes[mode].slots; i++)
        snprintf(buf[i], sizeof buf[i], "%s", modes[mode].slot[i].seed);
    field = 0;
}

void app_moles_init(void)
{
    mode = MO_MOLES;
    load_defaults();
}

int app_moles_timeout(void)
{
    return 0;
}

/* ---- the maths ---------------------------------------------------------- */

static bool value_of(int i, double *out)
{
    if (!buf[i][0]) return false;
    return numparse_cstr(buf[i], out) == NUMPARSE_OK;
}

/* Works the answer out and writes the two lines the panel shows: the
 * substitution and the answer with its unit. */
static bool result_lines(char *step, int step_size, char *answer,
                         int ans_size, char *extra, int extra_size)
{
    double a = 0, b = 0, c = 0, r = 0;
    bool ok = false;

    switch (mode)
    {
    case MO_MOLES:
        ok = value_of(0, &a) && value_of(1, &b) && chem_moles(a, b, &r);
        snprintf(step, step_size, "n = m / Mr = %s / %s", buf[0], buf[1]);
        snprintf(answer, ans_size, "n = %.4g mol", r);
        snprintf(extra, extra_size, "N = n x 6.022e23 = %.3g particles", r * 6.02214076e23);
        break;
    case MO_CONC:
        ok = value_of(0, &a) && value_of(1, &b) && chem_conc(a, b, &r);
        snprintf(step, step_size, "c = n / V = %s / %s", buf[0], buf[1]);
        snprintf(answer, ans_size, "c = %.4g mol/dm3", r);
        snprintf(extra, extra_size, "that is %.4g mol in %.4g cm3", a,
                 chem_dm3_to_cm3(b));
        break;
    case MO_GAS:
        ok = value_of(0, &a) && value_of(1, &b) && value_of(2, &c)
             && chem_gas_moles(a, b, c, &r);
        snprintf(step, step_size, "n = pV / RT = %s x %s / (8.314 x %s)",
                 buf[0], buf[1], buf[2]);
        snprintf(answer, ans_size, "n = %.4g mol", r);
        snprintf(extra, extra_size, "R = 8.314 kPa dm3 / mol K, T in kelvin");
        break;
    case MO_DILUTE:
        ok = value_of(0, &a) && value_of(1, &b) && value_of(2, &c)
             && chem_dilute(a, b, c, &r);
        snprintf(step, step_size, "c2 = c1 V1 / V2 = %s x %s / %s",
                 buf[0], buf[1], buf[2]);
        snprintf(answer, ans_size, "c2 = %.4g mol/dm3", r);
        snprintf(extra, extra_size, "diluted %.4g times, so %.4g mol/dm3 weaker",
                 c / b, a / r);
        break;
    case MO_YIELD:
        ok = value_of(0, &a) && value_of(1, &b) && chem_yield(a, b, &r);
        snprintf(step, step_size, "%% = actual / theory = %s / %s x 100",
                 buf[0], buf[1]);
        snprintf(answer, ans_size, "yield = %.4g %%", r);
        snprintf(extra, extra_size, "%.4g g short of what the equation says",
                 b - a);
        break;
    default:
        ok = false;
        break;
    }
    return ok;
}

/* ---- drawing ------------------------------------------------------------ */

static void draw_field(int i, int y)
{
    slot_t const *s = &modes[mode].slot[i];
    char num[16];
    int const x = TH_PAD + (i % 2) * (SCR_W / 2);
    int const value_x = x + 74;
    int const wnum = scr_text_w(buf[i][0] ? buf[i] : "_");

    scr_text(x, y, s->label, TH_INK);
    strncpy(num, buf[i][0] ? buf[i] : "_", sizeof num - 1);
    num[sizeof num - 1] = 0;
    scr_text(value_x, y, num, TH_INK);
    scr_text(value_x + wnum + 4, y, s->unit, TH_INK);

    if (i == field)
        scr_fill(value_x, y + TH_LINE - 2, wnum, 2, TH_RED);
}

static void draw_screen(void)
{
    char step[64], answer[32], extra[64];
    int y = TH_CONTENT_Y + 6;
    int const row_h = TH_LINE + 8;
    bool ok;

    /* Two fields a row, three at most per mode: the panel stays in the top
     * half and the answer gets the room. */
    for (int i = 0; i < modes[mode].slots; i++)
        draw_field(i, y + (i / 2) * row_h);
    y += ((modes[mode].slots + 1) / 2) * row_h + 6;

    ok = result_lines(step, (int)sizeof step, answer, (int)sizeof answer,
                      extra, (int)sizeof extra);

    /* The panel is as tall as what is in it, not as tall as the screen. */
    w_panel(TH_PAD, y, SCR_W - 2 * TH_PAD, 3 * TH_LINE + 14);
    if (ok)
    {
        scr_text(TH_PAD + 8, y + 6, step, TH_INK);
        scr_text(TH_PAD + 8, y + 6 + TH_LINE + 2, answer, TH_BLUE);
        scr_text(TH_PAD + 8, y + 6 + 2 * TH_LINE + 4, extra, TH_INK);
    }
    else
    {
        scr_text(TH_PAD + 8, y + 6, modes[mode].formula, TH_INK);
        scr_text(TH_PAD + 8, y + 6 + TH_LINE + 2, "Fill every field.", TH_INK);
        scr_text(TH_PAD + 8, y + 6 + 2 * TH_LINE + 4, "F1 puts the example back.",
                 TH_INK);
    }

    scr_text(TH_PAD, y + 3 * TH_LINE + 22,
             "UP/DOWN pick a field, F3/F4 change mode", TH_INK);
}

void app_moles_draw(void)
{
    scr_clear(TH_PAPER);
    w_title("Moles", modes[mode].name);
    draw_screen();
    w_softkeys((char const *const[6]){ "Example", "", "Mode -", "Mode +", "",
                                       "" }, -1);
    scr_present();
}

/* ---- keys --------------------------------------------------------------- */

static void type_char(char ch)
{
    size_t const n = strlen(buf[field]);

    if (n + 1 >= sizeof buf[field]) return;
    buf[field][n] = ch;
    buf[field][n + 1] = 0;
}

app_result_t app_moles_key(bench_event_t ev)
{
    int const slots = modes[mode].slots;

    switch (ev.key)
    {
    case BK_MENU:
        return APP_QUIT;
    case BK_F5:
        app_help_open(tool_by_id("moles"));
        return APP_CONTINUE;
    case BK_UP:
        field = (field + slots - 1) % slots;
        break;
    case BK_DOWN:
        field = (field + 1) % slots;
        break;
    case BK_DIGIT:
        type_char(ev.ch);
        break;
    case BK_DOT:
        type_char('.');
        break;
    case BK_DEL:
        if (buf[field][0])
            buf[field][strlen(buf[field]) - 1] = 0;
        break;
    case BK_F1:
        load_defaults();
        break;
    case BK_F3:
        mode = (mole_mode_t)((mode + MO_COUNT - 1) % MO_COUNT);
        load_defaults();
        break;
    case BK_F4:
        mode = (mole_mode_t)((mode + 1) % MO_COUNT);
        load_defaults();
        break;
    case BK_EXIT:
        return APP_BACK;
    default:
        break;
    }
    return APP_CONTINUE;
}
