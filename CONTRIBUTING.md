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
