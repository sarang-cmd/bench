#!/usr/bin/env python3
"""Rewrite the README's generated blocks: the badge line and the stats table.

    python3 tools/gen_readme_stats.py          # rewrite in place
    python3 tools/gen_readme_stats.py --check  # exit 1 if the file is stale

Every number is read from the repository, so the README cannot claim a tool the
add-in does not have or a test program that was deleted.  `make stats` runs it
and CI fails when the result would change.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

BADGES_BEGIN = "<!-- BADGES:BEGIN -->"
BADGES_END = "<!-- BADGES:END -->"
STATS_BEGIN = "<!-- STATS:BEGIN -->"
STATS_END = "<!-- STATS:END -->"


def counts():
    """Everything the two blocks show."""
    import json

    tools = json.loads((ROOT / "docs" / "tools.json").read_text())
    tool_rows = [t for t in tools if t["id"] not in ("settings", "about")]
    version = (ROOT / "version.txt").read_text().strip()

    tests = sorted(p.stem for p in (ROOT / "tests").glob("test_*.c"))
    tour = sorted((ROOT / "tools" / "sim" / "tour").glob("*.txt"))
    shots = sorted(p for p in (ROOT / "docs" / "images").glob("*.png")
                   if not p.name.startswith("brand-") and not p.name.startswith("og"))

    lines = 0
    files = 0
    for path in sorted((ROOT / "src").rglob("*")):
        if path.suffix in (".c", ".h"):
            files += 1
            lines += len(path.read_text().splitlines())

    return {
        "version": version,
        "tools": len(tool_rows),
        "categories": len({t["category"] for t in tool_rows}),
        "shots": len(shots),
        "tour": len(tour),
        "tests": len(tests),
        "lines": lines,
        "files": files,
    }


def badges(c):
    # The releases page, not the tag: the version in the tree is bumped by the
    # change that earned it, so between a bump and a release there is no tag to
    # link to, and a badge pointing at a 404 is worse than a badge pointing at
    # the page where the tag will be.
    release = "https://github.com/sarang-cmd/bench/releases"
    return "\n".join([
        BADGES_BEGIN,
        f"[![version](https://img.shields.io/badge/version-{c['version']}-2f2d2e)]"
        f"({release})",
        "[![build](https://github.com/sarang-cmd/bench/actions/workflows/build.yml/badge.svg)]"
        "(https://github.com/sarang-cmd/bench/actions/workflows/build.yml)",
        "[![licence](https://img.shields.io/badge/licence-MIT-2f5fd0)]"
        "(https://github.com/sarang-cmd/bench/blob/main/LICENSE)",
        # Live badges: the numbers come from the site's shields endpoint files,
        # which gen-stats.mjs writes from the repository, so a stale badge is
        # impossible rather than merely unlikely.
        "[![tools](https://img.shields.io/endpoint?url=https%3A%2F%2Fbench-gdc.vercel.app"
        "%2Fbadge-tools.json)](docs/tools.json)",
        "[![screens](https://img.shields.io/endpoint?url=https%3A%2F%2Fbench-gdc.vercel.app"
        "%2Fbadge-screens.json)](docs/images)",
        BADGES_END,
    ])


def stats(c):
    return "\n".join([
        STATS_BEGIN,
        "| | |",
        "|---|---|",
        f"| Tools | {c['tools']} in {c['categories']} categories |",
        f"| Screens captured | {c['shots']} |",
        f"| Tour scripts | {c['tour']} |",
        f"| Native test programs | {c['tests']} |",
        f"| C source | {c['lines']:,} lines in {c['files']} files |",
        f"| Version | {c['version']} |",
        STATS_END,
    ])


def replace_block(text, begin, end, block, path):
    start = text.find(begin)
    stop = text.find(end)
    if start < 0 or stop < 0:
        raise SystemExit(f"{path}: missing {begin} / {end}")
    return text[:start] + block + text[stop + len(end):]


def main(argv):
    check = "--check" in argv
    path = ROOT / "README.md"
    c = counts()

    text = path.read_text()
    text = replace_block(text, BADGES_BEGIN, BADGES_END, badges(c), path)
    text = replace_block(text, STATS_BEGIN, STATS_END, stats(c), path)

    old = path.read_text()
    if text == old:
        print(f"README is up to date ({c['tools']} tools, {c['shots']} screens)")
        return 0
    if check:
        print("README is stale: run python3 tools/gen_readme_stats.py")
        return 1
    path.write_text(text)
    print(f"README updated: {c['tools']} tools, {c['shots']} screens, "
          f"{c['tests']} test programs")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
