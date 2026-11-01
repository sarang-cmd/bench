# Contributing to Bench

Bench is a personal project that happens to be public. Bug reports and small,
focused pull requests are welcome; large rewrites are not.

## Before you change anything

Read `AGENTS.md`. It has the rules the code follows, and the ones it must not
break: `src/core` has no gint includes, no dependency is added without a reason,
and the add-in never writes to the calculator outside a session end or a settings
change.

## The checks

Everything below must pass before a pull request is merged:

```sh
make test          # native core tests + the screen-overlap check
make py-test       # the tools/ scripts
make sim           # the whole add-in against the PC stand-in for gint
make gint-check    # type-check against real gint headers (needs fxSDK)
make check-screens # 75 tour scripts, no overlapping text
```

The site is checked too:

```sh
cd site
npm ci
npm run gen-tools && npm run gen-stats && npm run gen-changelog
node scripts/check-pages.mjs
npm run build
```

The browser sweep needs Chromium and a preview server:

```sh
cd site
npm run build
npx vite preview --port 4173 &
npx playwright install chromium
node scripts/check-widths.mjs --url http://localhost:4173   # layout at 320-1440px
node scripts/check-band.mjs --url http://localhost:4173     # the hero's yellow band
```

`check-band.mjs` is the narrow one: it hides the band's own words, keeps its
box, and counts the neighbour ink the yellow covers. A bounding-box sweep cannot
see that bug, because the boxes never touch - the glyph just overflows its box,
which is how the band ended up cutting the tail of the g in "eighteen".

The WebAssembly build needs zig, which carries clang, wasm-ld and wasi-libc:

```sh
pip install --user ziglang
export ZIG="$(python3 -c 'import os, ziglang; print(os.path.join(os.path.dirname(ziglang.__file__), "zig"))')"
sh tools/wasm/build.sh                  # writes build-wasm/bench.wasm + demo.json
node tools/wasm/smoke.mjs "down,exe"    # runs it in Node, frames to /tmp/*.ppm
cd site && npm run wasm                 # copies it into public/sim/ and commits
```

`site/public/sim/bench.wasm` is committed on purpose: Vercel cannot compile C, so
the deployed simulator needs the bytes in the repository. CI rebuilds the module
and fails when the committed copy no longer matches the C.

## Generated files

Do not edit these by hand; regenerate them and commit the result:

| File | Regenerate with |
|---|---|
| `docs/tools.json` | `python3 tools/gen_tools_json.py` |
| `spike/spike.diff` | `python3 tools/gen_spike_patch.py` |
| `site/src/tools.generated.json` | `node site/scripts/gen-tools.mjs` |
| `site/src/stats.generated.json` | `node site/scripts/gen-stats.mjs` |
| `site/src/changelog.generated.json` | `node site/scripts/gen-changelog.mjs` |
| the README's badge and stats blocks | `make stats` |
| `docs/images/*.png` | `make shots` |

CI runs each of them and fails when the working tree changes, so a hand edit is
caught before it reaches `main`.

## Adding a tool

1. `src/core/<name>.c` and `.h` with no gint, plus `tests/test_<name>.c` with
   concrete vectors.
2. `src/apps/app_<name>.c` with `app_<name>_init/draw/key/timeout`.
3. One row in `src/apps/registry.c`, one `APP_SCREEN_*` in `app.h`, one
   `VIEW_*` in `main.c`.
4. A help string for the tool in `registry.c`, a tour script in
   `tools/sim/tour/`, a CHANGELOG entry, a README section and a site copy block
   in `site/src/toolCopy.js`.

If a step feels like a list maintained in two places, it probably is: say so in
the pull request instead of adding a second list.

## Commit messages

Explain what changed and why, in the present tense. "Fix" is not an explanation.
