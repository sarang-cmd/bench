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

## V2: unverified on device

Everything after 1.0.0 is marked **unverified on device** in the CHANGELOG. The
core arithmetic has native tests (32 test programs, `make test`) and every screen
is rendered by the PC simulator and checked for overlaps (`make check-screens`),
but neither is the calculator.

Not yet run on hardware:

| Area | What needs running |
|---|---|
| Triangle | SSS, SAS, ASA, SSA (two solutions), the DEG/RAD switch |
| Polynomials | Roots mode up to degree 8, solving from `state/exprs.txt` |
| Root finder | Bisection, Newton, secant; the iterate table; the plot |
| Sequences | Arithmetic, geometric, binomial; the sum line |
| Statistics | Mean, median, quartiles, standard deviation, single and paired |
| Sig figs | All six modes, in practice mode |
| SUVAT | Every combination, the flight-path view, `g = 9.81` and `10` |
| Moles | The three calculation screens |
| Base and bits | Bases 2, 8, 10, 16; the operator list; shifts and masks |
| Lab graph | Points typed in, the plot, the gradient |
| Complex numbers | The four operations, the polar line, DEG/RAD, division by zero |
| Settings v2 | The five rows, and that `settings.cfg` keeps its shape |
| Hardware report | F3 in Settings, marking a row, F2 writing `state/hwreport.txt` |
| First-run hint | It appears once and goes away after F5 |
| Demo content | `demo/` copied to the three folders and read by every tool |
| Study log | F1 adds a session, F2 removes the last, EXIT writes once |

## How to run the V2 pass

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
Triangle         pass/fail
Polynomials      pass/fail
Root finder      pass/fail
Sequences        pass/fail
Statistics       pass/fail
Sig figs         pass/fail
SUVAT            pass/fail
Moles            pass/fail
Base and bits    pass/fail
Lab graph        pass/fail
Settings         pass/fail
First-run hint   pass/fail
Demo content     pass/fail
Study log        pass/fail

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
