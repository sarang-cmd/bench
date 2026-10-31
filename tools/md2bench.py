#!/usr/bin/env python3
"""md2bench - clean a Markdown note so it renders on the Bench calculator.

Spec: docs/plan.md section 6.4, "tools/md2bench.py tasks":

  * replace smart quotes, dashes and ellipses with ASCII
  * replace unsupported symbols from a mapping table
  * strip front matter, HTML tags and image lines
  * report every change and every unsupported block
  * write the output into a folder ready to copy to the calculator

The viewer itself only draws the subset in md_parse.c; this tool deals with the
characters the calculator font may not have.  It never changes the note's
meaning: everything it rewrites is listed in the report.

Usage:
    python3 tools/md2bench.py note.md                 # writes build/notes/note.md
    python3 tools/md2bench.py note.md -o out/         # choose the folder
    python3 tools/md2bench.py note.md --check         # report only, exit 1 if changed
    python3 tools/md2bench.py note.md --stdout        # cleaned text on stdout

Standard library only, no dependency.
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path

# ---------------------------------------------------------------- character map

# Typography: smart quotes, dashes and ellipses (the plan's first bullet).
TYPOGRAPHY_MAP = {
    "\u2018": "'",      # left single quote
    "\u2019": "'",      # right single quote
    "\u201a": "'",      # single low quote
    "\u201c": '"',      # left double quote
    "\u201d": '"',      # right double quote
    "\u201e": '"',      # double low quote
    "\u2013": "-",      # en dash
    "\u2014": "--",     # em dash
    "\u2015": "--",     # horizontal bar
    "\u2026": "...",    # ellipsis
    "\u00a0": " ",      # non-breaking space
    "\u2022": "*",      # bullet
    "\u2023": "*",      # triangular bullet
    "\u00ab": '"',      # guillemets
    "\u00bb": '"',
}

# Maths and symbols (the plan's second bullet).  ASCII names are used because
# the calculator font coverage is only known after the M1 spike.
SYMBOL_MAP = {
    "\u00b1": "+/-",    # plus-minus
    "\u00d7": "x",      # times
    "\u00f7": "/",      # divide
    "\u2212": "-",      # minus sign
    "\u00b0": "deg",    # degree
    "\u00b5": "u",      # micro
    "\u03bc": "u",      # greek mu
    "\u2248": "~",      # almost equal
    "\u2261": "=",      # identical
    "\u2260": "!=",     # not equal
    "\u2264": "<=",     # less or equal
    "\u2265": ">=",     # greater or equal
    "\u221a": "sqrt",   # square root
    "\u221e": "inf",    # infinity
    "\u03c0": "pi",     # pi
    "\u03a9": "ohm",    # omega
    "\u03b1": "alpha",
    "\u03b2": "beta",
    "\u03b3": "gamma",
    "\u03b4": "delta",
    "\u03b5": "epsilon",
    "\u03b8": "theta",
    "\u03bb": "lambda",
    "\u03c1": "rho",
    "\u03c3": "sigma",
    "\u03c4": "tau",
    "\u03c6": "phi",
    "\u0394": "delta",
    "\u2211": "sum",
    "\u222b": "int",
    "\u00b2": "^2",     # superscript two
    "\u00b3": "^3",     # superscript three
    "\u00b9": "^1",
    "\u00bd": "1/2",
    "\u00bc": "1/4",
    "\u00be": "3/4",
    "\u2192": "->",
    "\u2190": "<-",
    "\u2191": "^",
    "\u2193": "v",
    "\u21d2": "=>",
}

# Kept as is: tab, newline and plain printable ASCII.
_SAFE_ASCII = set(chr(c) for c in range(0x20, 0x7F)) | {"\n", "\t"}

HTML_TAG_RE = re.compile(r"<[A-Za-z/!][^>\n]*>")
IMAGE_LINE_RE = re.compile(r"^\s*!\[(?P<alt>[^\]]*)\]\([^)]*\)\s*$")
INLINE_IMAGE_RE = re.compile(r"!\[(?P<alt>[^\]]*)\]\([^)]*\)")
TABLE_LINE_RE = re.compile(r"^\s*\|")
FOOTNOTE_RE = re.compile(r"^\s*\[\^[^\]]+\]:")
FRONT_MATTER_RE = re.compile(r"\A---\r?\n.*?\r?\n---\r?\n", re.DOTALL)


@dataclass
class Report:
    """What the tool changed, for the console and for the tests."""

    replacements: Counter = field(default_factory=Counter)
    removed_lines: list = field(default_factory=list)
    unsupported_blocks: list = field(default_factory=list)   # (line, kind)
    unsupported_chars: Counter = field(default_factory=Counter)
    html_tags: int = 0
    images: int = 0
    front_matter_lines: int = 0

    def is_clean(self) -> bool:
        return not (self.replacements or self.removed_lines
                    or self.unsupported_blocks or self.unsupported_chars
                    or self.html_tags or self.images or self.front_matter_lines)

    def lines(self) -> list:
        out = []
        if self.front_matter_lines:
            out.append(f"  front matter: {self.front_matter_lines} line(s) removed")
        if self.html_tags:
            out.append(f"  HTML tags: {self.html_tags} removed")
        if self.images:
            out.append(f"  images: {self.images} removed (alt text kept for "
                       f"inline images)")
        if self.removed_lines:
            shown = ", ".join(str(n) for n in self.removed_lines[:10])
            more = "" if len(self.removed_lines) <= 10 else f" (+{len(self.removed_lines) - 10} more)"
            out.append(f"  image lines removed: {shown}{more}")
        if self.replacements:
            parts = []
            for ch, count in sorted(self.replacements.items()):
                replacement = TYPOGRAPHY_MAP.get(ch, SYMBOL_MAP.get(ch, "?"))
                parts.append(f"{ch!r}->{replacement!r} x{count}")
            out.append("  symbols replaced: " + ", ".join(parts))
        if self.unsupported_chars:
            pairs = ", ".join(f"{ch!r} x{n}"
                              for ch, n in self.unsupported_chars.most_common())
            out.append(f"  characters with no mapping (left in place, may show "
                       f"as '?'): {pairs}")
        if self.unsupported_blocks:
            shown = ", ".join(f"line {n} {kind}"
                              for n, kind in self.unsupported_blocks[:10])
            more = "" if len(self.unsupported_blocks) <= 10 else \
                f" (+{len(self.unsupported_blocks) - 10} more)"
            out.append(f"  unsupported blocks (shown as plain text): {shown}{more}")
        return out


def clean(text: str) -> tuple:
    """Return (cleaned_text, Report).

    The input text is treated as UTF-8 already decoded.  Every change is
    recorded; nothing is dropped without a report entry.
    """
    report = Report()
    text = text.replace("\r\n", "\n").replace("\r", "\n")

    # 1. Front matter: a --- block at the very start of the file.
    match = FRONT_MATTER_RE.match(text)
    if match:
        report.front_matter_lines = match.group(0).count("\n")
        text = text[match.end():]

    lines = text.split("\n")
    out_lines = []

    for number, line in enumerate(lines, start=1):
        # 2. An image on its own line disappears completely.
        if IMAGE_LINE_RE.match(line):
            report.removed_lines.append(number)
            report.images += 1
            continue

        # 3. HTML tags go, their text content stays.
        def _drop_tag(m: re.Match) -> str:
            report.html_tags += 1
            return ""
        line = HTML_TAG_RE.sub(_drop_tag, line)

        # 4. An inline image becomes its alt text.
        def _inline_image(m: re.Match) -> str:
            report.images += 1
            return m.group("alt")
        line = INLINE_IMAGE_RE.sub(_inline_image, line)

        # 5. Blocks the viewer shows as plain text get a report entry.
        if TABLE_LINE_RE.match(line):
            report.unsupported_blocks.append((number, "table"))
        elif FOOTNOTE_RE.match(line):
            report.unsupported_blocks.append((number, "footnote"))

        # 6. Character replacements.
        rebuilt = []
        for ch in line:
            if ch in _SAFE_ASCII:
                rebuilt.append(ch)
                continue
            replacement = TYPOGRAPHY_MAP.get(ch) or SYMBOL_MAP.get(ch)
            if replacement is not None:
                report.replacements[ch] += 1
                rebuilt.append(replacement)
            else:
                report.unsupported_chars[ch] += 1
                rebuilt.append(ch)
        out_lines.append("".join(rebuilt))

    cleaned = "\n".join(out_lines)

    # Collapse runs of blank lines left behind by removed lines.
    cleaned = re.sub(r"\n{3,}", "\n\n", cleaned)
    if cleaned and not cleaned.endswith("\n"):
        cleaned += "\n"
    return cleaned, report


def convert_file(source: Path, out_dir: Path) -> tuple:
    """Convert one file into out_dir and return (path, Report)."""
    text = source.read_text(encoding="utf-8", errors="replace")
    cleaned, report = clean(text)
    out_dir.mkdir(parents=True, exist_ok=True)
    target = out_dir / source.name
    target.write_text(cleaned, encoding="utf-8", newline="\n")
    return target, report


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="Prepare Markdown for the Bench calculator.")
    parser.add_argument("source", type=Path, help="Markdown file to clean")
    parser.add_argument("-o", "--out", type=Path, default=Path("build/notes"),
                        help="output folder (default: build/notes)")
    parser.add_argument("--stdout", action="store_true",
                        help="print the cleaned text instead of writing a file")
    parser.add_argument("--check", action="store_true",
                        help="report only; exit 1 when the file needs changes")
    parser.add_argument("-q", "--quiet", action="store_true",
                        help="print nothing but errors")
    args = parser.parse_args(argv)

    if not args.source.is_file():
        print(f"md2bench: {args.source}: not a file", file=sys.stderr)
        return 2

    text = args.source.read_text(encoding="utf-8", errors="replace")
    cleaned, report = clean(text)

    if args.stdout:
        sys.stdout.write(cleaned)
    elif not args.check:
        args.out.mkdir(parents=True, exist_ok=True)
        target = args.out / args.source.name
        target.write_text(cleaned, encoding="utf-8", newline="\n")
        if not args.quiet:
            print(f"{args.source} -> {target}")

    if not args.quiet:
        if report.is_clean():
            print("  no changes needed")
        else:
            for line in report.lines():
                print(line)
        if args.check and not report.is_clean():
            print("  file needs cleaning")

    if args.check and not report.is_clean():
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
