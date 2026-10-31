# Changelog

Everything notable in Bench, newest first. Released versions have a tag of the
same name (`v1.0.0`) and a GitHub release; the release carries the `.g3a` built
by `.github/workflows/build.yml`.

The core modules below were written in milestone order (0.1.0 to 0.6.0) but
never tagged separately: they landed in one 1.0.0 release, so their sections are
milestones, not releases. Only `v0.0.1`, `v0.0.2` and `v1.0.0` are tags. Storage layout changes are called out explicitly, as `AGENTS.md` asks.

The add-in version lives in `version.txt`; `Bench-<version>.g3a` is named from
it, so the file, the About screen and the tag always agree.

## 1.0.0 - 2026-10-31

First release. One add-in, five tools, settings and an About screen.

### Added
- **Shell**: home screen with six tiles in the Bauhaus palette, title bar,
  softkey bar, lists, fields, modals, a progress bar and an on-screen keyboard.
  `src/platform/{screen,input,store,mem}` are the only files that call gint;
  `src/apps/app_menu.c` routes, `src/main.c` runs the loop.
- **Pacer** (`src/apps/app_pacer.c`): setup, plan with the checkpoint table,
  run with clock, marks, target, gap line and next checkpoint, done screen with
  the final gap.
- **Uncertainty** (`src/apps/app_uncert.c`): six operations, chain mode, value
  and absolute and percentage uncertainty on the result screen.
- **Cards** (`src/apps/app_cards.c`): deck list, deck report with problem lines,
  session with reveal, summary, box statistics; progress file written once at
  the end of a session.
- **Notes** (`src/apps/app_notes.c`): file list, Markdown and plain text
  rendering, scroll by line and page, search with wrap, heading list, per-line
  span drawing with the Phase A style (colour and underline).
- **Number theory** (`src/apps/app_numtheory.c`): nine functions, forms,
  results with steps for gcd and ext gcd, a progress bar for factorisation that
  EXIT cancels.
- **Settings** (`src/apps/app_settings.c`, `src/core/settings.c`): uncertainty
  figures, pacer checkpoint size, note width; the file is written only when a
  value changes.
- **About** (`src/apps/app_about.c`): version, build date, target, framebuffer
  and window sizes, largest free block, limits, and the Exam Mode note.
- **Brand**: `assets-cg/icon-uns.png` and `icon-sel.png` (92x64, neobrutalist
  B with a red square and three bars), a wordmark SVG and a 1200x630 card, all
  drawn by `tools/brand/make_brand.py`. No bitmap fonts, no external assets.
- **Host preview**: `tools/sim/` (a gint stand-in, key scripts and a tour) and
  `make sim`, `make shots`, which render every screen to `docs/images`.
- **CI**: `.github/workflows/build.yml` runs the native tests, the Python
  tests, the preview, and builds `Bench-<version>.g3a` with the real fxSDK,
  attaching it to the release on a `v*` tag.
- **Docs**: `README.md`, `docs/STATUS.md`, `docs/toolchain.md`, `docs/spike.md`,
  `site/` (the landing page: React 18 + Vite + Tailwind, deployed on Vercel).

### Storage layout (new)
```
Bench/notes/*.md|*.txt     notes (Notes)
Bench/cards/*.txt          decks, "question | answer" a line (Cards)
Bench/state/cards.sav      "# Bench cards state v1", "session N", "<hash> <box>"
Bench/state/settings.cfg   "key value" lines, written on change only
```
The folder tree is created on first run. Files are written once and renamed
into place; the calculator's filesystem needs even-sized writes, so an
odd-length text file is padded with a newline.

### Notes
- Screen geometry: 396x224 framebuffer, the OS shows the middle 384x216, so
  every drawing call goes through `src/platform/screen.c` and its `SCR_X`,
  `SCR_Y` offset. No app knows the offset exists.
- Colours are raw RGB565 words rather than `C_RGB()`: gint's macro shifts green
  by six while its own constants put green at bits 5..10, so the macro would
  move green one bit. The three palette colours are written out in `theme.h`
  with the hex they came from.
- Text entry never relies on ALPHA: the search field uses the on-screen
  keyboard, so a wrong ALPHA mapping cannot break input.

### Test log
```
make test     NATIVE TESTS PASSED (11 test programs)
              cards 95, leitner 126, md_layout 95, md_parse 203, numfmt 182,
              numparse 62, numtheory 1027, pacer 114, settings 659,
              textsearch 84, uncert 144  ->  2791 checks, 0 failed
make py-test  28 tests, OK
make sim      ok
make shots    26 screens rendered
```
Calculator side: **not run by the agent** (no fxSDK and no hardware here, see
`docs/STATUS.md`), so this version is 1.0.0 of the code, not a hardware-tested
release.

## Milestone 0.6.0 - 2026-10-31 (in 1.0.0)

Number theory.

- `src/core/numtheory.c`: gcd, lcm, extended gcd, modular inverse,
  addmod/mulmod, modpow, deterministic Miller-Rabin, trial division plus Pollard
  rho, totient, CRT, 18-digit input parsing, a cancellation hook.
- 1027 native checks, including the plan's acceptance rows
  (`factor(600851475143) = 71 x 839 x 1471 x 6857`, `modpow(2, 64, 1000) = 616`,
  `inverse(6, 9)` an error, `lcm(UINT64_MAX, 2)` an overflow).
- Factorisation time on a PC: 33 ms for a 1e9 x 1e9 semiprime, 1 ms for a prime
  near 1e18.

## Milestone 0.5.0 - 2026-10-31 (in 1.0.0)

Markdown.

- `src/core/md_parse.c`: block classifier and a single-pass inline span parser,
  no recursion. `src/core/md_layout.c`: greedy wrapping against font callbacks,
  display lines with span records, unsupported-glyph count, chunked layout.
- `tools/md2bench.py`: smart quotes and dashes to ASCII, a symbol table,
  front-matter and HTML stripping, a change report, `--check` for CI.
- `docs/samples/markdown.md` with every syntax row.

## Milestone 0.4.0 - 2026-10-31 (in 1.0.0)

Notes, plain text.

- `src/core/textsearch.c`: case-insensitive search, next/previous, wrap, and the
  line helpers the viewer scrolls with.
- `docs/samples/note.txt`.

## Milestone 0.3.0 - 2026-10-31 (in 1.0.0)

Cards.

- `src/core/cards_parse.c`: `question | answer` parsing in place, line numbers
  on errors, 500-card and 120-character limits, FNV-1a question hash.
- `src/core/leitner.c`: three boxes, due rules, session draw, progress file.
- `docs/samples/cards.txt`.

## Milestone 0.2.0 - 2026-10-31 (in 1.0.0)

Uncertainty and the decimal formatter.

- `src/core/numfmt.c`: exact double-to-decimal conversion with a small
  big-integer, half-up rounding on decimal digits, plain and scientific output,
  no printf and no libm.
- `src/core/uncert.c`: the six operations, display rounding, percentage.
- `src/core/numparse.c`: a parser for numbers typed on the keypad, so the app
  never needs `strtod` or float printf.

## Milestone 0.1.0 - 2026-10-31 (in 1.0.0)

Pacer.

- `src/core/pacer.c`: minutes per mark, checkpoints, target marks, gap to pace,
  pausable run clock. 114 native checks.

## Milestone 0.0.3 - 2026-10-31 (in 1.0.0)

Shell.

- Home screen, theme, widgets, `store.h`, settings, About.
- `src/core/settings.c` with 659 native checks.

## 0.0.2 - 2026-10-31

Spike. Tag `v0.0.2`.

- `spike/main.c`: a throwaway add-in with ten tests, one per question in
  `docs/spike.md` (BFile create/write/read/list/delete, float printf, font
  coverage, ALPHA entry, 1 Hz RTC, heap arenas, screen geometry, redraw time,
  `.g3a` size).
- `docs/spike.md`: the answer sheet, with the result column left for the user.

## 0.0.1 - 2026-10-31

Toolchain. Tag `v0.0.1`.

- `CMakeLists.txt` for the fxSDK, producing `Bench-<version>.g3a`.
- `src/main.c`: a blank add-in. Paper screen, red square, any key returns.
- `docs/toolchain.md`: install, build, test, and the record of what blocked the
  first build.

## Pack v1 (October 31, 2026)
- Bundle the site, the plan, and the agent files in one zip.
- Add task files for M0 to M9.

## Site v4
- Bench guide with the full implementation plan, Markdown download, 18 FAQ entries, and Future spec.

## Site v3
- Fix favicon with base64 SVG and PNG data URIs. Version every artifact.

## Site v2
- Add first three builds, three FAQ entries, Future spec, favicon.

## Site v1
- First FAQ page.

## Plan v1
- First implementation plan for Bench.
