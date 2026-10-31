# Status

Last updated: 31 October 2026, end of the build session that took the project
from the first line of the core to version 1.0.0 in the source tree.
A fresh session should read this file first, then `AGENTS.md`, `docs/plan.md`
and the task file for the milestone it is about to start.

## V2 progress

The V2 master prompt is a multi-session job: eleven workstreams, fifteen new
tools, a WebAssembly simulator and an eight-page site. What is done so far, and
what is not, is in the table at the end of this file.

## One sentence

Bench is finished, tested on a PC, documented **and built**: `v1.0.0` carries
`Bench-1.0.0.g3a` (128 976 bytes) as a release asset, on the `builds` branch and
on the website. `.github/workflows/build.yml` is the only thing that can build
it - this sandbox has no SuperH compiler and cannot reach
`git.planet-casio.com` - and the toolchain it needs is cached, so a release
takes about two and a half minutes.

The one thing still open is hardware: nobody has run the add-in on a real
fx-CG 50 yet. That is the user's step (USB Flash mode, copy to the storage
root); see `docs/STATUS.md` "What needs the calculator".

## Where the project is

| Stage | Task | State |
|-------|------|-------|
| 1 | M0 toolchain + blank add-in | **done**: `.g3a` built by CI, release v1.0.0 |
| 2 | M1 spike add-in | source done; `docs/spike.md` still has an empty result column |
| 3 | core logic, no hardware | **done**, 11 modules, 2 791 checks |
| 4 | M2 shell + settings | **done** (`src/platform`, `src/ui`, `src/apps/app_menu.c`) |
| 5 | M3..M8 wiring | **done**, all five tools and About |
| 6 | M9 polish and release | **done**: icons, About screen, README, changelog, tag v1.0.0 + release asset |

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
site/           landing page, React + Vite + Tailwind (source in site/src,
                screenshots copied from docs/images at build time)
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

1. Install `Bench-1.0.0.g3a` (release asset, `builds` branch or the website) and
   report whether the icon and the name sit correctly in the main menu.
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


## V2 workstream state

| Workstream | State |
|---|---|
| A1 tile colour bug | **done**: every foreground measured against its fill, icon outlined, check in `make test` |
| A2 tool registry | **done**: `src/apps/registry.c` is the only list (10 rows); `docs/tools.json` is generated from it and CI diffs it |
| A3 home categories and pages | **done in source**, unverified on device: F1..F4 categories, 3 x 2 grid, page dots, Study dashboard (deck and note counts) |
| A4 design system | **done**: `docs/design.md`, `theme.h`, shapes, icons, tile anatomy |
| A5 shared services | **partly**: help is done (screen, key, paging, tests) and `core/mathfn.c` gives core sqrt, exp, ln, log10, pow and the trigonometric functions without libm (the expression engine's ground); expression engine, number entry, working lines, history and settings v2 are not started |
| B overlap hunt | **done for every existing screen**: 39 scripts, 0 findings; worst cases added |
| C fifteen tools | **F1, F3 and F9 done** (Function table; polynomials; sig figs); F2 is next |
| D demo content | **not started** |
| E settings v2 | **not started** |
| F tutorial and help | **partly**: help text lives in the registry and the shared help screen renders it (F5 everywhere, paging in `core/help.c`); the first-run tour, empty states and `docs/TUTORIAL.md` are not written |
| G WebAssembly simulator | **not started** |
| H website v2 | **not started** |
| I README and metadata | **not started** |
| J spike docs | **done**: `docs/spike.md` says what it is and why it is not released |
| K CI and release | **partly**: the screen checks run in `make test`; version stays 1.0.0 until 1.1.0 is cut |

### Decisions taken without asking

- **Text on paper is ink or blue.** Red is 4.46:1 on paper and teal 3.00:1, so
  neither may carry body text. Alerts are ink; "good news" is blue.
- **Selection inverts the tile instead of tinting it.** A selected tile is ink
  with paper text (12.27:1 whatever the accent); an unselected one is paper
  with ink text and an icon in the tool's colour. The first version filled the
  tile with its accent and put a paper plate under lines that could not reach
  AA; it passed every check and looked like a form, so the plate is gone.
- **F5 is help on every screen.** It is drawn in the softkey strip even where a
  screen leaves the slot empty, because it works there: the menu shows the help
  for the tile under the cursor, a tool screen its own help, and EXE returns to
  the screen that asked.
- **A tile keeps its accent only when selected.** Unselected tiles are paper, so
  the grid reads as a sheet of cards and the accent marks the one you are on.
  The subtitle of a selected tile sits on a paper plate when the palette cannot
  carry ink or paper at 4.5:1 (red 4.46, teal 4.09, violet 3.79).
- **The Tools category holds Settings and About**, not the "Settings tools, Help,
  Tour" of the category map, until those screens exist; F5 and F6 keep their
  shortcuts so Settings is one key away from anywhere.
- **The angle mode is a per-tool toggle for now** (F4 in the Function table, shown
  in the title bar) and moves into Settings v2 with workstream E, where it
  becomes the shared setting the master prompt asks for.
- **The Function table's tile reads "Function"**, because a tile title is 14
  characters at most; the docs and the help text call it the Function table.
- **F6 is Settings on the home screen.** About is a tile in the Tools category
  rather than another shortcut, so the strip shows four categories, help and
  settings and nothing twice.
- **The title floor is 3.0:1, body is 4.5:1.** Tile titles and the title bar are
  treated as WCAG large text; they are the biggest, boldest labels on the
  screen and the only way to keep ink on red.
- **`STORE_NAME_MAX` 48 -> 72.** A 57-character name was silently left out of
  every list. No storage layout change.
- **The Study dashboard shows deck and note counts, not cards due and streak.**
  Those need the Study log (F13) and Cards v2 (F15); the strip grows with them.
- **The Tools category holds Settings and About** until F12 and the help screen
  exist.

### Found by the checks, fixed

| Screen | Finding | Fix |
|---|---|---|
| Home | icon in the tile's own colour | measured foreground + outline |
| Home | ink on red (2.75:1), small subtitle | paper plate, then selection inverted (see above) |
| Home | the selected tile's shadow was drawn inside the tile | an L outside the shape |
| Home | triangle and framed icons grew outside their 20x20 box | outline grows by `TH_ICON_OL` on every side |
| Help | an empty softkey strip with six dividers | `w_strip`, one hint line, no dividers |
| About | the Exam Mode sentence in red on paper (4.46:1) | ink with a red mark before it |
| Pacer run | paper on teal (3.00:1) gap line | plate inside the coloured band |
| Pacer, Cards, Number theory | red cursor on the yellow field (2.91:1) | ink cursor |
| Settings | teal "Saved to ..." (3.00:1) | ink |
| Markdown | code span painted over the space before it | background starts after the previous span |
| Markdown | code in teal on shade (2.28:1) | ink on shade |
| Notes search | keyboard half-erased the note behind it | modal clears the content area first |
| Notes search | the space key was an empty cell | marked with a bar |
| Notes | a 57-character file name crossed the title bar | title cut to the room it has |
| everywhere | 21 labels drawn in red or teal on paper | ink or blue |

### Unverified on device

Everything in this session. The hardware is the user's: the new home screen,
the inverted selection, the shadows, the help screen, the keyboard change and
the title-bar cut have been looked at in the host preview only.
