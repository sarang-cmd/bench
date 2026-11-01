# Changelog

Everything notable in Bench, newest first. Released versions have a tag of the
same name (`v1.0.0`) and a GitHub release; the release carries the `.g3a` built
by `.github/workflows/build.yml`.

The core modules below were written in milestone order (0.1.0 to 0.6.0) but
never tagged separately: they landed in one 1.0.0 release, so their sections are
milestones, not releases. Only `v0.0.1`, `v0.0.2` and `v1.0.0` are tags. Storage layout changes are called out explicitly, as `AGENTS.md` asks.

The add-in version lives in `version.txt`; `Bench-<version>.g3a` is named from
it, so the file, the About screen and the tag always agree.

## 1.2.5 - unreleased (source only)

Numbers here describe the source in `main`. The newest build anyone can download
is `Bench-1.2.3.g3a`.

- **"No notes yet" while the notes sat one folder away**: the calculator moves
  anything it will not keep in main memory to `SAVE-F` (fx-CG 50 manual, 13-6:
  contents past three levels of `@MainMem`, and anything it does not recognise).
  A `Bench` folder copied into `@MainMem` therefore lands in `SAVE-F/Bench` when
  the cable is pulled, and Bench - which reads `\fls0\Bench` and nothing else -
  reported an empty folder. The three list screens now check
  `SAVE-F/Bench/<folder>` when their own folder is empty, and say so:
  "Notes are in SAVE-F / Move SAVE-F/Bench/notes to Bench/notes". The default
  empty text also names the place - "Put .md or .txt in Bench/notes on the card
  root" - because "Bench/notes" alone was read as "any Bench folder". New tour
  step 98 covers it, with a sample note under `docs/samples/savef/`.
  **Unverified on device.**
- **The README said where the files go without saying where they must not**:
  `@MainMem` was mentioned once, in passing, as a landmark. It is not a
  landmark, it is the trap, and the drive root / `@MainMem` / `SAVE-F` triangle
  now has a section of its own, plus four numbered install steps that put the
  `.g3a` and the `Bench` folder in the root and say "never inside it" twice.
  `docs/TUTORIAL.md` and `dist/README.md` carry the same warning.
- **Four more README corrections found while checking it against the tree**:
  the file list was missing `study/log.txt`, `formulas/*.txt` and
  `state/hwreport.txt`; the `settings.cfg` example listed five of the eight
  keys; the `builds` branch line still fetched `Bench-1.0.0.g3a`; and the
  "make py-test" line described a single converter when it now runs 68 tests
  over three tools. `docs/TUTORIAL.md`'s Notes key table was wrong in five rows
  - it promised ALPHA for search (F1, and it is the on-screen keyboard), F3/F4
  for previous/next file (they are the heading list and the top), and EXE for
  page down (it goes back to the list).

- **1.1.0 ran on the calculator, and everything in it worked**: the owner of the
  hardware reported after the 1.1.0 build that every tool it shipped with - Pacer,
  Cards, Notes, the study log, Complex, Function, Sequences, Polynomials, Root
  finder, Triangle, Base and bits, Number theory, Statistics, Lab graph,
  Uncertainty, Sig figs, SUVAT, Moles, Settings, About and the hardware report
  screen - ran, and the one defect was the bottom bar: labels pressed into their
  slots and the first-run hint sitting on the tile borders, both fixed in 1.2.3.
  `docs/HARDWARE.md` records it as a summary from the person who ran it rather
  than a per-row log, because that is what it is. The claims that said only 1.0.0
  had ever been on a calculator - README, `docs/STATUS.md`, the site - say 1.0.0
  and 1.1.0 now, and the "needs the calculator" list is shortened to what is
  actually left: the four V2 tools, the new bar and hint, and the four settings
  rows added after 1.1.0. **1.2.3 itself is still unverified on device.**

## 1.2.3 - 2026-11-01

Numbers here describe the source in `main`, not a run on hardware: 1.2.3 is
built by CI from this commit, and nobody has had it on a calculator yet.

- **The calculator build has been failing since the Pi game landed**:
  `app_pi.c` calls `nt_append` without including `ui/numtext.h`. The host
  preview took it as a warning and drew a perfect screenshot; the fxSDK build
  runs `-Werror=implicit-function-declaration` and stopped, so there has been no
  `.g3a` since 1.1.0 and the tagged release for 1.2.2 could not exist. The
  include is one line; the reason it went unseen for four merges is that the two
  builds disagreed about which warnings matter, so the preview now compiles with
  the same `-Werror=implicit-function-declaration` as the calculator and the
  candidate `.g3a` cannot be the only thing that finds it.
- **The committed WebAssembly build is rebuilt from these sources**, so the
  simulator on the site is the same code as the screenshots again. Its smoke
  runs (`down,exe` and the worker path) pass. **Unverified on device.**
- **The screen checker can see a label sitting on a border**: it compared
  rectangles, so two things that merely touched were "not overlapping" and
  passed. The menu's first-run hint spent eleven merged pull requests with its
  ascenders on the bottom tile border, and 97 green screenshots said nothing
  about it. `scr_frame` is traced now - `scr_fill` records only panels of 8x4 or
  more, and a two-pixel border is exactly the size that goes wrong - and a label
  within two pixels of any frame, inside or out, fails the run. Checked against
  the old layout: it reports the hint and nothing else, so the rule has teeth
  without inventing work on the other 96 screens. `docs/design.md` lists it.
  **Unverified on device.**

- **The softkey bar has room to breathe, and the first-run hint is no longer
  jammed into the tiles**: a 10-pixel label in a 16-pixel bar left three pixels
  above and below, so the six labels read as text pressed into a slot rather
  than six buttons. The bar is 20 pixels now, dividers stop short of its top and
  bottom edges, a label keeps at least four pixels from the separator beside it
  and is ellipsised rather than run into its neighbour, and the bar grows into
  the content area on all thirteen screens at once because every screen places
  its footer relative to `TH_SOFT_H`. The menu's first-run hint also had its
  ascenders on the bottom tile border: it now has its own band, sized from the
  text, and the grid stops clear of it, so there are eight pixels above the
  words and five below. The line also lost the words "a tile" so it keeps a
  margin at both ends instead of running edge to edge. **Unverified on device.**
- **The site was not deploying at all**: `node scripts/gen-tools.mjs` has failed
  on every push since the four V2 tools landed, because each says in its own
  words what it does and `site/src/toolCopy.js` still listed twenty-two of them.
  The build reported "registry tool \"factor\" (Prime factors) has no copy" and
  stopped, so the last successful deployment predates Prime factors, Percentages,
  the Formula book and the Pi game. All four are in `toolCopy.js` now, with their
  real screenshots, and are in the "own words" set that needs proofreading.
- **The download picks the newest build by version, not by file date**: `dist/`
  is a git checkout, so every `.g3a` in it is written at the same moment and the
  order the filesystem hands back decided which one the page offered. It offered
  1.0.0 while 1.1.0 sat beside it. The fallback compares the numbers in the file
  name now, and a build's commit and date are dropped when the bytes came from a
  different file than the one `BUILD.txt` describes.

- **CodeRabbit, configured in the repository instead of a dashboard**:
  `.coderabbit.yaml` tells the reviewer what this project actually breaks on -
  `src/core` must not include gint or libm, a screen may only use the nine
  palette colours with 2 px ink borders and a 3 px shadow under the selected
  item, a new core function needs a test with a concrete vector, a regenerated
  table is not a code change, and files are written to flash only at session
  end. Generated files, images, binaries and the site lockfile are out of scope,
  poems and UML diagrams are off, and automatic approvals are off because
  merging here depends on whether anyone ran the build on a calculator.
  `tools/check_coderabbit.py` parses the YAML with its own small reader - no new
  dependency - and rejects an unknown key, a `path_instructions` glob that
  matches no file (how a rule for a renamed directory stops applying), a tool
  switched on with no config file or for a language the repository does not
  contain, tabs and CRLF. `make coderabbit-check` runs it and CI runs that.
  Installing the GitHub App is the one step a commit cannot do;
  `docs/CODERABBIT.md` has the four clicks. **Unverified on device.**
- **A version number that moves with the work, and a check that it did**: the
  tree said 1.1.0 through fourteen merged pull requests, so the About screen, the
  `.g3a` name and the website would have disagreed with the changelog the moment
  anything shipped. The rule is now that every change bumps the version in the
  same commit - patch for a fix or a doc, minor for a feature, major only for a
  storage-layout change - and the version of record is 1.2.0. `tools/
  bump_version.py` rewrites `version.txt`, the `src/version.h` fallback and the
  example file name in `CMakeLists.txt` in one command (`make bump-minor`,
  `make bump-patch`), and `--check` compares those against the changelog heading
  and the README badge, naming each file that disagrees. `make test` runs it, and
  so does CI, so a forgotten bump fails a build instead of shipping. 21 Python
  tests cover the arithmetic and the rewriting, including that a bad version is
  refused before any file is touched. The README's version badge now links to the
  releases page rather than to `releases/tag/v<version>`, because between a bump
  and a release there is no such tag and a badge pointing at a 404 is a lie about
  a page. **Unverified on device.**

- **Percentages tool**: the four questions a paper asks - percentage change,
  add or take a percentage, reverse percentage, compound growth or depreciation -
  on one Math screen, with the working written out under the answer. The point
  of the reverse question is that it divides (`34 / (1 - 15/100) = 40`), which is
  what people get wrong by adding 15% of 34 back on. `src/core/percent.c` with 41
  checks covers the vectors (500 at 4.5% for 7 years is 680.43; a car at 12% for
  5 years keeps 52.8%), zero years, a negative rate, and the three questions with
  no answer - a change from zero, a 100% discount, a period count outside 0..200
  - each of which is a sentence on screen rather than a 0 or an inf. One bug
  caught by the tests before the screen existed: the fast exponentiation gave up
  on an overflow by returning the *principal*, so a question with an unshowable
  answer said "no growth". **Unverified on device.**

- **The formula book**: the IB AA HL, Physics 2025 and Chemistry 2025 booklet
  content, as a Study tile called Formula book. The booklets are plain text in
  `Bench/formulas/`, one `.txt` per booklet, so a teacher (or you) can add one
  without a rebuild - the list shows every file in the folder and its first `#`
  line as the description. The viewer scrolls a line at a time and jumps a
  **topic** at a time with LEFT/RIGHT, headings coloured red, blue and ink by
  level. New `src/core/formula.c` holds the line and heading arithmetic with 40
  checks, including that a jump ignores `###` sub-headings - with those counted,
  a booklet with a heading every few lines moved five lines a press, which is
  not a section.
  **The content is a transcription nobody has proofread yet**: the PDFs print
  formulae as images, so the text was typed, and it is marked as unproofread in
  three places - the first line of every booklet, `docs/samples/formulas/
  needs-human-proofreading/README.md`, and the help page. Treat an unproofed
  line as unchecked.

- **The Pi game**: a Study tile. The digits you have earned stay on screen,
  grouped five to a line; type the next ones, EXE checks them, and the first
  wrong digit costs a life. Three lives, F2 gives the next digit away and costs
  one - a hint that is free is not part of a game - and the round ends with how
  far you got and the best of the session. Nothing is written to the card, so
  the best score lives and dies with the session, which is what the file-write
  rule allows. The digits are `src/core/pi.c`: 200 of them, and
  `tests/test_pi.c` checks the table against a Rabinowitz-Wagon spigot computed
  from the algorithm rather than copied - a typo in the table cannot hide by
  being duplicated in the test (40 checks, first 100 digits plus the human
  check that the 100th decimal is 9). `pi_match` stops at the first wrong digit
  so a guess that runs off the end cannot read past the table.
  **Unverified on device.**

- **Prime factors, on its own screen in Math**: factorisation existed, three
  levels down a list of nine Number theory functions, which is not where anyone
  looks for it. The new screen takes a number up to 18 digits and shows the
  factorisation written out - `360 = 2^3 x 3^2 x 5` - with the four numbers that
  follow from it: divisor count, divisor sum, totient and square-free part, plus
  how many distinct primes and whether the number is a perfect square. F2 halves
  the number and factors again, which is the question that usually comes next.
  New in `core/numtheory.c`, each with vectors checked by hand (1091 checks in
  that suite): `nt_factor_text` (exact buffer arithmetic, so a text of exactly
  the buffer's length is accepted and one byte less refused),
  `nt_divisor_count`, `nt_divisor_sum` (the geometric series without a division,
  wrapping checked), `nt_factor_radical` and `nt_factor_is_square`.
  **Unverified on device**: how long the rho routine takes on a real CPU for a
  hard 18-digit semiprime, and the screen draws no progress bar because of it.

- **About says who made it**: the page was one screenful of version numbers. It
  now says whose work it is (Sarang, `github.com/sarang-cmd`), what it is, how it
  was made - maths in plain C tested without a calculator, screens a thin layer
  over gint, every doc picture a real simulator frame, builds from GitHub
  Actions - and keeps the build facts and the Exam Mode promise at the end. The
  page is longer than the screen, so it scrolls with the bar and `UP/DOWN`;
  scrolling reuses `core/help.c`'s tested rules. Tour 84 and 85 are its two
  ends. Also: `src/version.h`'s fallback said 1.0.0 while `version.txt` said
  1.1.0, so the simulator's About screen was a release behind the add-in it was
  drawing - the fallback is 1.1.0 now. **Unverified on device.**

- **Three more settings, and three places they reach**: Settings had five rows
  and two of them were read-only information. It has eight now, and each one
  does something:
  * **Opens on** - which category the home screen starts on, so someone who
    lives in Science does not press F3 every launch.
  * **Show hint line** - the first-run line on the home screen can be turned off
    for good without having to open Help first.
  * **Pacer clock red at** - minutes remaining when the Pacer's "Left mm:ss"
    line turns red, 0 to never. Default 5.

  All three are lines in `Bench/state/settings.cfg` as well, so they can be
  edited by hand, and a file from an older build still loads: the new keys are
  absent, the defaults stand (checked in `tests/test_settings.c`, 721 checks,
  including an out-of-range hand edit being clamped rather than rejected).
  **Unverified on device.**

- **The Tools category, centred**: "so awkward" was about Tools holding two
  tiles hard against the left margin with a third of the screen empty beside
  them. A row that is not full is centred under the one above it now, which
  leaves a full row exactly where it was. `src/core/layout.c` (21 native checks:
  full and short rows, a row wider than the screen, an empty row, the per-row
  count at a page boundary). Study's fourth tile and Science's short row move
  with it, since they were the same shape of problem. **Unverified on device.**

- **The page indicator, and the missing Math title bar**: a category with more
  than six tools paged with two 4 px squares and a hole punched in the current
  one - "too subtle" was the report, and it was right: at 384 px that reads as
  dust. The indicator is now a bordered pill reading `1 / 2` with an arrow on
  each side that leads somewhere (only the live ones are drawn; a greyed one
  would be shade on paper at 1.3:1, which the screen checker rejects). The pill
  takes the space it needs only in categories that page, so every other category
  keeps the full grid. Chasing it found a second bug: the centring maths counted
  every tool in the category rather than the tools on the page, so Math's nine
  tools meant three rows of height, a negative offset, and the first row drawn
  over the title bar - the Math category had no title at all since the V2 push.
  Both are fixed, and `tests/test_pageind.c` checks the label, the arrow states
  and the clamping (31 checks), including the 10 / 12 two-digit case and a
  buffer one byte too small. **Unverified on device.**

- **A scrollbar on every screen that scrolls**: a Notes file, a deck list, the
  study log, the root finder's iteration table and a help page all scrolled with
  nothing on screen saying so. The bar is now three pixels wide (it was two, a
  hairline at 384 px), ink thumb on a shade track, 12:1 and 4:1 in the palette
  with no new colours. It is drawn only when there is something off-screen, and
  the position is a fraction of the list, so the last screen shows the thumb at
  the bottom and nothing is drawn for a list that fits. **Unverified on
  device**: the bar is checked in the simulator against the pixel rules
  (`make check-screens`), not on the calculator's panel.

- **A folder full of notes that the add-in could not see**: three matching bugs,
  all on the way *out* of the filesystem rather than in it. The glob was built as
  `*` plus the extension, which CASIOWIN's `BFile_FindFirst` answers with nothing
  on some OS versions; the extension test was case sensitive, so a Windows copy
  saved as `.MD` was invisible; and the OS's `.`/`..` entries were not filtered.
  The rules are now `src/core/filelist.c` (56 native checks: case, leading dots,
  separators, double extensions, a buffer one byte too small), the listing tries
  both wildcard spellings and filters in C, and the empty screen names the folder
  and says the case does not matter. **Unverified on device**: the two-glob retry
  exists because of how CASIOWIN is documented, and only a calculator can confirm
  which spelling its Fugue build wants.

- **EXIT goes back where you came from**: closing a tool used to land on the home
  screen with Study selected, first tile, every single time - so a Maths or
  Science session meant walking back to the right category after every tool.
  The add-in now keeps a small stack of the screens it came through
  (`src/core/nav.c`, 525 native checks over push/pop, repeat pushes, a full
  stack and 500 random presses). EXIT and the help screen's way out both walk
  back one screen at a time, and the home screen remembers the category and the
  tile you were on instead of resetting itself. Two tour scripts record it:
  `78_back_to_maths` and `79_back_from_settings_about`.

- **The browser simulator has the calculator's keypad**: it was a flat 5-column
  grid that followed no physical keyboard - DEL next to 9, ALPHA where x² is,
  arrows spread over two rows. It is now the fx-CG 50 layout: six function keys
  with the case's own captions, the SHIFT/OPTN/VARS/MENU and ALPHA blocks, the
  four-way pad on the right where the real one sits, and 7-8-9-DEL-AC /
  4-5-6-×-÷ / 1-2-3-+-(−) / 0-.-×10ˣ-(−)-EXE. Keys Bench does not read are drawn
  as muted hardware rather than pretending to work; every live key is a real
  button with an aria-label. Checked at 320, 390 and 1280px: no horizontal
  scroll, smallest key 26px.

- **The diagnostic add-in is on the site and in `dist/`**: `Bench-spike-0.0.2.g3a`
  is now published beside Bench's own `.g3a` - in `dist/` with a `SPIKE.txt`
  manifest, and on the downloads page under "The diagnostic add-in" - instead of
  being reachable only as a CI artifact on blob storage. The patch
  (`spike.diff`) is downloadable from the same place, so the spike can be had
  even when no binary has been built. It is still labelled as a diagnostic
  everywhere and still never appears on a release or next to the install button.

- **The hero band stops cutting the `g`**: the yellow behind "in one" was
  clipping the descender of the `g` in "eighteen". Two causes: Tailwind's
  `text-5xl`/`text-6xl` set their own line-height and were beating the `h1`'s
  `leading-[1.35]`, so the heading ran on a 1.0 line box; and the band was an
  inline box, which paints over the font's content area, which at 60px is taller
  than the line. The heading now sets its line-height at every breakpoint and
  the band is an inline-block at line-height 1. `site/scripts/check-band.mjs`
  counts the neighbour pixels the band covers - 331 at 1280px before, 0 at every
  width from 320 to 1440px now - and runs in CI after the width sweep.

## 1.1.0 - 2026-11-01

The two bugs from the first hardware session, the V2 tools, and the machinery
that stops the next one.

**Run on a device** (added after the fact, see 1.2.4): the owner of the hardware
put `Bench-1.1.0.g3a` on the calculator and every tool in this release worked.
The one defect was the bottom bar - labels pressed into their slots and the
first-run hint on the tile borders - fixed in 1.2.3. The "unverified on device"
notes on the entries below were written before that run; the per-entry wording is
left as it was, and `docs/HARDWARE.md` carries the result.

- **A hardware report you can fill in on the calculator (V2, workstream J)**:
  Settings gained **F3**, which opens a screen listing the eight assumptions
  Bench makes about the CASIO filesystem, the font, ALPHA, the clock and the
  frame time. Each row starts at `not tried`; LEFT/RIGHT marks it `works` or
  `fails` after the matching test in `docs/HARDWARE.md`. F2 writes
  `state/hwreport.txt` in plain text, so the result comes off the calculator by
  copy rather than by retyping, and nothing is written at any other moment.
  The formatter is `src/core/hwreport.c` (63 native checks, including a buffer
  that is one byte too small), and `docs/HARDWARE.md` says how the file slots
  into the report.

- **A shared number field and a history of answers (V2, workstream A5)**:
  `src/core/numedit.c` is the one place that decides what a numeric field
  accepts - digits, at most one dot, a lone zero replaced rather than stacked, a
  leading dot turned into `0.` - with `numedit_valid()` for "would numparse read
  this?" (54 checks). The moles screen was carrying its own copy, which is how
  `1.2.3` got in; it now uses the shared one. `src/core/history.c` keeps the last
  eight formatted results (33 checks) and the Root finder pushes every solve onto
  it, with F4 to show the list - a panel, not a file, so nothing is written to
  flash.

- **The simulator runs in the browser (V2, workstream G)**: `tools/wasm/build.sh`
  compiles the add-in and `tools/sim/sim.c` - the same sources the screenshots
  come from - to a 300 KiB WebAssembly module. No emscripten and no Asyncify:
  zig carries clang, wasm-ld and wasi-libc, so the toolchain is one
  `pip install ziglang`. Blocking `getkey()` is fine because the module runs in
  a Web Worker, and keys and frames cross to the page through a
  SharedArrayBuffer (the deployment sends the two headers that allows). The page
  is `site/src/components/WasmSim.jsx`, the other half is
  `tools/wasm/worker.js`, and `tools/wasm/smoke.mjs` runs the module in Node -
  real keys in, real frames out as PPM - so the build is checked without a
  browser. The binary is committed (`site/public/sim/bench.wasm`) because Vercel
  has no C compiler; CI rebuilds it and fails if the committed copy is stale.

- **Browser widths in CI (V2, workstream B)**: `site/scripts/check-widths.mjs`
  loads all eight routes at 320, 390, 768, 1024 and 1440 px and fails on a
  horizontal scrollbar, an element that crosses the right edge or one wider than
  the viewport. Chromium could not be downloaded here, so the sweep has only run
  in CI; the result of that first run is in the pull request.

- **Demo content (V2, workstream D)**: `demo/` holds three notes, three decks
  and a fortnight study log so a fresh install is not three empty screens. The
  text is written from scratch, not copied from a booklet or a past paper, and
  `demo/README.md` says in its first section that it needs human proofreading.
  `tests/test_demo_files.c` reads every demo file with the add-in's own parsers
  (108 checks) and fails on a single skipped line, so the sample cannot rot into
  something the calculator refuses to read.

- **The site is eight pages, not one (V2)**: `/features`, `/simulator`,
  `/tutorial`, `/install`, `/downloads`, `/changelog` and `/about` join the
  landing page, which keeps its layout. `src/router.jsx` is a 120-line path
  switch with real `<a href>`s, so back, forward, middle-click and a shared link
  all work; `vercel.json` rewrites every path to `index.html`. Every route sets
  its own title, description, canonical and Open Graph image.
  `scripts/gen-stats.mjs` and `scripts/gen-changelog.mjs` write the numbers and
  the changelog from `CHANGELOG.md`, `docs/tools.json` and `src/`, and
  `scripts/check-pages.mjs` renders all eight routes in Node and fails the build
  if one throws. Eight 1200x630 Open Graph cards come from
  `tools/brand/make_brand.py`. The canonical URL is now
  `bench-gdc.vercel.app` everywhere; `sitemap.xml` lists all eight
  routes.

- **The simulator page is honest about the simulator**: the interactive
  WebAssembly build (workstream G) is not done, and the page says so instead of
  pretending. What it shows today is the real frame buffer: 75 captures made by
  the add-in's own simulator.

- **Settings, five rows (V2)**: `src/core/settings.c` gains two keys,
  `angle_unit` (0 degrees, 1 radians) and `tour_done` (0 or 1). The screen now
  lists the unit the Triangle tool opens in and whether the first-run line on
  the home screen has been read; `app_triangle_init()` takes its starting unit
  from the setting, and F3/F4 still switch for that session only, without
  writing the file. A settings file from 1.0.0 still loads: the new keys fall
  back to their defaults. 687 native checks in `tests/test_settings.c`.
  Unverified on device.

- **First-run hint (V2)**: with `tour_done 0` the home screen prints one line
  above the softkeys - "F1-F4 categories, F5 explains a tile, EXE opens it" -
  and opening help with F5 sets `tour_done` and writes the file once.

- **Complex numbers (F6)**: `src/core/cplx.c` adds, subtracts, multiplies,
  divides (division by zero is a NaN, not a wrong number), raises to an integer
  power and reports the modulus, the argument (with `bm_atan2`, new in
  `src/core/mathfn.c`) and the polar form. `src/apps/app_cplx.c` shows all three
  at once, F2 cycles the operation, F3/F4 pick degrees or radians in the title
  bar. 37 native checks. Unverified on device.

- **The M1 spike has its own artifact.** It used to ride inside `bench-g3a`,
  next to the add-in, so the thing you download to get Bench offered two
  `.g3a` files and one of them printed diagnostics. `bench-g3a` now holds
  `Bench-<version>.g3a` and nothing else; the spike is in `bench-spike`. It was
  never a release asset and still is not.

- **Root finder (F4)**: `src/core/roots.c` does bisection, Newton (numeric
  derivative, step 1e-6) and the secant method, keeping every iterate so the
  screen can show n, x, f(x) and the step size. `src/apps/app_roots.c` solves
  from the shared expression engine, F2 cycles the method, F3 solves, and a
  small plot marks each iterate on the curve. A bracket with no sign change, a
  zero slope and a parse error each get their own sentence and column. 40
  native checks. Unverified on device.

- **Statistics with working (F7)**: `src/core/stats.c` gives n, sum, mean,
  median, mode, range, both quartile methods, the outlier fences and the
  population and sample standard deviations (53 native checks). Paired data
  gets the least squares line, r and r squared. `src/apps/app_stats.c` adds
  values as you type them, shows the summary and a box plot, has a screen for
  the working and a screen for the regression. Unverified on device.

- **SUVAT and projectiles (F10)**: `src/core/suvat.c` solves any three of s, u,
  v, a and t, one table entry per triple, and reports the equation each answer
  came from; when a quadratic gives two sensible times it keeps the first and
  names the second (73 native checks). `src/apps/app_suvat.c` fills the grid as
  you type, marks worked-out values with a dot, and draws the flight path for a
  launch speed, angle and height. Unverified on device.

- **Moles and solutions (F11)**: `src/core/chem.c` covers n = m / Mr, c = n / V,
  pV = nRT with R in kPa dm3, dilution and the two percentages, refusing a zero
  where a formula divides (47 native checks). `src/apps/app_moles.c` has one
  mode per family, opens each on a worked example, and writes the substitution
  out under the answer. Unverified on device.


- **Base and bits (F12)**: `src/core/base.c` parses and prints any base from 2
  to 36, writes two's complement at 8, 16 and 32 bits (hex padded to the width,
  binary grouped in fours), and does AND, OR, XOR and logical shifts, refusing a
  shift of 63 or more (78 native checks). `src/apps/app_base.c` shows one value
  in five bases with all three widths, and a second tab for the operators; F2
  opens the letter keyboard because the keypad has no A-F keys. Unverified on
  device.


- **Lab graph (F8)**: `src/core/graph.c` fits a line to measured points and
  quotes the standard error of the gradient and the intercept from the
  residuals, plus a round axis range for the plot (42 native checks).
  `src/apps/app_graph.c` takes x, y and an error per point, lists them with the
  fit, and draws the graph with a bar through each point and the line of best
  fit. Unverified on device.


- **Study log (F13)**, **Notes v2 (F14)** and **Cards v2 (F15)**: a plain text
  log of what you studied and for how long, with totals, days logged and the
  longest streak (`src/core/logbook.c`, 56 native checks); the Notes footer now
  counts words as well as lines (`textsearch_words`, 6 more checks); and a
  Cards deck can be drilled in a mixed order inside each box
  (`leitner_shuffle`, deterministic for a session, 8 more checks). Bench cannot
  read the calendar - gint's date fields were never checked on the hardware - so
  a log entry takes the day after the newest one and UP/DOWN moves it.
  Unverified on device.


### Fixed
- **Home tiles: the icon vanished on the selected tile.** The icon was painted
  in the tool's accent colour on a tile filled with that same colour. Every
  foreground is now chosen by measurement (`col_better_on`), the icon is drawn
  with a 2 px outline, and a check samples the pixels afterwards. This is
  unverified on device: nobody has looked at the new screen on a calculator yet.
- **Labels drawn in a colour too close to their background.** Ink on red is
  2.75:1, paper on teal 3.00:1, a red cursor on the yellow field 2.91:1. Text is
  now ink or blue on paper, and a selected tile is ink with paper text instead
  of a tinted tile with a paper plate under the label.
- **The keyboard covered half of the note behind it.** The modal now clears the
  content area first, and the space key is marked instead of being drawn as an
  empty cell.
- **A 57-character file name ran across the whole title bar.** `w_title()` now
  cuts the title to the room it has, and `STORE_NAME_MAX` went from 48 to 72 so
  such a file is listed at all.
- **Markdown code spans painted over the character before them**, and code was
  teal on shade (2.28:1). The background starts after the previous span, and
  code is ink on shade.

### Added
- **The .g3a lives in the repository**, at `dist/Bench-<version>.g3a`, with
  `BUILD.txt` recording the commit, size and SHA-256. Release assets and
  artifacts both come from blob storage, which some networks cannot reach; a git
  remote is plain git. CI refreshes `dist/` for tagged and automatic builds with
  a `[skip ci]` commit, so the copy does not go stale.
- **The M1 spike as one patch**: `spike/spike.diff` holds the whole diagnostic
  add-in, generated from `spike/` by `tools/gen_spike_patch.py`
  (`make spike-patch`). `git apply spike/spike.diff` on a tree without `spike/`
  reproduces it byte for byte; CI fails if the patch and the sources disagree.
  The spike still is not on any release.
- **Home screen v2**: four categories on F1..F4, a 3 x 2 page of tiles, page
  dots when a category has more than one page, and a dashboard strip on Study
  that counts the decks and notes on the card. The selected tile keeps the
  tool's accent as its fill, a 3 px border and a hard shadow; everything on it
  is coloured by measurement, and a subtitle the palette cannot carry gets a
  paper plate hugging the words. Unverified on device.
- **Sequences and binomials (F5)**: `src/core/seq.c` does arithmetic and
  geometric terms and sums, the sum to infinity, exact `C(n,k)` (cancelling
  before multiplying, so it can answer up to n = 62 instead of wrapping) and
  Pascal's rows. `src/apps/app_seq.c` is three screens with the first few terms
  listed under the answers. 41 native checks. Unverified on device.
- **Triangle solver (F2)**: `src/core/triangle.c` solves SSS, SAS, ASA/AAS and
  SSA from whatever is given, with the ambiguous SSA case reporting two
  triangles rather than silently picking one. `src/apps/app_triangle.c` is six
  fields, the answers written back into them, area and perimeter underneath,
  and F2 to swap between the two SSA triangles. 42 native checks.
  Unverified on device.
- **Polynomials (F3)**, quadratics and the arithmetic behind them:
  `src/core/poly.c` parses and prints polynomials, adds, subtracts, multiplies,
  expands `(ax + b)^n`, divides with quotient and remainder, solves a quadratic
  with real or complex roots, and factors over the rationals with the rational
  root theorem. `src/apps/app_poly.c` is three screens - quadratic (disc,
  roots, vertex, factor pair), expand, and roots from typed coefficients.
  83 native checks. Unverified on device.
- **Sig figs and standard form (F9)**, the second of the fifteen:
  `src/core/sigfig.c` counts figures on the text (so `1200` is 2 and `1200.` is
  4), rounds half-up on the decimal digits, and writes standard form and SI
  prefixes without a unit of its own. `src/apps/app_sigfig.c` has six modes -
  count, round to n figures, round to n decimals, standard form, SI prefix and
  a practice question with a score. 41 native checks. Unverified on device.
- **Home screen: one clean block.** A page shows only the rows it needs, the
  grid is centred in what is left, and the softkey strip reads Study, Math,
  Science, Tools, Help, About - F5 is help for the selected tile here as it is
  on every other screen, so Settings is a tile and not a second meaning for F5.
  Unverified on device.
- **Function table (F1)**, the first of the fifteen: `src/core/expr.c` is a
  shunting-yard parser (fixed 32-entry stack, implicit multiplication, column
  numbers on every error), `src/core/exprs.c` reads and writes the six slots,
  and `src/apps/app_function.c` is the slot list, the table and the probe.
  Unverified on device.
- `src/core/mathfn.{c,h}`: sqrt, exp, ln, log10, pow, sin, cos, tan and the
  degree conversion in plain C — no libm, no gint, so the calculator build and
  the host tests run the same code. 85 native checks against the textbook
  values. This is the ground the expression engine (A5) stands on.
- `src/core/color.{c,h}`: 5-6-5 packing, WCAG luminance and contrast, no libm.
  `lin_table.h` is generated by `tools/gen_lin_table.py`. 80 native checks.
- `src/apps/registry.{c,h}`: the single list of tools (id, title, subtitle,
  category, accent, icon, screen, help). The home screen reads it, and
  `tools/gen_tools_json.py` writes `docs/tools.json` from it via the host build.
- A home screen with categories on F1..F4, a 3 x 2 tile grid, pages with dots,
  a dashboard strip on Study, and the selected tile's hard shadow. This is
  unverified on device.
- **Help on every screen (F5).** One page per tool, rendered from the tool's
  registry row: no second copy to drift. Paging (count, copy, clamp) is
  `src/core/help.c`, 37 native checks. On the home screen F5 shows the help for
  the tile under the cursor; in a tool it shows that tool's help; EXE or EXIT
  returns to the screen that asked. Unverified on device.
- Screens with no softkeys draw a plain hint strip (`w_strip`) instead of six
  empty divider cells, and an empty F5 slot advertises F5 as help.
- `make check-screens` (part of `make test`): every key script is checked for
  overlapping labels, labels off-screen, labels in the title bar or softkey
  strip, labels painted over, text under WCAG AA, softkey labels too wide, and
  icons that match their tile. 30 scripts, 0 findings.
- Worst-case screens: a 120-character deck, a 57-character file name with
  89-character table cells.
- `docs/design.md`: the palette with measured contrast ratios, the rules, and
  the checks that enforce them.

### Changed
- `STORE_NAME_MAX` 48 -> 72. Storage layout unchanged: existing files are read
  as before, nothing moves.
- `theme.h` colour constants regenerated from `col_rgb565()`; six of the eight
  were one step out.

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
