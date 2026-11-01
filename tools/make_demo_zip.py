#!/usr/bin/env python3
"""Pack the demo notes, decks, study log and formula booklets as one zip.

Bench 1.2.6 carries the same text inside the .g3a and writes it to the card on
first run, so this download is not required.  It is here for the two cases the
in-app copy cannot serve:

  * a card that is full, or the second card someone keeps their own work on
  * editing the demo on a PC before it ever reaches the calculator - the files
    in the zip are the same bytes the add-in writes, so an edit here is an edit
    there

Layout: the archive holds a single `Bench` folder, so extracting it into the
drive root (`D:\\` in USB Flash mode) gives `D:\\Bench\\notes`, `D:\\Bench\\cards`,
`D:\\Bench\\study`, `D:\\Bench\\formulas` - exactly where the add-in looks, and
exactly the folders it creates.  Do not extract it into `@MainMem`: the
calculator moves files it does not keep there into `SAVE-F\\Bench`, and then the
lists look empty (manual 13-6).

    python3 tools/make_demo_zip.py             write dist/Bench-demo-<version>.zip
    python3 tools/make_demo_zip.py --check     fail if the committed zip is stale

The zip is reproducible: entries are sorted, timestamps are fixed and the
permissions are constant, so the same sources give the same bytes and a diff in
`dist/` means the demo really changed.
"""

from pathlib import Path
import sys
import zipfile

ROOT = Path(__file__).resolve().parent.parent
DIST = ROOT / "dist"

# (folder in Bench/, directory in the repository, extension)
SOURCES = [
    ("notes", "demo/notes", ".md"),
    ("notes", "demo/notes", ".txt"),
    ("cards", "demo/cards", ".txt"),
    ("study", "demo/study", ".txt"),
    ("formulas", "docs/samples/formulas", ".txt"),
]

# 1980-01-01 00:00:00, the earliest a zip can store, and a constant run to run.
FIXED_TIME = (1980, 1, 1, 0, 0, 0)


def version():
    return (ROOT / "version.txt").read_text(encoding="utf-8").strip()


def out_path():
    return DIST / ("Bench-demo-%s.zip" % version())


def collect():
    """Every file that goes in, as (name inside the zip, bytes)."""
    entries, seen = {}, set()
    for folder, directory, ext in SOURCES:
        base = ROOT / directory
        if not base.is_dir():
            continue
        for path in sorted(base.glob("*" + ext)):
            # Proofreading-pending text does not ship, in the add-in or here.
            if "needs-human-proofreading" in path.parts:
                continue
            name = "Bench/%s/%s" % (folder, path.name)
            if name in seen:
                raise SystemExit("duplicate entry: %s" % name)
            seen.add(name)
            data = path.read_bytes()
            try:
                data.decode("ascii")
            except UnicodeDecodeError as error:
                raise SystemExit(
                    "%s: not ASCII - run tools/md2bench.py first (%s)"
                    % (path.relative_to(ROOT), error.reason)
                )
            entries[name] = data
    if not entries:
        raise SystemExit("nothing to pack: demo/ and docs/samples/formulas are empty")
    return entries


def build(entries):
    """The zip as bytes, written to an in-memory buffer so --check can compare."""
    import io

    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w", zipfile.ZIP_DEFLATED) as archive:
        for name in sorted(entries):
            info = zipfile.ZipInfo(name, date_time=FIXED_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            archive.writestr(info, entries[name])
    return buffer.getvalue()


def main(argv):
    entries = collect()
    data = build(entries)
    path = out_path()

    if "--check" in argv:
        if not path.exists():
            print("MISSING: %s - run make demo-zip" % path.relative_to(ROOT))
            return 1
        if path.read_bytes() != data:
            print(
                "STALE: %s does not match demo/ - run make demo-zip and commit it"
                % path.relative_to(ROOT)
            )
            return 1
        print("demo zip OK (%d files, %d bytes)" % (len(entries), len(data)))
        return 0

    DIST.mkdir(exist_ok=True)
    path.write_bytes(data)
    print(
        "demo zip: %d files, %d bytes -> %s"
        % (len(entries), len(data), path.relative_to(ROOT))
    )
    for name in sorted(entries):
        print("  %s (%d B)" % (name, len(entries[name])))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
