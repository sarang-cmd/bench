# Changelog

Everything notable in the Bench add-in, newest first. Storage layout changes are
called out explicitly, as AGENTS.md requires.

## Unreleased - Stage 3 core (31 October 2026)

No .g3a was produced: the container cannot install the fxSDK or a SuperH
compiler (docs/toolchain.md section 5). Native tests only.

### Added
- `src/core/pacer`: minutes per mark, checkpoint times and count, target marks,
  gap to pace, pausable run clock. 114 native checks.
- `src/core/numfmt`: exact double-to-decimal conversion with a small
  big-integer, half-up rounding on decimal digits, plain and scientific
  rendering, no printf and no libm. 182 checks.
- `src/core/uncert`: add, subtract, multiply, divide, power, scale with
  absolute uncertainties; significant-figure and decimal-place display rules;
  percentage uncertainty. 144 checks.
- `src/core/cards_parse`: `question | answer` parsing with line numbers for bad
  lines, 500-card and 120-character limits, FNV-1a question hash. 95 checks.
- `src/core/leitner`: three boxes, right/wrong moves, session draw rules,
  progress file read/write. 126 checks.
- `src/core/textsearch`: case-insensitive search, next/previous, wrap, and the
  line helpers the viewer scrolls with. 84 checks.
- `src/core/md_parse`: block classifier and single-pass inline span parser for
  the Markdown subset. 203 checks.
- `src/core/md_layout`: greedy wrapping against font callbacks, display lines
  with span records, unsupported-glyph count, chunked layout. 95 checks.
- `src/core/numtheory`: gcd, lcm, extended gcd, modular inverse,
  addmod/mulmod, modpow, Miller-Rabin, trial division plus Pollard rho,
  totient, CRT, 18-digit input parsing, cancellation hook. 1027 checks.
- `tools/md2bench.py` and its 28 unit tests; `docs/samples/markdown.md`.
- `make test`, `make py-test`, `make test-all`, `make gint-check`.

### M0/M1 source (not built here)
- `CMakeLists.txt`, `src/main.c` (blank add-in: paper screen, red square, key),
  `spike/CMakeLists.txt`, `spike/main.c` (ten spike tests).
- Verified by type-checking against real gint 2.10.0 headers.

### Docs
- `docs/toolchain.md`, `docs/spike.md` (empty result column), `docs/STATUS.md`.
- `AGENTS.md`, `docs/plan.md` and `docs/tasks/` now exist where the pack's
  README expects them (they shipped under `agent/`).

### Storage layout (new, not yet on any calculator)
```
Bench/notes/*.md|*.txt        notes (M6/M7)
Bench/cards/*.txt            decks, "question | answer" a line (M5)
Bench/state/cards.sav        "# Bench cards state v1", "session N", "<id> <box>"
Bench/state/settings.cfg     settings, written on change only (M2)
```
The cards state format is fixed by `src/core/leitner.c` and covered by tests.

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
