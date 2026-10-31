# Releasing Bench

The add-in binary is built by `.github/workflows/build.yml`; nothing in this
repository is a checked-in `.g3a`. The workflow needs the fxSDK, the SuperH
compiler and gint, all of which come from `gitea.planet-casio.com` — a GitHub
runner can reach that, a sandbox usually cannot.

## Cutting a version

```sh
echo 1.0.1 > version.txt                 # the only place the number lives
git commit -am "1.0.1"
git tag v1.0.1
git push origin main --tags              # the tag triggers the build + release
```

What happens then, in `.github/workflows/build.yml`:

1. `native-tests`: `make test`, `make py-test`, then `make sim` and `make shots`
   so the screenshots are regenerated with the code that is being released. The
   PNGs are kept as an artifact.
2. `g3a`: installs fxSDK, `sh-elf-binutils`, `sh-elf-gcc`, OpenLibm, fxlibc and
   gint with GiteaPC, reads the version from the tag, runs `fxsdk build-cg`,
   builds the M1 spike add-in too if `spike/` is still present, and uploads the
   files.
3. On a `v*` tag it also creates the GitHub release and attaches
   `Bench-<version>.g3a` (and `Bench-spike-<version>.g3a` when it exists).

## If the tag push does not start a run

A push made with an app token (for example from an automation sandbox) does not
trigger workflows. Start the build by hand instead:

```sh
gh workflow run build.yml --ref main -f version=1.0.1
gh run watch
gh run download --name bench-g3a --dir dist
gh release create v1.0.1 dist/*.g3a --title "Bench 1.0.1" --notes-file <file>
```

## If the build itself fails

Read the `g3a` job log. The usual causes, in order:

| Symptom | Cause |
|---|---|
| `find_package(Gint)` fails | gint was installed before the compiler; reorder the GiteaPC lines |
| `fxsdk: command not found` | `$HOME/.local/bin` is not on `PATH` for that step |
| `generate_g3a: ICONS expects exactly 2 files` | `assets-cg/icon-*.png` missing; run `python3 tools/brand/make_brand.py` |
| `undefined reference to pow` | a libm call slipped into `src/core`; the core must stay libm-free except in the tests |
| version mismatch | `version.txt` and the tag disagree; the tag wins in CI |

## Checking a release before you publish it

1. Copy `Bench-<version>.g3a` to the calculator in USB Flash mode.
2. Run the checklist in `docs/STATUS.md` → "What needs the calculator".
3. Fill in `docs/spike.md` while you are there; the ten answers decide whether
   any of the assumptions in `docs/STATUS.md` need changing.
4. Only then publish the release.
