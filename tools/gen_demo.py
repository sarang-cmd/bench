#!/usr/bin/env python3
"""Embed the demo notes, decks, study log and formula booklets in the add-in.

The .g3a is one file: a folder cannot travel with it.  So the text goes in as C
string literals, and the add-in writes any of it that is missing the first time
it runs.  That turns "copy demo/notes into Bench/notes" from three steps on a PC
into nothing at all - unplug, open Bench, and the demo is in the lists.

Writes src/apps/demo_data.c and src/apps/demo_data.h.  Both are generated and
checked by CI (`make demo-check`), because a hand edit here would be a copy of
the demo that silently stops matching demo/.

    python3 tools/gen_demo.py            rewrite the two files
    python3 tools/gen_demo.py --check    fail when they are stale (make demo-check)

The sources:

    demo/notes/*.md          ->  Bench/notes
    demo/cards/*.txt         ->  Bench/cards
    demo/study/log.txt       ->  Bench/study
    docs/samples/formulas/*.txt -> Bench/formulas

ASCII only.  The add-in draws with a font that has no glyph above 0x7E, and
`tools/md2bench.py` exists to strip the rest, so anything outside printable
ASCII is an error here rather than a `?` on the calculator.
"""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
OUT_C = ROOT / "src/apps/demo_data.c"
OUT_H = ROOT / "src/apps/demo_data.h"

# Total size the add-in is allowed to carry.  The .g3a is ~270 KB and storage
# memory is 16 MB; this is a guard against someone dropping a novel in demo/,
# not a hardware limit.
MAX_BYTES = 48 * 1024

# store_write() hands the card driver one 8 KiB buffer; leave room for the pad.
PART_LIMIT = 6144

SOURCES = [
    ("notes", "demo/notes", ".md"),
    ("notes", "demo/notes", ".txt"),
    ("cards", "demo/cards", ".txt"),
    ("study", "demo/study", ".txt"),
    ("formulas", "docs/samples/formulas", ".txt"),
]


def collect():
    entries = []
    seen = set()
    for folder, directory, ext in SOURCES:
        base = ROOT / directory
        if not base.is_dir():
            continue
        for path in sorted(base.glob("*" + ext)):
            if path.parent.name == "needs-human-proofreading":
                continue
            key = (folder, path.name)
            if key in seen:
                raise SystemExit("duplicate: %s/%s" % (folder, path.name))
            seen.add(key)
            raw = path.read_bytes()
            try:
                text = raw.decode("ascii")
            except UnicodeDecodeError as error:
                line = raw[: error.start].count(b"\n") + 1
                raise SystemExit(
                    "%s:%d: not ASCII - run tools/md2bench.py first (%s)"
                    % (path.relative_to(ROOT), line, error.reason)
                )
            for number, line in enumerate(text.split("\n"), 1):
                for char in line:
                    if not (0x20 <= ord(char) <= 0x7E):
                        raise SystemExit(
                            "%s:%d: character %r is not printable ASCII"
                            % (path.relative_to(ROOT), number, char)
                        )
            if text and not text.endswith("\n"):
                text += "\n"
            entries.append((folder, path.name, text, path.relative_to(ROOT)))
    if not entries:
        raise SystemExit("nothing to embed: demo/ and docs/samples/formulas are empty")
    for folder, name, text, _source in entries:
        if len(text) > PART_LIMIT:
            raise SystemExit(
                "%s/%s is %d bytes; the add-in writes at most %d per file"
                % (folder, name, len(text), PART_LIMIT)
            )
    total = sum(len(e[2]) for e in entries)
    if total > MAX_BYTES:
        raise SystemExit(
            "the demo is %d bytes, over the %d budget" % (total, MAX_BYTES)
        )
    return entries


def c_literal(text):
    """One C literal, wrapped on spaces so the generated file reads like prose.

    Every chunk keeps its trailing space, so the concatenation is exact: a
    chunk that ended at a space contains that space.  Chunks that had to be cut
    mid-word (a long formula) do not gain one.
    """
    escaped = []
    for char in text:
        if char == "\n":
            escaped.append("\\n")
        elif char in '"\\':
            escaped.append("\\" + char)
        else:
            escaped.append(char)

    lines, line = [], ""
    for piece in escaped:
        line += piece
        if len(line) >= 92 and piece == " ":
            lines.append(line)
            line = ""
        elif len(line) >= 108:
            lines.append(line)
            line = ""
    if line:
        lines.append(line)
    return '"' + '"\n    "'.join(lines) + '"'


PART_MAX = 3000


def parts(text):
    """Cut a file into chunks of at most PART_MAX bytes, on line boundaries.

    ISO C only guarantees that a compiler handles a 4095-byte string literal,
    and gcc -pedantic rejects anything longer, so each file is a list of
    literals that the add-in joins back into one buffer.  Splitting on newlines
    keeps the generated source readable.
    """
    out, current = [], ""
    for line in text.splitlines(keepends=True):
        if current and len(current) + len(line) > PART_MAX:
            out.append(current)
            current = ""
        current += line
        # A single line longer than PART_MAX (a long formula) is cut here.
        while len(current) > PART_MAX:
            out.append(current[:PART_MAX])
            current = current[PART_MAX:]
    if current:
        out.append(current)
    return out or [""]


def render_c(entries):
    body = []
    body.append("/* demo_data.c - GENERATED by tools/gen_demo.py.  Do not edit.")
    body.append(" *")
    body.append(" * The demo notes, decks, study log and formula booklets, embedded so")
    body.append(" * the add-in can write them to the card on first run.  %d files,"
                % len(entries))
    body.append(" * %d bytes of text." % sum(len(e[2]) for e in entries))
    body.append(" */")
    body.append("")
    body.append('#include "demo_data.h"')
    body.append("")
    for index, (folder, name, text, source) in enumerate(entries):
        chunks = parts(text)
        body.append("/* %s (%d bytes, %d part%s) */"
                    % (source, len(text), len(chunks),
                       "" if len(chunks) == 1 else "s"))
        body.append("static char const *const file_%d[] = {" % index)
        for chunk in chunks:
            body.append("    %s," % c_literal(chunk))
        body.append("    NULL,")
        body.append("};")
        body.append("")
    body.append("demo_file_t const demo_files[] = {")
    for index, (folder, name, text, source) in enumerate(entries):
        body.append(
            '    { "%s", "%s", file_%d, %d },' % (folder, name, index, len(text))
        )
    body.append("};")
    body.append("")
    body.append("int const demo_file_count = %d;" % len(entries))
    body.append("")
    return "\n".join(body)


HEADER = '''/* demo_data.h - GENERATED by tools/gen_demo.py.  Do not edit.
 *
 * One entry per embedded file.  `folder` is relative to \\\\fls0\\Bench.  `parts`
 * holds the text in adjacent chunks, NULL-terminated: ISO C only promises a
 * compiler handles 4095-byte literals, and one booklet is longer than that, so
 * the add-in joins the chunks into a buffer before writing.  `len` is the byte
 * count of the joined text.
 */

#ifndef BENCH_DEMO_DATA_H
#define BENCH_DEMO_DATA_H

#include <stddef.h>   /* NULL */

typedef struct {
    char const *folder;
    char const *name;
    char const *const *parts; /* NULL-terminated */
    int len;
} demo_file_t;

extern demo_file_t const demo_files[];
extern int const demo_file_count;

#endif /* BENCH_DEMO_DATA_H */
'''


def main(argv):
    entries = collect()
    c_text = render_c(entries)
    h_text = HEADER
    check = "--check" in argv
    stale = []

    for path, text in ((OUT_C, c_text), (OUT_H, h_text)):
        current = path.read_text() if path.exists() else None
        if check:
            if current != text:
                stale.append(path.relative_to(ROOT))
        else:
            path.write_text(text)

    if check:
        if stale:
            print(
                "GENERATED FILE STALE: %s\n"
                "  the demo changed without a regeneration - run make demo-gen "
                "and commit the result" % ", ".join(str(p) for p in stale)
            )
            return 1
        print("demo data OK (%d files, %d bytes)" % (len(entries),
                                                     sum(len(e[2]) for e in entries)))
        return 0

    print(
        "demo data: %d files, %d bytes -> %s"
        % (len(entries), sum(len(e[2]) for e in entries), OUT_C.relative_to(ROOT))
    )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
