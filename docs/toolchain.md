# Toolchain

How Bench is built, what was verified where, and what the user has to do on
their own machine. Date: 31 October 2026.

**Status: no .g3a was built in this container.** It cannot reach the fxSDK's
download source and has no SuperH compiler. The reason and the evidence are in
section 5; the way that actually produces the file is the GitHub workflow in
`.github/workflows/build.yml` (section 7). Everything that does not need a
cross-compiler is done and tested here.

## 1. What the project needs

| Piece | Version | Source |
|-------|---------|--------|
| Casio fx-CG50 | OS 3.81 | user's calculator |
| fxSDK | 2.10.0 | `gitea.planet-casio.com/Lephenixnoir/fxsdk` |
| gint | 2.10.0 | `gitea.planet-casio.com/Lephenixnoir/gint` |
| fxlibc | current | `gitea.planet-casio.com/Vhex-Kernel-Core/fxlibc` |
| libm | current | installed by the fxSDK install |
| SuperH cross compiler | `sh-elf-*` | `gitea.planet-casio.com/Lephenixnoir/sh-elf-gcc` |
| CMake | >= 3.15 | distribution |
| Host C compiler | any C11 | only for the native tests |

The plan's fact table (docs/plan.md section 2) matches what the fxSDK README
says: CMake project, `fxsdk build-cg` produces the .g3a, fxSDK is officially
supported on Linux and through WSL Ubuntu on Windows, and `fxlink` does not
work inside WSL.

## 2. Install (Linux, or WSL Ubuntu on the user's Windows machine)

The fxSDK README gives three installation routes. The recommended one is
GiteaPC; the manual one is the long route. The README does not list the
individual `giteapc` sub-commands, so use the GiteaPC README that comes with
it; do not guess flags.

```bash
# 1. Distribution packages the fxSDK needs (Debian/Ubuntu names)
sudo apt install build-essential cmake git python3 pkg-config \
                 libgmp-dev libmpfr-dev libmpc-dev flex bison

# 2. GiteaPC, per its own README
git clone https://gitea.planet-casio.com/Lephenixnoir/GiteaPC
cd GiteaPC && ./install.sh          # check GiteaPC's README for the exact step

# 3. The SDK itself, in this order; each line is one GiteaPC package
#    (order from the fxSDK README: cross-compiler, libm, libc, kernel, user
#    libraries)
giteapc install Lephenixnoir/sh-elf-binutils
giteapc install Lephenixnoir/sh-elf-gcc
giteapc install Lephenixnoir/fxsdk
giteapc install Lephenixnoir/gint
giteapc install Vhex-Kernel-Core/fxlibc

# 4. Check
fxsdk path        # prints where the SDK and the compiler live
```

This is the same sequence `.github/workflows/build.yml` runs, with the
`noudisks2` variant of the fxSDK so no UDisks2 is needed on a runner.

If a package name differs on your mirror, use `giteapc list` and copy the
name; the repository names above come from the README's links.

## 3. Build Bench

```bash
cd /path/to/bench
fxsdk build-cg
```

`build-cg/` then contains the add-in. The output name comes from
`version.txt` and lands in the project root:

```
Bench-<version>.g3a        e.g. Bench-1.0.0.g3a
```

Bump `version.txt` when a version is tagged; the About screen shows the same
number. To build a different version without touching the file:

```bash
fxsdk build-cg -DBENCH_VERSION=1.0.1
```

The spike add-in builds on its own so it can be deleted after M1:

```bash
cd spike && fxsdk build-cg          # produces Bench-spike-0.0.2.g3a
```

`fxsdk send-cg` needs UDisks2 and does not work inside WSL (fxSDK README), so
copy the .g3a to the calculator by hand in USB Flash mode, as the plan already
says.

## 4. Native tests (no calculator, no fxSDK)

```bash
make test          # C core tests, gcc, -Werror
make py-test       # tools/md2bench.py unit tests
make test-all      # both
```

These run anywhere a C11 compiler and Python 3 exist, including this container.

There is also a build of the whole add-in that needs neither a calculator nor
the fxSDK, useful for checking layout and for the screenshots:

```bash
make sim        # compiles src/{platform,ui,apps} against tools/sim
make shots      # renders every screen into docs/images
```

### Optional: gint header check

Every platform file is type-checked against the real gint headers before it
reaches the calculator build. This catches a misspelled or invented gint call
without needing the cross-compiler (it does not check the SuperH ABI):

```bash
make gint-check GINT_INC=/path/to/gint/include
```

## 5. What blocked the M0 and M1 builds here

Evidence gathered in this container:

| Check | Result |
|-------|--------|
| `gitea.planet-casio.com` (fxSDK, gint, GiteaPC, the cross compiler) | unreachable: the sandbox allows only github.com, codeload.github.com, api.github.com, registry.npmjs.org, pypi.org, files.pythonhosted.org |
| `apt` / `deb.debian.org` | no network, and no `sh-elf-gcc` package in Debian anyway |
| Prebuilt SuperH toolchain on GitHub | none found; the SDK repos that do exist on GitHub are mirrors of the fxSDK *tools* (fxconv/fxgxa/fxlink, Python) and of gint's source, not of `sh-elf-gcc` |
| Building GCC from source | would need binutils + gcc + gmp + mpfr + mpc + flex + bison sources; only GitHub is reachable, and the GCC mirror is a 4 GB git tree, i.e. not a practical base here |
| `cmake` | not installed, no apt access (a PyPI `cmake` wheel is available, so this alone would not block) |

Consequently: no `fxsdk`, no `sh-elf-gcc`, no `.g3a`. **No placeholder or
hand-made .g3a was produced** — inventing an add-in binary would be worse than
reporting the gap.

What was done instead:

* `docs/toolchain.md` (this file) records every install step.
* `CMakeLists.txt` and `spike/CMakeLists.txt` are written from the real fxSDK
  2.10.0 project template (`assets/CMakeLists.txt`), including the correct
  `generate_g3a` arguments and icon handling.
* `src/main.c` (M0) and `spike/main.c` (M1) were type-checked with the host
  compiler against the real gint 2.10.0 headers, with `-Wall -Wextra`, and are
  clean. Every gint call used is quoted with its header in `docs/spike.md`.
* gint 2.10.0's own sources were read for the facts that matter to the next
  milestone (arena names `_uram` and `_ostk`, `kmalloc_max`, the BFile world
  switch requirement, `rtc_ticks()` at 128 Hz, `rtc_periodic_enable(1, ...)`
  for a 1 Hz interrupt).

### Two ways forward — the user picks one

1. **Build in WSL (or any Linux box) and hand the .g3a back.** Slowest per
   iteration, no new accounts, and it is exactly the workflow AGENTS.md
   describes.
2. **Give this sandbox access to the fxSDK sources** (allow
   `gitea.planet-casio.com`, or upload the fxSDK, gint, fxlibc and
   `sh-elf-gcc` build trees as a tarball). Then M0 and the spike can be built
   here and the g3a sizes reported without involving the user's machine.

A third route, and the one this repository now takes, is the CI workflow in
section 7: a GitHub runner installs the toolchain and builds the file. Until the
first successful run of it, every "build passes" line in the milestone list
means "native tests pass and the sources type-check against gint; the
calculator build has not been run".

## 6. Versions used for the header checks

| Item | Value |
|------|-------|
| gint headers | 2.10.0 (`project(Gint VERSION 2.10.0)`) |
| fxSDK project template | 2.10.0 |
| Host compiler used for the checks | gcc 12.2.0 (Debian 12) |
| Python for `tools/` | 3.11 |

## 7. Where the .g3a comes from

`.github/workflows/build.yml` has two jobs:

| Job | What | When |
|-----|------|------|
| `native-tests` | `make test`, `make py-test`, `make sim`, `make shots` | every push and pull request |
| `g3a` | installs fxSDK + `sh-elf-gcc` + fxlibc + gint with GiteaPC, runs `fxsdk build-cg`, uploads `Bench-<version>.g3a`; on a `v*` tag it also attaches the file to the GitHub release | after `native-tests` passes |

A GitHub runner has the network access the sandbox lacks. To cut a release:

```bash
echo 1.0.1 > version.txt          # bump
git commit -am "1.0.1"
git tag v1.0.1
git push origin main --tags       # the workflow builds and publishes
```

The workflow's install steps follow the pattern used by other gint projects;
they have not been run from this repository yet, so the first tag is also the
first test of the pipeline.

