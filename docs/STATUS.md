# Status

Last updated: 31 October 2026, at **STOP 1**.
A fresh session should read this file first, then AGENTS.md, docs/plan.md and
the task file for the milestone it is about to start.

## Where the project is

| Stage | Task | State |
|-------|------|-------|
| 1 | M0 toolchain + blank add-in | source written, **not built** (no cross compiler here) |
| 2 | M1 spike add-in | source written, **not built**; docs/spike.md ready with an empty result column |
| 3 | core logic without hardware | **done**, all nine core modules and their tests |
| 4 | M2 shell | next, blocked on the spike results |
| 5 | M3..M8 wiring | not started |
| 6 | M9 release | not started |

**The two .g3a files do not exist.** This container cannot reach
`gitea.planet-casio.com` (fxSDK, gint, the SuperH cross compiler) and has no
apt; there is no prebuilt `sh-elf` toolchain on the sources that are reachable.
docs/toolchain.md section 5 has the evidence and the two ways forward. No
placeholder .g3a was invented.

Versions built so far: none. Versions tagged: none.

## Where the task files are

They shipped in `agent/tasks/`, not `docs/tasks/`. They are now copied to
`docs/tasks/` (and `agent/tasks/` is left untouched so the pack stays intact).
`AGENTS.md` and `docs/plan.md` were copied from `agent/` to the repository root
and `docs/`, as the pack's README describes.

## Tests

Native core suite, `make test` (gcc, `-Wall -Wextra -Wshadow -Werror
-pedantic`, no gint):

| Test program | Module | Checks |
|--------------|--------|--------|
| test_pacer | pacer | 114 |
| test_numfmt | numfmt | 182 |
| test_uncert | uncert | 144 |
| test_cards | cards_parse | 95 |
| test_leitner | leitner | 126 |
| test_textsearch | textsearch | 84 |
| test_md_parse | md_parse | 203 |
| test_md_layout | md_layout | 95 |
| test_numtheory | numtheory | 1027 |
| **total** | | **2070 checks, 0 failures** |

Python, `make py-test`: 28 unittest cases for `tools/md2bench.py`, all pass.
Sanitizers: the same nine programs are clean under ASan + UBSan.
gint header check, `make gint-check GINT_INC=...`: `src/main.c` and
`spike/main.c` type-check against real gint 2.10.0 headers with
`-Wall -Wextra -Werror`.

Timing noted on the PC (2 GHz x86, guide only): factorising a 1e9 x 1e9
semiprime takes 33 ms here, so the calculator will need seconds and the
progress indicator plus EXIT cancel that M8 asks for.

## Files created (this session)

Core, no gint, all tested:

```
src/core/pacer.c/.h            M3  mpm, checkpoints, gap, pausable clock
src/core/numfmt.c/.h           M4  exact double->decimal, half-up, no printf
src/core/uncert.c/.h           M4  add/sub/mul/div/pow/scale + display
src/core/cards_parse.c/.h      M5  "q | a" parsing, limits, issue list, hash
src/core/leitner.c/.h          M5  boxes, due rules, progress file
src/core/textsearch.c/.h       M6  case-insensitive search + line helpers
src/core/md_parse.c/.h         M7  block classifier, inline spans
src/core/md_layout.c/.h        M7  wrap with font callbacks, span records
src/core/numtheory.c/.h        M8  gcd/lcm/egcd/inverse/modpow/MR/rho/CRT
```

Tests and tools:

```
tests/test_util.h              tiny assert harness
tests/test_*.c                 one program per module (nine)
tools/md2bench.py              Markdown cleaner for the calculator
tools/test_md2bench.py         28 unittest cases
docs/samples/markdown.md       one file with every supported Markdown row
```

Platform and build:

```
CMakeLists.txt                 fxSDK project, output Bench-<version>.g3a
src/main.c                     M0 blank add-in: paper screen, red square, key
spike/CMakeLists.txt           throwaway M1 project
spike/main.c                   ten spike tests
assets-cg/icon-uns.png         placeholder icons, copied from the fxSDK
assets-cg/icon-sel.png           project template (M9 makes real ones)
assets-cg/fxconv-metadata.txt  from the same template
```

Docs:

```
AGENTS.md                      copy of agent/AGENTS.md (repo root, as the pack asks)
docs/plan.md                   copy of agent/docs/plan.md
docs/tasks/TASK-M0..M9.md      copies of agent/tasks/
docs/toolchain.md              install steps, versions, what blocked the build
docs/spike.md                  ten questions, calls used, empty result column
docs/STATUS.md                 this file
```

Build and test plumbing: `Makefile` (`test`, `py-test`, `test-all`,
`gint-check`, `clean`), `.gitignore`.

## Screens and keys used

M0 add-in (`src/main.c`): one screen, no title bar yet (that is M2).
Paper background, one red square centred on the OS window. Keys: any key
exits (`getkey()`); the calculator's own MENU handling is gint's default.

Spike add-in (`spike/main.c`):

| Screen | Contents |
|--------|----------|
| menu | title bar + ten entries, the selected one inverted |
| result screens | up to 14 short lines of returned values |
| test 4, 6, 8, 9 | custom drawings: font samples, live clock, frame, redraw loop |

Keys: UP, DOWN (choose), EXE (run), any key (leave a result), EXIT (leave the
add-in).

## Files the add-ins read or write

M0: none.

Spike: creates the folder `\\fls0\Bench`, creates and writes
`\\fls0\Bench\spike.txt` (16 bytes), reads it back, lists
`\\fls0\Bench\*`, then deletes `spike.txt`. It leaves the empty `Bench` folder
behind and never touches anything else. No settings, no timers that outlive the
add-in (the 1 Hz RTC interrupt is disabled before the screen is left).

## What needs the calculator

1. Build both add-ins and report the two file sizes (`Bench-0.0.1.g3a`,
   `Bench-spike-0.0.2.g3a`).
2. Run M0: the whole 384x216 window should be paper colour with a centred red
   square, and any key should return to the menu.
3. Run the spike and answer the ten questions in docs/spike.md. The answers
   that change the plan most: file API (1, 2), float printf (3), font coverage
   (4), ALPHA entry (5), heap (7), redraw time (9).
4. Leave an empty `Bench` folder on the calculator for M5/M6 to use.

## Open questions for the user

1. **Toolchain.** Which way forward from docs/toolchain.md section 5: build in
   WSL yourself and post the .g3a files back, or give this sandbox access to
   the fxSDK sources so it can build here? Until this is settled there will be
   no .g3a from this side, at any milestone.
2. **Fractional exponents in Uncertainty.** `uncert_pow` handles whole
   exponents (repeated squaring) and n = ±0.5 (own square root) without libm.
   Other fractional exponents (0.25, 1.7, ...) return an error today because
   they need `pow()`, and this build does not yet know whether libm's `pow` is
   linked. Question: keep the restriction, or assume libm and use `pow`? The
   plan's tables only use whole exponents.
3. **Card decks over 500 lines.** The plan says 500 cards a deck, and a longer
   file reports `deck full` per extra line. That could be 100 lines of warnings.
   Question: is a single "deck full" summary line enough?
4. **Note size cap.** 32 KiB from plan 6.4 is kept until the heap answer (spike
   question 7) arrives. Confirm after the spike that raising it is wanted.

## Next task

TASK-M2 (shell), version 0.0.3, after the spike answers arrive: theme, widgets,
platform layer (screen, input, store, mem), home screen with five tiles, and
settings that are written only on change. It must not start before the gate
(M1 results) is confirmed.

## Decisions taken that the plan did not settle

| Decision | Where | Why |
|----------|-------|-----|
| "Round the uncertainty to 1 significant figure" is the default; the "use 2 when the first digit is 1" rule is the *other* setting | uncert.h | plan 6.2 offers both and its acceptance row (0.15 -> 0.2) only works with the 1-figure default |
| Percentage uncertainty is computed from the rounded uncertainty that is shown | uncert.c | keeps the three numbers on screen consistent with each other |
| Decimal rounding is two-stage: exact binary value -> 15 significant digits -> half-up at the target place | numfmt.c | "half-up on decimal digits, not on the binary float"; 0.15 stored as 0.1499... must still round to 0.2 |
| Card too long: kept and truncated to 120 characters with a warning | cards_parse.c | plan says "show a warning", not "skip" |
| Session index is 0-based; box 2 due on sessions 1,3,5..., box 3 on 3,7,11... | leitner.c | plan gives the pattern, not the phase |
| `leitner_state_box` treats an unknown card as box 1 | leitner.c | new cards start in box 1; edits to a question therefore reset the card |
| Marks per checkpoint list stops at floor(M/C) | pacer.c | "checkpoint k sits at k*C*mpm"; a partial last block is not a checkpoint |
| gint cannot call BFile from inside gint; paths are FONTCHARACTER `u"\\\\fls0\\..."`; on fx-CG the filesystem is Fugue so the C99 file API is available behind a world switch | docs/toolchain.md, spike | gint 2.10.0 `bfile.h` header text; this is spike question 1/2 and will be confirmed on the device |
