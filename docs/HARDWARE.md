# Hardware report

What has actually been run on a Casio fx-CG 50, and what has not. Every claim in
the README, the CHANGELOG and on the site is supposed to trace back to a row in
this file.

Rules this file follows:

- **A row exists only if someone ran it on the calculator.** No row means
  unverified, and the CHANGELOG says so.
- "Unverified on device" in a CHANGELOG entry means exactly that, not "probably
  fine".
- I do not have the calculator. The person who asked for Bench runs every test
  below, on Windows, copying the `.g3a` by hand in USB Flash mode.

## 1.0.0, confirmed

Reported by the person who owns the hardware, after the 1.0.0 build.

| Area | Result |
|---|---|
| Install | `Bench-1.0.0.g3a` copied to `fls0/`, appears in MENU, launches |
| Storage | `Bench/` and its subfolders created on first run |
| Pacer | Clock runs, F1 logs a mark, F2 pauses, F3 finishes |
| Uncertainty | Absolute and percentage arithmetic, chain mode |
| Cards | Deck list, session, boxes saved at the end |
| Notes | A `.md` note renders; scrolling and paging work |
| Expression engine | The six function slots, tables |
| Number theory | gcd, prime test, factorise, base conversion |

Two bugs came out of that session:

1. a `mode_t` typedef that collided with `sys/types.h` and stopped the add-in
   building for SuperH;
2. a save path that wrote during a session instead of at the end.

Both are fixed and covered by native tests (see `CHANGELOG.md`).

## 1.1.0, confirmed

Reported by the person who owns the hardware, after the 1.1.0 build: **every
tool in 1.1.0 worked**, with one defect, the bottom bar described below. This is
a summary from the person who ran it, not a per-row log; nothing in it was
itemised tool by tool, so treat "worked" as "ran without a crash or a wrong
answer on the screens they opened", which is what they were asked to check.

The build tested was `Bench-1.1.0.g3a` (250 500 bytes, sha256 `5288af0e...`),
the one on the `v1.1.0` release, in `dist/` and on the `builds` branch.

| Area | What 1.1.0 contained |
|---|---|
| Install | Copied to `fls0/`, appears in MENU, launches |
| Storage | `Bench/` and its subfolders, cards and notes read from them |
| Study | Pacer, Cards, Notes, Study log |
| Math | Complex, Function, Sequences, Polynomials, Root finder, Triangle, Base and bits, Number theory, Statistics |
| Science | Lab graph, Uncertainty, Sig figs, SUVAT, Moles |
| Tools | Settings, About, the hardware report screen |

The one defect found, and its fix:

| Defect | State |
|---|---|
| The bottom bar: labels pressed into their slots, and the first-run hint sitting on the tile borders below the grid | **fixed in 1.2.3**, which is a re-layout of a bar that 1.1.0 does not have a spare pixel in. The fix itself has not been on a calculator; see below |

## After 1.1.0: unverified on device

Everything that exists only in 1.2.x is marked **unverified on device** in the
CHANGELOG, and it is a longer list than "the tools": the bottom bar is 20 pixels
now instead of 16, every screen's footer moved with it, and the first-run hint
has a band of its own.

Not yet run on hardware:

| Area | What needs running |
|---|---|
| Prime factors | A number typed in, the power display, the divisor count, sum, totient and square-free part |
| Percentages | The four questions, and the three cases the page refuses with a reason |
| Formula book | Files listed from `Bench/formulas`, scrolling, LEFT/RIGHT sections, F1 to the top |
| Pi game | Digits accepted, a wrong one costing a life, F2 costing a life, the round-over screen |
| The bottom bar | That six labels and a selected tab sit right at 396x224, not just in the preview |
| First-run hint | That it clears the tiles, and that F5 still makes it go away for good |
| Settings v2 | The rows added after 1.1.0: note width, opens on, show hint line, pacer clock red at |
| Demo content | `demo/` copied to the three folders and read by every tool |


## How to run the next pass

1. Copy `Bench-<version>.g3a` to `fls0/` in USB Flash mode.
2. Run it once, then copy `demo/notes`, `demo/cards` and `demo/study` into
   `Bench/notes`, `Bench/cards` and `Bench/study`.
3. Walk the table above, top to bottom.
4. For anything that misbehaves, note the tool, the keys pressed and what the
   screen said. A screenshot from a phone is enough.

The add-in writes nothing during a session except the settings file, and that
only when a value changes, so a crash mid-session also tells us something: cards
should be unchanged and the study log should be unchanged.

## Report template

```
Build:        Bench-____.g3a, version ____, date ____
Copied to:    fls0/  (file size ____ bytes)
Launched:     yes / no
Exam Mode:    off

Tool             Result    Notes
---------------- --------  ---------------------------------------------
Prime factors    pass/fail
Percentages      pass/fail
Formula book     pass/fail
Pi game          pass/fail
Bottom bar       pass/fail   six labels, selected tab, no crowding
First-run hint   pass/fail   clear of the tiles, gone after F5
Settings v2      pass/fail   note width, opens on, hint line, clock red at
Demo content     pass/fail

Anything else:
```

## The report screen

Settings has an **F3: hardware report**. It lists the eight assumptions in
`docs/STATUS.md`, one per row, and each row starts as `not tried`. After
running the matching test above, mark it `works` or `fails` with LEFT/RIGHT.
F2 writes `state/hwreport.txt`; nothing is written at any other time.

The file is the report: copy it off the calculator (USB Flash mode, the same
way the `.g3a` got there), and paste it under this heading with the build it
came from. One row per line, so a diff shows what changed between builds.

```
Bench hardware report
version <the version on the About screen>

<the eight lines, each followed by "works", "fails" or "not tried">

All eight marked.        # or: Some rows are still not tried.
```

The screen keeps its marks only while it is open. That is deliberate: the
report is the artefact, and a half-marked list left in flash would look like a
result it is not.
