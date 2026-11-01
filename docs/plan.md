# Bench: implementation plan

Version: plan v1. Date: October 31, 2026.
Target: Casio fx-CG50, Casio OS 3.81, add-in file Bench.g3a.
Toolchain: fxSDK with gint, C11, CMake.

## 1. What you build

One add-in with five tools behind one menu.

| # | Tool | Job | Priority |
|---|------|-----|----------|
| 1 | Pacer | Minutes per mark and live pace for a practice paper | Build first |
| 2 | Uncertainty | Combine measured values with uncertainties | Build second |
| 3 | Cards | Leitner flashcard drill from a text file | Build third |
| 4 | Notes | Viewer for text and Markdown files, with search | Build fourth |
| 5 | Number theory | GCD, modular inverse, primality, factors, CRT | Build fifth |

Rules for the whole project:

- Exam Mode blocks add-ins. Never read, change, or work around Exam Mode.
- Never write code for the OS, bootloader, or flash. Use file calls only.
- Write core logic in plain C with no gint calls. Test the core on your PC.
- Ship small versions. Tag each build.

## 2. Facts to respect

| Fact | Source or status |
|------|------------------|
| fxSDK builds with CMake. `fxsdk new` makes a project. `fxsdk build-cg` makes the .g3a. | fxSDK README |
| fxSDK supports Linux officially. Windows works through WSL Ubuntu. | fxSDK README |
| `fxlink` USB transfer does not work inside WSL. Copy the .g3a by hand. | fxSDK README |
| `fxsdk send-cg` copies the file with fxlink using UDisks2. | fxSDK README |
| gint adds roughly 50 to 100 KiB to every .g3a. | Cemetech announcement |
| C support covers C11. The fxlibc library covers most of C99. C++20 also works. | fxSDK README |
| gint offers extra heap arenas and on-chip memory sections. | Cemetech announcement |
| The visible screen is 384 by 216 pixels. The gint frame buffer is larger. | Confirm in the gint headers |
| Earlier notes in this project wrote `fxsdk build-cg -s`. Treat the `-s` flag as unverified. Use `fxsdk build-cg` then `fxsdk send-cg`. | Correction |

Your Windows setup shapes the workflow. Build in the cloud or in WSL. Put the calculator in USB Flash mode. Copy the .g3a to the calculator drive with Explorer.

## 3. Repository layout

```
bench/
  AGENTS.md
  CMakeLists.txt
  CHANGELOG.md
  docs/                 WikiPrizm pages, gint headers, this plan
  assets-cg/            icon-uns.png, icon-sel.png, fonts, fxconv-metadata.txt
  src/
    core/               plain C, no gint includes, tested on PC
      pacer.c  pacer.h
      uncert.c uncert.h
      numfmt.c numfmt.h
      leitner.c leitner.h
      cards_parse.c cards_parse.h
      numtheory.c numtheory.h
      md_parse.c md_parse.h
      md_layout.c md_layout.h
      textsearch.c textsearch.h
    platform/           gint calls only
      screen.c  screen.h      drawing helpers
      input.c   input.h       keys, text entry, timers
      store.c   store.h       file read, write, list
      mem.c     mem.h         arenas and limits
    ui/
      widgets.c widgets.h     list, field, modal, softkey bar
      theme.h                 colors and spacing
    apps/
      app_menu.c app_pacer.c app_uncert.c app_cards.c app_notes.c app_numtheory.c
    main.c
  tests/                native gcc tests for src/core
  tools/
    md2bench.py         clean Markdown before copy to the calculator
    make_vectors.py     generate test vectors
```

Why this split: every bug in math or parsing shows up on your PC in seconds. Only drawing and input need the calculator.

## 4. Storage on the calculator

Folder: `Bench` in the calculator storage root.

```
Bench/notes/*.md        your notes
Bench/notes/*.txt
Bench/cards/*.txt       one card per line: question | answer
Bench/state/cards.sav   box numbers per card
Bench/state/settings.cfg
```

Write rules, to protect flash:

- Save card progress at the end of a session only.
- Save settings on change only.
- Never write inside a loop or on a timer.
- Write to a temp file, then replace the old file.

Open question: the exact file calls. Spike M1 settles this. Expect gint BFile functions with paths in UTF-16. Confirm in the headers before writing `store.c`.

## 5. Screen and input model

- Layout: title bar at the top, content in the middle, softkey bar at the bottom.
- Softkeys: F1 to F6 match labels in the bottom bar, as on the stock OS.
- Keys: arrows move, EXE confirms, EXIT goes back, MENU leaves the add-in.
- Redraw: draw a full frame, then update the screen. Redraw on key press only. Pacer redraws once per second.
- Palette (Bauhaus): paper `#f4f3ee`, ink `#2f2d2e`, red `#d92d27`. Convert to the 16-bit color format in `theme.h`.
- Home screen: five tiles with a square, circle, triangle, half circle, and bar. Plain shapes, no animation.

Text entry is the hard part. The calculator has an ALPHA shift. Spike M1 tests letter entry. Fallback: choose files from a list and avoid typing names. Search uses a short field with ALPHA input.

## 6. Tool specs

### 6.1 Pacer

Inputs: total minutes `T`, total marks `M`.

Math:

- `mpm = T / M` minutes per mark.
- Checkpoints every `C` marks, default 10. Checkpoint `k` sits at `k * C * mpm` minutes.
- While running: you press F1 to log marks done `d` at the current time `t`.
- Target marks at time `t` is `t / mpm`.
- Gap in minutes is `(t / mpm - d) * mpm = t - d * mpm`. Positive means behind.

Screens:

1. Setup: two number fields, checkpoint size.
2. Plan: mpm, checkpoint table.
3. Run: big clock, marks logged, gap line, next checkpoint time.
4. Done: total time, final gap.

Acceptance tests:

| T | M | mpm | Checkpoint at 10 marks |
|---|---|-----|------------------------|
| 90 | 80 | 1.125 | 11.25 min |
| 120 | 110 | 1.0909 | 10.91 min |
| 60 | 0 | error line, no next screen | none |

Edge cases: zero or empty input, marks above 999, pause and resume, clock reaches `T`.

### 6.2 Uncertainty

Represent each quantity as value and absolute uncertainty. Use `double` for math and a custom formatter for output. Do not depend on float printf until the spike proves float printing works.

Operations:

| Op | Value | Uncertainty |
|----|-------|-------------|
| A + B | a + b | da + db |
| A - B | a - b | da + db |
| A * B | a * b | abs(result) * (da/abs(a) + db/abs(b)) |
| A / B | a / b | abs(result) * (da/abs(a) + db/abs(b)) |
| A ^ n | a^n | abs(result) * abs(n) * da/abs(a) |
| k * A | k * a | abs(k) * da |

Chain mode: the result becomes A for the next operation.

Rounding (setting, default 1 significant figure for uncertainty):

- Round the uncertainty to 1 significant figure. Use 2 if the first digit is 1. Make this a setting.
- Round the value to the same decimal place as the uncertainty.
- Use half-up rounding on the decimal digits, not on the binary float.

Show on screen: result, absolute uncertainty, percentage uncertainty.

Acceptance tests:

| Input | Expected |
|-------|----------|
| (10.0 ± 0.1) + (5.0 ± 0.2) | 15.0 ± 0.3 |
| (2.0 ± 0.1) * (3.0 ± 0.1) | 6.0 ± 0.5 |
| (4.0 ± 0.2) ^ 2 | 16 ± 2 |
| (10.0 ± 0.1) / (4.0 ± 0.2) | 2.5 ± 0.2 (raw 0.15, half-up) |
| A with value 0 in a product | error line, no crash |

Edge cases: zero value, negative values, uncertainty larger than value, tiny and huge exponents.

### 6.3 Cards

File format, one card per line:

```
What is the unit of impulse? | N s
```

Rules: split at the first pipe. Trim spaces. Skip blank lines and lines starting with `#`. Limit 500 cards per deck and 120 characters per side. Show a warning for longer lines.

Leitner boxes: three.

- New cards start in box 1.
- Right answer: box + 1, capped at 3.
- Wrong answer: box 1.
- Session draws from box 1 first, then box 2 every second session, then box 3 every fourth session.

Screens: deck list, session (question, EXE to reveal, F1 right, F2 wrong), summary, stats per box.

Progress file maps a card id to a box. Card id is a hash of the question text, so edits to the answer keep progress. Edits to the question reset the card.

Acceptance tests:

- Quit and relaunch keeps every box.
- Editing an answer keeps the box. Editing a question resets the box.
- A file with 500 cards loads and runs.
- A file with a bad line shows the line number and skips the line.

### 6.4 Notes with Markdown

Viewer features: file list, scroll, page jump, search, jump to heading list, back to list.

Supported Markdown subset:

| Syntax | Render |
|--------|--------|
| `#`, `##`, `###` | Heading levels, color and size |
| `**bold**` | Bold style |
| `*italic*` or `_italic_` | Italic style |
| `code` in backticks | Monospace span |
| Fenced code block | Monospace block, no wrap |
| `-` or `*` bullets | Bullet with indent |
| `1.` lists | Numbered with indent |
| `>` quote | Left bar and indent |
| `---` | Horizontal rule |
| `[text](url)` | Show text only |
| `- [ ]` and `- [x]` | Box and check marks |

Not supported in v1: tables, images, HTML, footnotes, nested lists beyond 3 levels. Show these as plain text.

Math: show `$...$` as plain text. A converter in `tools/md2bench.py` replaces common symbols with ASCII or glyphs the font covers.

Parser design:

1. Read the file into one buffer. Cap 32 KiB at first. Raise the cap after the memory spike.
2. Split into lines. Classify each line into a block type.
3. Parse inline spans inside each block with a small state machine. No recursion.
4. Layout: measure word widths with the font, wrap to the screen width, and produce an array of display lines. Each display line holds span records: offset, length, style.
5. Render only the visible display lines.
6. Scroll position is a display line index. Rewrap on font change.

Style plan, two phases:

- Phase A: no extra fonts. Use color and underline. Headings use red or ink with a rule. Bold uses the ink color on a light highlight bar. Italic uses an underline.
- Phase B: generate bold, italic, and monospace fonts with fxconv. Swap styles through `theme.h`.

Search: case-insensitive substring. F-key jumps to the next match and highlights the line.

Encoding: the loader reads UTF-8. Characters outside the font map to `?` and the viewer lists a count in the footer. `md2bench.py` rewrites known symbols first.

`tools/md2bench.py` tasks:

- Replace smart quotes, dashes, and ellipses with ASCII.
- Replace unsupported symbols from a mapping table.
- Strip front matter, HTML tags, and image lines.
- Report every change and every unsupported block.
- Write the output into a folder ready to copy.

Acceptance tests:

- A note with every syntax row renders each row correctly.
- A 30 KiB note scrolls without visible lag.
- Search finds a word on the last line.
- A file with a missing glyph shows `?` and the footer count.
- An empty file shows an empty state, not a crash.

### 6.5 Number theory

Limits: unsigned 64-bit math, inputs up to 18 digits. Reject larger input with an error line. The SuperH compiler has no 128-bit integer, so write `mulmod` with 32-bit limbs or double-and-add.

Functions:

| Function | Notes |
|----------|-------|
| gcd, lcm | Euclid |
| ext gcd | Bezout coefficients |
| modular inverse | Fails if gcd is not 1 |
| modpow | Square and multiply with `mulmod` |
| primality | Deterministic Miller-Rabin for 64-bit |
| factorization | Trial division by small primes, then Pollard rho. EXIT cancels. |
| totient | From the factorization |
| CRT | Two congruences first, then a list |

Screens: function list, input form, result view with steps for gcd and ext gcd.

Acceptance tests:

| Call | Expected |
|------|----------|
| gcd(84, 36) | 12 |
| inverse(3, 11) | 4 |
| modpow(2, 10, 1000) | 24 |
| isprime(1000000007) | yes |
| factor(600851475143) | 71 x 839 x 1471 x 6857 |
| totient(36) | 12 |
| CRT x=2 mod 3, x=3 mod 5 | x = 8 mod 15 |
| inverse(6, 9) | error, no inverse |

Edge cases: 0, 1, equal inputs, a prime near 10^18, a product of two primes near 10^9. Show a progress indicator for long factorizations.

## 7. Milestones

Each milestone ends with a tagged build and a short test log in `CHANGELOG.md`.

| M | Name | Runs on | Size | Done when |
|---|------|---------------|------|-----------|
| M0 | Toolchain and hello | Cloud or WSL, then calculator | S | Blank add-in builds and runs on the calculator |
| M1 | Spike | Calculator | M | Written answers to the spike questions |
| M2 | Shell | Cloud, then calculator | M | Menu, theme, widgets, store layer, settings |
| M3 | Pacer | Cloud, then calculator | M | Tests pass on PC and on the calculator |
| M4 | Uncertainty | Cloud, then calculator | M | All test rows pass |
| M5 | Cards | Cloud, then calculator | L | Progress survives relaunch |
| M6 | Notes, plain text | Cloud, then calculator | M | List, scroll, search work |
| M7 | Markdown | Cloud, then calculator | L | Every syntax row renders |
| M8 | Number theory | Cloud, then calculator | L | All test rows pass |
| M9 | Polish and release | Calculator | M | Icon, version screen, Exam Mode test, release post |

### M1 spike questions

Build a throwaway add-in. Record an answer for each line in `docs/spike.md`.

1. Which file calls list, read, write, and delete files?
2. How do paths look? What is the folder separator?
3. Does `printf` with floats work? If not, which formatter do you use?
4. Does the default font cover the symbols for degrees, plus-minus, times, and micro? Which Greek letters work?
5. How do you read ALPHA letters from the keyboard?
6. Which timer or RTC call gives one tick per second?
7. How much heap does `malloc` give? What does the extra arena add?
8. What is the exact drawable width and height?
9. How long does a full-screen redraw take?
10. What size is the empty add-in?

### Build and test loop

1. Agent edits `src/core` and `tests`.
2. Agent runs `make test` on the PC. Native gcc, no gint.
3. Agent runs `fxsdk build-cg` in the container.
4. CI stores `Bench-<version>.g3a` as an artifact.
5. You download the artifact and copy the file to the calculator.
6. You run the milestone checklist and log results.
7. You report failures back to the agent with a photo or a text description.

## 8. Agent workflow

Cloud first. The agent writes code, runs native tests, and builds the .g3a. You own the hardware tests.

Task template for the agent:

```
Task: <one sentence>
Milestone: <M number>
Files you may edit: <list>
Files you must not edit: <list>
Spec: <section of this plan>
Tests: add or update tests in tests/ before the code
Done means: tests pass, fxsdk build-cg passes, you list screens and keys used
```

Extended AGENTS.md rules, on top of the first draft:

- Keep `src/core` free of gint includes.
- Add a test for every function in `src/core`.
- Limit each task to one tool or one module.
- Do not add dependencies.
- Do not change the storage layout without a note in `CHANGELOG.md`.
- Print no debug text in release builds.
- Never write files from a loop.

Token budget tips:

- Give the agent only the relevant plan section and the needed headers.
- Use native tests to catch errors before any long build.
- Reuse one container image.
- Start a fresh session per milestone and paste the milestone table row.

## 9. Risks

| Risk | Effect | Response |
|------|--------|----------|
| File API differs from the plan | M2 store layer rewrites | M1 spike, then one `store.h` interface |
| No float printf | Wrong output in Uncertainty | Custom decimal formatter in `numfmt.c`, tested on PC |
| Font lacks symbols | Missing glyphs in notes | `md2bench.py` mapping, custom font in Phase B |
| Heap too small for 32 KiB note | Crash on open | Check size before load, stream by line, use extra arena |
| Text entry is clumsy | Search is slow to use | Short fields, file picker, no filename typing |
| fxlink fails in WSL | No one-step send | Copy by hand in USB Flash mode |
| Add-in breaks on a new OS | Crash after update | Test on 3.81, log OS version in the version screen |
| Too many writes | Flash wear | Save on session end only |
| Scope grows | Nothing ships | Ship M3, then M4, then each tool as its own tag |
| Exam Mode confusion | Rule breach | No exam code. Test at home and confirm the block |

## 10. Release plan

Tags:

- `0.1.0`: shell and Pacer.
- `0.2.0`: Uncertainty.
- `0.3.0`: Cards.
- `0.4.0`: Notes, plain text.
- `0.5.0`: Markdown.
- `0.6.0`: Number theory.
- `1.0.0`: polish, icon, docs, Cemetech post.

File names: `Bench-0.1.0.g3a`. Keep every released file.

Release checklist:

1. Run the milestone checklist on OS 3.81.
2. Run Examination Mode once and confirm the add-in is blocked.
3. Back up calculator memory before the first install of each tag.
4. Write a readme with keys, screens, file formats, and known problems.
5. Record the OS version and build date in the readme.
6. Post to Cemetech with source, readme, and a note on AI use.

## 11. First three actions

1. Set up the container or WSL toolchain. Build M0.
2. Run the M1 spike on your calculator. Fill in `docs/spike.md`.
3. Hand the agent the M2 task with this plan and the spike answers.

## 12. What changed while building it

Plan v1 held up: the module split, the core/platform boundary and the acceptance
tables all survived contact with the code. These are the places the
implementation had to decide something the plan left open. Each one is
reflected in the code and in `docs/STATUS.md`.

| # | Plan said | Built as | Why |
|---|-----------|----------|-----|
| 1 | Palette in "the calculator's 16-bit format" via `C_RGB()` | raw RGB565 words in `src/ui/theme.h`, with the hex they came from | gint's `C_RGB()` shifts green by six while its own `C_GREEN` constant is `0x07e0`, so the macro moves green one bit; writing the word keeps `#f4f3ee` as `#f4f3ee` |
| 2 | "The visible screen is 384x216, the framebuffer is larger. Confirm in the headers." | confirmed: 396x224, window centred at (6,4) | every drawing call goes through `src/platform/screen.c`, which adds the offset and clips, so no app knows it exists |
| 3 | Text entry may need the ALPHA shift; fallback is a file list | search uses an on-screen keyboard (arrows, EXE) | the ALPHA mapping is spike question 5 and unconfirmed; the keyboard works whatever it says |
| 4 | Round the uncertainty to 1 significant figure, "use 2 if the first digit is 1" as a setting | 1 figure is the default, the 2-when-1 rule is the *other* setting | the plan's own acceptance row (`0.15 -> 0.2`) only holds with 1 figure |
| 5 | Percentage uncertainty "shown on screen" | computed from the rounded uncertainty that is displayed | keeps the three numbers on the screen consistent with each other |
| 6 | Session draws box 2 every second session, box 3 every fourth | session index is 0-based: box 2 on sessions 1, 3, 5..., box 3 on 3, 7, 11... | the plan gives the pattern, not the phase; 0-based keeps "session 0" meaning "never drilled" |
| 7 | A too-long card line is "a warning" | the side is truncated to 120 characters and reported with its line number | "show a warning", not "skip the card" |
| 8 | 32 KiB note cap, raise after the memory spike | kept at 32 KiB; the About screen reports the largest free block | spike question 7 is still open |
| 9 | Settings saved "on change only" | a value comparison against the file's contents, then one write | a key press that changes nothing writes nothing |
| 10 | Files in `docs/tasks/` | they shipped in `agent/tasks/`, copied to `docs/tasks/` and left in place | do not disturb the original pack |
| 11 | Build with `fxsdk build-cg` in the sandbox | written and type-checked here; built by `.github/workflows/build.yml` | the sandbox cannot reach `git.planet-casio.com` (docs/toolchain.md section 5) |
| 12 | Home screen: five tiles | six tiles: the five tools plus Settings, with About on F6 | Settings needs a door; F6 keeps the house in six tiles and three rows |

Two additions the plan did not name, both small and both tested:

- `src/core/numparse.c`: the calculator may not have a usable `strtod` for the
  values people type into a keypad, so the tools parse typed numbers with this
  module instead (62 checks).
- `src/core/settings.c`: the settings file needed a parser anyway, and a parser
  in the core can be tested (659 checks).

Not done, because it needs the calculator: the ten spike answers, the Exam Mode
check, the redraw-time measurement, and every hardware test in section 7.

### Settings keys (V2)

| key | range | default | reaches |
|---|---|---|---|
| `uncert_sig` | 1-2 | 1 | Uncertainty tool's figures |
| `pacer_checkpoint` | 1-200 | 10 | Pacer checkpoints |
| `notes_wrap` | 0, 50-150 | 0 | Notes line width |
| `angle_unit` | 0 = degrees, 1 = radians | 0 | Triangle opens in |
| `tour_done` | 0-1 | 0 | the first-run line, one way |
| `startup_cat` | 0-3 | 0 | which category the home screen opens on |
| `show_hint` | 0-1 | 1 | whether the first-run line is shown at all |
| `pacer_warn` | 0-30 | 5 | minutes left when the Pacer clock turns red |

Unknown keys are ignored and counted, so a file written by a newer build still
loads in an older one.
