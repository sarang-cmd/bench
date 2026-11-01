# dist/ - the built add-in, in the repository

The `.g3a` is the whole product, and release assets and CI artifacts both live on
blob storage, which some networks cannot reach. A git remote is plain git, so the
build also lives here.

| | |
|---|---|
| File | `Bench-<version>.g3a`, plus `Bench-spike-<version>.g3a` |
| Version | from `version.txt` (the spike carries its own) |
| Built from | the commit in `BUILD.txt` |
| Refreshed by | CI, on every push to `main` and every tag, as a commit marked `[skip ci]` |

`BUILD.txt` records the commit, the size and the SHA-256, so a file found here can
be checked against the release without downloading it again. `SPIKE.txt` does the
same for the diagnostic add-in.

The M1 diagnostic add-in (`spike/`) sits next to it as
`Bench-spike-<version>.g3a`, described by `SPIKE.txt` instead of `BUILD.txt`.
It is a diagnostic, not Bench: it runs ten measurements and prints the numbers.
Both files are here so that a hardware session needs one clone and nothing from
blob storage; `docs/spike.md` says which is which.

## Which copy is which

| Where | What it is |
|---|---|
| `dist/` | a snapshot of the newest green build, with its checksum |
| `builds` branch | the same file, force-pushed on every build: `git fetch origin builds` |
| releases | `v<version>` is the released build; `auto` is rebuilt every few days |

If they disagree, the release is the one to trust: it carries the same bytes as
the `builds` branch at the time of the build.

## Installing

1. Copy `Bench-<version>.g3a` to the root of the calculator's storage, next to
   `@MainMem`, over USB Flash mode.
2. Eject, press MENU, open Bench.

No folder, no extraction, nothing else.
