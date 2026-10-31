# Status

Last updated: 31 October 2026, end of the build session that took the project
from the first line of the core to version 1.0.0 in the source tree.
A fresh session should read this file first, then `AGENTS.md`, `docs/plan.md`
and the task file for the milestone it is about to start.

## One sentence

All of Bench is written, tested on a PC and documented; **no `.g3a` exists yet**,
because this sandbox has no SuperH compiler and cannot reach
`git.planet-casio.com`. `.github/workflows/build.yml` builds the file on a
GitHub runner, which is where the release asset comes from.

## Where the project is

| Stage | Task | State |
|-------|------|-------|
| 1 | M0 toolchain + blank add-in | source done, `.g3a` built by CI |
| 2 | M1 spike add-in | source done; `docs/spike.md` still has an empty result column |
| 3 | core logic, no hardware | **done**, 11 modules, 2 791 checks |
| 4 | M2 shell + settings | **done** (`src/platform`, `src/ui`, `src/apps/app_menu.c`) |
| 5 | M3..M8 wiring | **done**, all five tools and About |
| 6 | M9 polish and release | **done in source**: icons, About screen, README, changelog, tags |

The milestone gate in the task files ("start only after the user confirms the
previous milestone passed on the calculator") was waived by the user's
instruction to finish every stage in one go. The gates therefore hang on the
spike answers now: see "Open questions" below, and expect to change the four
items under "Assumptions that still need the calculator" when they arrive.

## Where the task files are

They shipped in `agent/tasks/`, not `docs/tasks/`. They are copied to
`docs/tasks/`; `agent/` is left untouched so the original pack stays intact.
`AGENTS.md` and `docs/plan.md` were copied from `agent/` to the repository root
and `docs/`, as the pack's README describes.

## Tests

`make test`: gcc, `-Wall -Wextra -Wshadow -Wstrict-prototypes
-Wmissing-prototypes -Werror -pedantic`, no gint.

| Test program | Module | Checks |
|--------------|--------|-------:|
| test_pacer | pacer | 114 |
| test_numfmt | numfmt | 182 |
| test_numparse | numparse | 62 |
| test_uncert | uncert | 144 |
| test_cards | cards_parse | 95 |
| test_leitner | leitner | 126 |
| test_textsearch | textsearch | 84 |
| test_md_parse | md_parse | 203 |
| test_md_layout | md_layout | 95 |
| test_numtheory | numtheory | 1027 |
| test_settings | settings | 659 |
| **total** | | **2791 checks, 0 failures** |

- `make py-test`: 28 unittest cases for `tools/md2bench.py`, all pass.
- `make sim`: the whole add-in (platform, ui, apps) compiles against
  `tools/sim`, a stand-in for gint, and runs.
- `make shots`: 26 screens rendered to `docs/images`.
- `make gint-check GINT_INC=/path/to/gint/include`: every `src/` file
  type-checks against the real gint 2.10.0 headers with `-Werror`. This is the
  strongest check available without the cross compiler, and it passes.
- ASan + UBSan sweep of the nine original core programs: clean.
- Timing on the PC (guide only): factor a 1e9 x 1e9 semiprime 33 ms, factor a
  prime near 1e18 1 ms. The calculator will be 30-100x slower, which is why
  factorisation has a progress bar and an EXIT cancel.

## Files

```
src/core/       cards_parse leitner md_layout md_parse numfmt numparse
                numtheory pacer settings textsearch uncert   (+ .h each)
src/platform/   screen input store mem                        (+ .h each)
src/ui/         theme.h widgets shapes numtext                (+ .c/.h)
src/apps/       app.h menu pacer uncert cards notes numtheory settings about
src/main.c      router, settings owner, main loop
src/version.h   BENCH_VERSION, kept in step with version.txt
tests/          test_util.h + eleven test programs
tools/sim/      gint stand-in, key scripts (tour/), run_tour.sh, ppm2png.py,
                make_font.py -> sim_font.h
tools/brand/    make_brand.py -> the two icons, wordmark, social card
tools/          md2bench.py + test_md2bench.py
spike/          the M1 add-in (throwaway, kept for the answers)
assets-cg/      icon-uns.png, icon-sel.png, fxconv-metadata.txt
docs/           plan, tasks/, toolchain, spike, STATUS, samples/, images/
site/           landing page (index.html, style.css), old guide kept
.github/        workflows/build.yml  (native tests, preview, .g3a, release)
```

## Screens and keys

Screens (all in `make shots` output, `docs/images/`):

1. home: six tiles, F5 Settings, F6 About, MENU leaves.
2. pacer: setup, plan, run, done.
3. uncertainty: A, operation, B, result.
4. cards: deck list, deck report, question, answer, summary, boxes.
5. notes: file list, view, headings, on-screen keyboard.
6. number theory: function list, form, progress, result.
7. settings: three rows.
8. about: version, build date, geometry, heap, limits.

Keys are listed per screen in the header comment of each `src/apps/*.c` file and
in one table in `README.md`.

## Files the add-in reads or writes

Reads: `Bench/notes/*` (32 KiB cap), `Bench/cards/*.txt` (64 KiB cap),
`Bench/state/cards.sav`, `Bench/state/settings.cfg`.
Writes: `Bench/state/cards.sav` (once, at the end of a session) and
`Bench/state/settings.cfg` (only when a value changes). Creates the folder tree
once at start-up. Nothing else, ever.

## What needs the calculator

1. Build (or let CI build) `Bench-1.0.0.g3a`; report its size and whether the
   icon and the name sit correctly in the main menu.
2. Run the M0 check: full window in paper colour, centred red square, any key
   back to the menu.
3. Run the spike (`spike/`, `docs/spike.md`) and fill in the ten answers.
4. Every tool on the device: Pacer clock drift over 30 minutes, Uncertainty
   display widths, Cards progress surviving a relaunch, Notes scroll speed on a
   30 KiB note, Number theory factorisation time and the EXIT cancel.
5. Exam Mode: the user's own test. The add-in must simply not appear.

## Assumptions that still need the calculator

Each of these is coded to the safest option and will be revisited when the
spike answers land:

| # | Assumption in the code | If the spike says otherwise |
|---|------------------------|-----------------------------|
| 1 | BFile files are 0xFF padded and `BFile_Size` counts the allocation, so `store_read` trims trailing 0xFF and NUL | trim less, or not at all |
| 2 | A file is written once, after a `BFile_Remove` + `BFile_Create`, then renamed over the target | rewrite in place if the medium allows it |
| 3 | No float printf anywhere: numbers go through `numfmt` | nothing changes; the module stays |
| 4 | The default font covers ASCII only, so anything above 0x7E is a `?` | widen `scr_has_glyph`, and the footer count drops |
| 5 | ALPHA may not map to letters, so text entry uses the on-screen keyboard | the keyboard can stay; ALPHA could feed it |
| 6 | `rtc_ticks()` is the clock; the wrap at midnight is handled by unsigned differences | no change needed either way |
| 7 | 32 KiB note cap and 500-card decks fit the heap | raise the caps; both are single `#define`s |
| 8 | A frame costs roughly 10-20 ms, so Pacer redraws at 4 Hz | adjust `app_pacer_timeout()` |

## Open questions for the user

1. **Toolchain.** The repository now builds the `.g3a` in CI. If you would
   rather build in WSL, `docs/toolchain.md` has the steps; either way the first
   hardware run is yours.
2. **Fractional exponents in Uncertainty.** Whole exponents and ±0.5 work
   without libm; 0.25 or 1.7 return an error because they need `pow()` and the
   build does not know whether libm is linked. Keep the restriction, or assume
   libm?
3. **Decks over 500 cards.** One `deck full` line per extra card could mean 100
   warnings. Is a single summary line enough?
4. **The two settings that need a real run.** `notes_wrap` (percentage width)
   and `uncert_sig` are shipped with defaults; tell me if the defaults feel
   wrong on the device.

## Next task

None pending from the milestone list: M0 to M9 are all written. The next real
step is the hardware pass in "What needs the calculator", then, if the spike
answers change anything, TASK-M2's file layer plus the eight assumptions above.
