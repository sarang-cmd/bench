# Versions

`version.txt` is the one true value. Everything else reads it or is generated
from it: the `.g3a` file name (CMake), the About screen (`src/version.h`, which
CMake overrides), the README badges and stats, the site's generated data, and the
heading of the changelog section the release is cut from.

## The rule

**Every change bumps the version, before it is committed.** Not at release time,
not when someone remembers. A branch whose diff is bigger than a typo and whose
`version.txt` is unchanged is a branch that has not finished.

| the change | the bump | example |
|---|---|---|
| a fix, a wording change, a test, a doc correction | **patch** | 1.2.0 → 1.2.1 |
| a new tool, a new screen, a new setting, a new file format | **minor** | 1.2.0 → 1.3.0 |
| the storage layout, or the meaning of a key already in `settings.cfg` | **major** | 1.2.0 → 2.0.0 |

Two patches in a row are normal. A feature that also fixes something is a minor,
because the feature is what someone installing the new build gets.

## How

```
python3 tools/bump_version.py minor      # or patch / major, or --set 1.4.2
python3 tools/gen_readme_stats.py
cd site && npm run gen-stats && npm run gen-changelog
make version-check
```

Or, from the top: `make bump-minor`, then `make docs`, then `make version-check`
(which `make test` runs anyway).

Add the changelog section as part of the same change, named after the version:

```markdown
## 1.3.0 - unreleased (source only)
```

The first line says "source only" until there is a build, at which point it
becomes `## 1.3.0 - 12 November 2026` and the tag `v1.3.0` carries the `.g3a`.

## Why it is checked, not remembered

A version that lives in six files drifts the moment one of them is edited by
hand. `make version-check` compares `version.txt` against `src/version.h`, the
example file name in `CMakeLists.txt`, the changelog heading and the README
badge, and fails with a sentence for each that disagrees. It runs in `make test`
and in CI, so a forgotten bump fails a build instead of shipping.

## What a bump is not

A bump does not mean a release, and it does not mean anything was tested on a
calculator. The changelog section says which build the numbers came from; until
there is one, it says "source only".
