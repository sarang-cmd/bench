#!/usr/bin/env python3
"""Bump the add-in version, in every place it is written down.

    python3 tools/bump_version.py patch     # 1.2.0 -> 1.2.1, for a fix
    python3 tools/bump_version.py minor     # 1.2.0 -> 1.3.0, for a feature
    python3 tools/bump_version.py major     # 1.2.0 -> 2.0.0
    python3 tools/bump_version.py --set 1.4.2
    python3 tools/bump_version.py --check   # exit 1 if the tree disagrees

Rules (docs/VERSIONING.md): every change bumps the version before it is
committed.  A feature is a minor bump, a fix is a patch bump, a change to the
storage layout or to the meaning of an existing key is major.

What it edits:

    version.txt            the one true value
    src/version.h          the fallback used when CMake does not pass one
    CMakeLists.txt         the example file name in the header comment

Everything else - the README badges and stats, the site's generated data, the
changelog's heading - is generated from version.txt, so `make docs` rewrites it
and `--check` asserts it is right rather than trusting it.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VERSION_FILE = ROOT / "version.txt"
HEADER = ROOT / "src" / "version.h"
CMAKE = ROOT / "CMakeLists.txt"

SEMVER = re.compile(r"^(\d+)\.(\d+)\.(\d+)$")


def read_version() -> str:
    return VERSION_FILE.read_text().strip()


def parse(version: str) -> tuple[int, int, int]:
    match = SEMVER.match(version)
    if not match:
        raise SystemExit(f"version.txt holds {version!r}, which is not x.y.z")
    return tuple(int(part) for part in match.groups())  # type: ignore[return-value]


def bumped(version: str, part: str) -> str:
    major, minor, patch = parse(version)
    if part == "major":
        return f"{major + 1}.0.0"
    if part == "minor":
        return f"{major}.{minor + 1}.0"
    if part == "patch":
        return f"{major}.{minor}.{patch + 1}"
    raise SystemExit(f"unknown part {part!r}: use major, minor or patch")


def write_version(new: str) -> list[str]:
    """Write the new value everywhere it is literal.  Returns the files touched."""
    parse(new)  # a bad value must never reach a file
    touched = []

    if VERSION_FILE.read_text().strip() != new:
        VERSION_FILE.write_text(new + "\n")
        touched.append("version.txt")

    header = HEADER.read_text()
    new_header = re.sub(
        r'(#define BENCH_VERSION ")[0-9]+\.[0-9]+\.[0-9]+(")',
        lambda m: m.group(1) + new + m.group(2),
        header,
    )
    if new_header != header:
        HEADER.write_text(new_header)
        touched.append("src/version.h")

    cmake = CMAKE.read_text()
    new_cmake = re.sub(
        r"(#   fxsdk build-cg                      -> Bench-)[0-9]+\.[0-9]+\.[0-9]+(\.g3a)",
        lambda m: m.group(1) + new + m.group(2),
        cmake,
        count=1,
    )
    if new_cmake != cmake:
        CMAKE.write_text(new_cmake)
        touched.append("CMakeLists.txt")

    return touched


def problems() -> list[str]:
    """Every place that disagrees with version.txt, as sentences."""
    found = []
    try:
        version = read_version()
        parse(version)
    except SystemExit:
        return ["version.txt is not x.y.z"]

    header = HEADER.read_text()
    match = re.search(r'#define BENCH_VERSION "([0-9.]+)"', header)
    if not match:
        found.append("src/version.h has no BENCH_VERSION")
    elif match.group(1) != version:
        found.append(
            f"src/version.h says {match.group(1)}, version.txt says {version}"
        )

    cmake = CMAKE.read_text()
    if f"Bench-{version}.g3a" not in cmake:
        found.append(f"CMakeLists.txt's example file name is not Bench-{version}.g3a")

    changelog = (ROOT / "CHANGELOG.md").read_text()
    if not re.search(rf"^## {re.escape(version)}\b", changelog, re.M):
        found.append(
            f"CHANGELOG.md has no '## {version}' section - the changelog heading "
            "is the version, so the two cannot drift"
        )

    readme = (ROOT / "README.md").read_text()
    if f"version-{version}-" not in readme:
        found.append(
            f"README.md's version badge is not {version} - run "
            "python3 tools/gen_readme_stats.py"
        )
    return found


def main(argv: list[str]) -> int:
    if not argv or argv[0] in {"-h", "--help"}:
        print(__doc__.strip())
        return 0

    if argv[0] == "--check":
        found = problems()
        if found:
            print("VERSION CHECK FAILED")
            for item in found:
                print(f"  - {item}")
            return 1
        print(f"VERSION CHECK PASSED ({read_version()})")
        return 0

    if argv[0] == "--set":
        if len(argv) != 2:
            raise SystemExit("--set needs a version, e.g. --set 1.4.2")
        new = argv[1]
    elif argv[0] in {"major", "minor", "patch"}:
        new = bumped(read_version(), argv[0])
    else:
        raise SystemExit(f"unknown argument {argv[0]!r}; see --help")

    old = read_version()
    touched = write_version(new)
    print(f"version {old} -> {new}")
    for name in touched:
        print(f"  wrote {name}")
    print("next:")
    print("  add a '## %s - unreleased' section to CHANGELOG.md" % new)
    print("  python3 tools/gen_readme_stats.py && (cd site && npm run gen-stats)"
          " && npm run gen-changelog")
    print("  make version-check")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
