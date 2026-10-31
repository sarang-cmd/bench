# M1 spike

**Not part of Bench, and not on any release.** This is a diagnostic add-in: it
runs ten measurements on the real calculator and prints the answers on screen.
It existed so the code in `src/` could be written against measured facts instead
of guesses (file API, paths, float printing, font coverage, key values, tick
rate, heap, drawable area, redraw cost, empty-add-in size). Bench does not call
it and does not need it at runtime.

It is still built by CI, so it stays compiled and available when a hardware
session needs it, but it travels in the `bench-g3a` artifact only:

```sh
gh run download --name bench-g3a --dir dist    # Bench-spike-*.g3a is in there
```

Keeping it off the release page is deliberate: two `.g3a` files with similar
names, one of which prints debug numbers, is a trap for anyone who just wants
the add-in. `dist/` in this repository holds Bench's `.g3a` and only that, for
the same reason.

## The patch

`spike/spike.diff` is the whole spike - `spike/main.c` and
`spike/CMakeLists.txt` - as one patch, so it can be carried into a checkout, a
toolchain container or a support thread without the rest of the repository:

```sh
git apply spike/spike.diff     # from the repository root, on a tree without spike/
cd spike && fxsdk build-cg
```

It is generated from the sources (`make spike-patch`, which runs
`tools/gen_spike_patch.py`) and CI fails when the committed copy is stale, so
the two cannot drift apart. Applying it to a tree that already has `spike/`
fails on purpose rather than overwriting anything: `git apply` refuses to touch
existing files.

---

Throwaway add-in that answers the ten questions in docs/plan.md section 7.
Everything it prints is a fact to be read off the real calculator; nothing here
is a guess.

* Add-in: `Bench-spike-0.0.2.g3a` (source: `spike/main.c`)
* Menu name on the calculator: `BenchSpk`
* Build: `cd spike && fxsdk build-cg` (see docs/toolchain.md)
* Copy: USB Flash mode, drag the file to the calculator drive, run it from the
  main menu

The build has **not** been run here: the sandbox that wrote this had no SuperH
toolchain (see docs/toolchain.md section 5). The CI workflow builds it, or
`fxsdk build-cg` in `spike/` does. The source is type-checked against the real gint
2.10.0 headers with the host compiler and `-Wall -Wextra`, which proves the
call names and signatures, not the SuperH code generation.

## How to run

| Key | Action |
|-----|--------|
| UP / DOWN | choose one of the ten tests |
| EXE | run the chosen test |
| any key | leave a result screen |
| EXIT | leave the add-in |

Write the answer to each question in the **Your result** line below it. If a
test shows an error code where a number was expected, write the code.

Photographs are fine; write the numbers into the file if you prefer.

## Result table

| # | Question | Answer wanted | Your result |
|---|----------|---------------|-------------|
| 1 | Which file calls list, read, write, delete? | the stack of calls that worked, and their return values | |
| 2 | How do paths look? Folder separator? | does `\\fls0\Bench\...` work, and is `Bench` created | |
| 3 | Does `printf`/`snprintf` with floats work? | the three lines printed by test 3, verbatim | |
| 4 | Which symbols does the default font have? | which of the nine samples draw, and which show `?` | |
| 5 | How do you read ALPHA letters? | the key/alpha values printed for A, B, C | |
| 6 | Which call ticks once per second? | does the 1 Hz counter advance once per second, and the ticks passed | |
| 7 | How much heap does `malloc` give? What do the arenas add? | the three KiB figures | |
| 8 | Exact drawable width and height? | is the drawn frame fully visible on all four sides | |
| 9 | How long does a full-screen redraw take? | milliseconds per frame | |
| 10 | What size is the empty add-in? | file size in bytes from Explorer | |

## Test by test

### 1. File calls: create and write

Menu entry `1 file calls (create/write)`.

Calls, all from `gint/bfile.h`, run inside `gint_world_switch()` because
`bfile.h` states BFile cannot be used from within gint:

| Call | Header | Purpose |
|------|--------|---------|
| `BFile_Create(dir, BFile_Folder, NULL)` | bfile.h | make `\\fls0\Bench` |
| `BFile_Create(file, BFile_File, &size)` | bfile.h | make the test file |
| `BFile_Open(path, BFile_WriteOnly)` | bfile.h | open it |
| `BFile_Write(fd, data, 16)` | bfile.h | write 16 bytes (even, as bfile.h warns) |
| `BFile_Close(fd)` | bfile.h | close it |

The screen shows the return value of each step.

Your result:

```

```

### 2. File calls: read, list, delete

Menu entry `2 file calls (read/list/delete)`.

| Call | Header | Purpose |
|------|--------|---------|
| `BFile_Open(path, BFile_ReadOnly)` | bfile.h | reopen |
| `BFile_Size(fd)` | bfile.h | file size |
| `BFile_Read(fd, buf, 16, 0)` | bfile.h | read 16 bytes at offset 0 |
| `BFile_FindFirst(glob, &h, found, &info)` | bfile.h | list `\\fls0\Bench\*` |
| `BFile_FindNext(h, found, &info)` | bfile.h | next entry |
| `BFile_FindClose(h)` | bfile.h | close the search |
| `BFile_Remove(path)` | bfile.h | delete the test file |

Note for the M2 store layer: on the fx-CG series the filesystem is Fugue, and
`bfile.h` says gint then supports the Unix and C99 file APIs instead, still
behind a world switch. This test uses BFile so the answers cover both.

Your result:

```

```

### 3. `printf` with floats

Menu entry `3 printf with floats`. Uses `snprintf` with `%.2f`, `%f`, `%.3e`
and one `dprint("%f", 1.5)`.

Plan impact if this fails: `src/core/numfmt.c` already formats every number
without printf, so the Uncertainty tool and the Pacer are safe either way. The
answer only decides whether the version screen may use `%d` freely and whether
we ever print a float directly.

Your result:

```

```

### 4. Font symbols

Menu entry `4 font symbols`. Nine UTF-8 samples are drawn, and `dsize()`
(`gint/display.h`) reports each width in pixels:

| Sample | Code point | ASCII fallback already in `tools/md2bench.py` |
|--------|-----------|-----------------------------------------------|
| °C | U+00B0 | `degC` |
| ±0.1 | U+00B1 | `+/-0.1` |
| ×3 | U+00D7 | `x3` |
| µm | U+00B5 | `um` |
| αβγ | U+03B1..3 | `alpha beta gamma` |
| √2 | U+221A | `sqrt2` |
| ≤ | U+2264 | `<=` |
| π | U+03C0 | `pi` |
| → | U+2192 | `->` |

If a sample draws as `?` or as a blank, the converter already has a mapping
for it, so nothing breaks; the answer only tells us which mappings are needed
and which can be dropped from the notes.

Your result:

```

```

### 5. ALPHA letters

Menu entry `5 ALPHA letter reading`. gint 2.10.0 has no `KEY_A`..`KEY_Z` and no
`keycode_alpha()` helper (checked in `gint/keycodes.h` and
`gint/keyboard.h`); `key_event_t` reports the physical `key`, plus `alpha` and
`shift` bits. This test prints `key=0x.. alpha=.. shift=..` for each press.

Please press: ALPHA then the A key, ALPHA then B, ALPHA then C, and any three
other letters you like. Then EXIT.

Plan impact: if the letters come out as distinct keycodes with `alpha=1`, the
Notes search field and the Pacer number field can use a small table written by
us. If not, the fallback in the plan stands: pick from a list, no typing.

Your result:

```

```

### 6. One tick per second

Menu entry `6 clock: one tick per second`. Uses `gint/rtc.h`:

| Call | Header | What it gives |
|------|--------|---------------|
| `rtc_ticks()` | rtc.h | 128 Hz counter since midnight (header text) |
| `rtc_get_time(&t)` | rtc.h | `hours`, `minutes`, `seconds`, and `ticks` (128 Hz sub-second) |
| `rtc_periodic_enable(1, GINT_CALL(cb))` | rtc.h | 1 Hz periodic interrupt |
| `rtc_periodic_disable()` | rtc.h | stop it |

Watch the `1 Hz counter` while the RTC seconds change, wait about five
seconds, then press a key.

Your result:

```

```

### 7. Heap

Menu entry `7 heap size`. Uses `gint/kmalloc.h` and `stdlib.h malloc`:

| Call | Header | Purpose |
|------|--------|---------|
| `kmalloc_max(&size, "_ostk")` | kmalloc.h | largest block in the OS-stack arena (name from gint's kernel.c) |
| `kmalloc_max(&size, "_uram")` | kmalloc.h | largest block in the static-RAM arena |
| `kfree(p)` | kmalloc.h | release those blocks |
| `malloc(size)` / `free(p)` | stdlib.h | largest single allocation that succeeds |

Plan impact: section 6.4 caps a note at 32 KiB until this answer arrives. If
the largest block is well above that we raise the cap; if it is below, the
loader streams by line instead.

Your result (three KiB figures):

```

```

### 8. Drawable size

Menu entry `8 drawable size`. Prints `DWIDTH` and `DHEIGHT` from
`gint/display-cg.h` (396 and 224), then draws a frame exactly around the OS
window the header describes (384x216 at offset 6,4) and a second rectangle on
the whole surface.

The question is which of the two frames the calculator really shows, and
whether any of the outer frame is cut off.

Your result:

```

```

### 9. Full-screen redraw time

Menu entry `9 full redraw time`. Runs 50 `dclear` + `drect` + `dtext` +
`dupdate` frames between two `rtc_ticks()` reads and prints the ticks and the
milliseconds per frame (128 ticks = 1 second).

Plan impact: the plan says "Redraw on key press only; Pacer redraws once per
second". If a frame costs more than about a third of a second, the notes
viewer will redraw only the changed lines.

Your result:

```

```

### 10. Size of the empty add-in

Menu entry `10 empty add-in size`. The add-in cannot reliably stat its own
file (its stored name is set by the g3a header), so the test prints what it can
and asks for the Explorer figure.

Your result:

```

```

## After the results arrive

1. Fill in `docs/spike.md` and set the answers in the M1 row of
   docs/STATUS.md.
2. List every plan assumption the results change and update docs/plan.md and
   CHANGELOG.md (Stage 4).
3. Then TASK-M2 (shell), version 0.0.3.

## Status of these answers

Nothing in the result column has been filled in, and it will not be filled in by
an agent: every line needs the real calculator. When the answers arrive, the
files to update are `docs/plan.md` (the assumptions table in section 2 and the
loader rules in section 6.4), `docs/STATUS.md` (the "Assumptions that still need
the calculator" table) and `CHANGELOG.md` (a short note under the next version).

Until then the code takes the safe side of each question:

| # | Question | What the code does without the answer |
|---|----------|----------------------------------------|
| 1, 2 | file calls and paths | one place, `src/platform/store.c`; every call inside `gint_world_switch()`; paths as `u"\\\\fls0\\Bench\\..."` |
| 3 | float printf | never used; `src/core/numfmt.c` formats every number |
| 4 | font coverage | ASCII only; anything above 0x7E draws as `?` and is counted in the Notes footer |
| 5 | ALPHA letters | not used; text entry is the on-screen keyboard in `src/ui/widgets.c` |
| 6 | 1 Hz RTC | `rtc_periodic_enable` is only used by the spike; the add-in uses `rtc_ticks()` |
| 7 | heap | three buffers, allocated once per screen, freed on the way out |
| 8 | geometry | 396x224 framebuffer, 384x216 window, offset added in `src/platform/screen.c` |
| 9 | redraw time | Pacer redraws at 4 Hz, everything else only on a key |
| 10 | `.g3a` size | gint dominates it; CI reports the number on every build |
