/* settings.h - the settings file, parsed and formatted without gint.
 *
 * Spec: docs/plan.md sections 4, 5 and 6.2 (the significant figure setting).
 *
 * The file is plain text, one "key value" pair per line:
 *
 *     # Bench settings v1
 *     uncert_sig 1
 *     pacer_checkpoint 10
 *
 * Rules, so a hand-edited file never stops the add-in:
 *   - unknown keys are ignored, blank lines and '#' comments are skipped
 *   - a value that is not a number, or is out of range, is clamped/ignored and
 *     counted, so the settings screen can say "2 lines ignored"
 *   - the file is only written when a value actually changes
 *
 * Plain C11, no gint. */

#ifndef BENCH_SETTINGS_H
#define BENCH_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#define SETTINGS_VERSION 1
#define SETTINGS_BUFSIZE 224

/* Uncertainties are shown with 1 significant figure by default; the second
 * option is the plan's "use 2 when the first digit is 1". */
#define SETTINGS_CATEGORY_COUNT 4
#define SETTINGS_WARN_MIN 0
#define SETTINGS_WARN_MAX 30
#define SETTINGS_UNCERT_SIG_MIN 1
#define SETTINGS_UNCERT_SIG_MAX 2
#define SETTINGS_CHECKPOINT_MIN 1
#define SETTINGS_CHECKPOINT_MAX 200

typedef struct {
    uint8_t uncert_sig;        /* 1 or 2 significant figures for the uncertainty */
    uint8_t pacer_checkpoint;  /* marks between checkpoints, default 10 */
    uint8_t notes_wrap;        /* 0 = wrap to screen width, else percent (50..150) */
    uint8_t angle_unit;        /* SETTINGS_ANGLE_DEG or SETTINGS_ANGLE_RAD */
    uint8_t tour_done;         /* 1 once the first-run hint has been read */
    uint8_t startup_cat;       /* category the home screen opens on, 0 = Study */
    uint8_t show_hint;         /* 1 to show the first-run hint line at all */
    uint8_t pacer_warn;        /* minutes left when the clock turns red, 0 = never */
    uint8_t font_scale;        /* 1 = standard text, 2 = large (readability) */
} bench_settings_t;

#define SETTINGS_FONT_MIN 1
#define SETTINGS_FONT_MAX 2

#define SETTINGS_ANGLE_DEG 0
#define SETTINGS_ANGLE_RAD 1

/* Everything at its documented default. */
void settings_defaults(bench_settings_t *s);

/* Parse a settings file.  Never fails: a line that cannot be used is counted in
 * *ignored (may be NULL).  Returns the number of keys applied, or -1 when the
 * arguments are bad. */
int settings_parse(char const *text, int len, bench_settings_t *s, int *ignored);

/* Serialise.  Returns the length written (excluding the NUL), or -1 when the
 * buffer is too small.  The output ends with a newline. */
int settings_format(bench_settings_t const *s, char *buf, int size);

/* Clamp a value into its allowed range.  Used by both the parser and the
 * settings screen. */
uint8_t settings_clamp_uncert_sig(int v);
uint8_t settings_clamp_checkpoint(int v);
uint8_t settings_clamp_wrap(int v);
uint8_t settings_clamp_angle(int v);
uint8_t settings_clamp_startup_cat(int v);
uint8_t settings_clamp_warn(int v);
uint8_t settings_clamp_flag(int v);
uint8_t settings_clamp_font(int v);

/* True when two settings structures hold the same values (the app only writes
 * the file when this is false). */
bool settings_equal(bench_settings_t const *a, bench_settings_t const *b);

/* Human text for a setting, for the settings screen. */
char const *settings_uncert_sig_text(uint8_t v);
char const *settings_angle_text(uint8_t v);
char const *settings_font_text(uint8_t v);

/* "5 min", "off" - the pacer warning, as the settings row shows it. */
char const *settings_warn_text(uint8_t v, char *buf, int size);

#endif /* BENCH_SETTINGS_H */
