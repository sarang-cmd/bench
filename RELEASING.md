# Releasing Bench

The add-in binary is built by `.github/workflows/build.yml`; nothing in this
repository is a checked-in `.g3a`. The workflow needs the fxSDK, the SuperH
compiler and gint, all of which come from `gitea.planet-casio.com` — a GitHub
runner can reach that, a sandbox usually cannot.

## Automatic builds

`.github/workflows/auto-build.yml` refreshes a rolling release called `auto`
without anybody cutting a version. It runs on a schedule and is the reason the
download does not go stale.

| Question | Answer |
|---|---|
| How often? | checked daily at 03:30 UTC; builds when the last automatic build is **3 or more days old** |
| Always? | no - if `main` has not moved since the last one, it skips in about 15 seconds (same code, same file) |
| Where does it go? | release `auto`, always exactly one: the previous release and its tag are deleted, then recreated at the commit that was just built |
| Which version? | whatever `version.txt` says; an automatic build is identified by its build date, not by a new number |
| What is in the notes? | the commit it was built from and the recent history, nothing hand-written |
| What if it fails? | nothing is deleted until the build is green, so the previous good build stays downloadable; the next day it tries again |
| How do I force one? | Actions -> Automatic build -> Run workflow (the `force` box is on by default) |
| How do I stop it? | disable the workflow in the Actions tab; nothing else depends on it |

The `.g3a` is only uploaded after `fxsdk build-cg` succeeds, so a red day never
replaces a working file.

It also keeps the 1.5 GB toolchain cache alive. GitHub discards a cache that has
not been used for seven days, and refilling it means about 45 minutes of
compiling binutils and GCC, so a build every few days is much cheaper than the
rebuild it prevents.

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
2. `toolchain`: builds fxSDK, `sh-elf-binutils`, `sh-elf-gcc`, OpenLibm, fxlibc
   and gint with GiteaPC — roughly 45 minutes — and caches the whole install
   tree (`~/.local`, `~/.giteapc`, key `fxsdk-sh3eb-elf-gcc14-v1`). This job is
   separate on purpose: a cache is only saved by a job that finishes green, so
   a compile error in the add-in must not be able to throw the tree away. After
   the first run it is a cache restore of about 80 seconds.
3. `g3a`: restores that tree, reads the version from the tag, runs
   `fxsdk build-cg`, builds the M1 spike add-in too if `spike/` is still there,
   uploads the files, force-pushes `Bench-<version>.g3a` to the `builds` branch,
   and on a `v*` tag creates the GitHub release with the notes from the matching
   `CHANGELOG.md` section.

A whole tagged release therefore takes about two and a half minutes once the
toolchain cache exists (measured: 78 s for the toolchain job, 68 s for the
build job), instead of the ~50 minutes the first one took.

Bump `TOOLCHAIN_CACHE_KEY` in the workflow when a new gint or GCC is wanted;
otherwise the cached one is reused forever.

## Where the .g3a ends up

- Release asset: `Bench-<version>.g3a` on the `v*` release.
- A copy is force-pushed to the `builds` branch, so
  `git fetch origin builds` gets it without touching release download hosts.
- The website fetches the release asset at deploy time
  (`site/scripts/fetch-addin.mjs`) and serves it from its own domain, so the
  Download button on the page never sends anyone to GitHub.

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

## What ends up on a release

Only `Bench-<version>.g3a`. The M1 spike add-in (`spike/`) is a diagnostic tool
built in the same run and kept in its own `bench-spike` artifact, but it is not
a release asset and not part of Bench: shipping it next to the add-in only
confuses the download.

```sh
gh run download --name bench-g3a   --dir dist   # Bench-<version>.g3a
gh run download --name bench-spike --dir dist   # Bench-spike-0.0.2.g3a
```

## Checking a release before you publish it

1. Copy `Bench-<version>.g3a` to the calculator in USB Flash mode.
2. Run the checklist in `docs/STATUS.md` → "What needs the calculator".
3. If something behaves oddly on the device, run the M1 spike (see
   `docs/spike.md` and its artifact) to read the facts about the hardware.
4. Only then publish the release.
