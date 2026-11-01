"""Tests for the bundled starter pack: gen_starterpack.py and make_starterpack_zip.py.

The starter pack exists twice on purpose - compiled into the .g3a, and as a zip - so
these tests check the pieces that make the two copies agree:

  * the chunking that keeps every string literal under ISO C's 4095 bytes
  * the escaping that has to round-trip byte for byte
  * the zip layout, which is what makes extracting onto D:\\ land in Bench/
  * both generated outputs being current, the way CI checks them

Run with: make py-test
"""

import io
import subprocess
import sys
import unittest
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))

import gen_starterpack  # noqa: E402
import make_starterpack_zip  # noqa: E402


class TestChunks(unittest.TestCase):
    def test_short_text_is_one_part(self):
        self.assertEqual(gen_starterpack.parts("abc\n"), ["abc\n"])

    def test_empty_text_still_has_a_part(self):
        self.assertEqual(gen_starterpack.parts(""), [""])

    def test_parts_never_exceed_the_limit(self):
        text = "".join("line %03d of a long note\n" % i for i in range(400))
        chunks = gen_starterpack.parts(text)
        self.assertGreater(len(chunks), 1)
        for chunk in chunks:
            self.assertLessEqual(len(chunk), gen_starterpack.PART_MAX)

    def test_parts_join_back_to_the_original(self):
        text = "".join("line %03d\n" % i for i in range(400))
        self.assertEqual("".join(gen_starterpack.parts(text)), text)

    def test_a_single_long_line_is_cut(self):
        text = "x" * (gen_starterpack.PART_MAX * 2 + 7)
        chunks = gen_starterpack.parts(text)
        self.assertEqual("".join(chunks), text)
        self.assertTrue(all(len(c) <= gen_starterpack.PART_MAX for c in chunks))

    def test_split_prefers_line_boundaries(self):
        text = ("a" * 40 + "\n") * 200
        chunks = gen_starterpack.parts(text)
        # Every chunk but possibly none should end on a newline: the rule cuts
        # between lines, never through one, when the lines are short enough.
        self.assertTrue(all(c.endswith("\n") for c in chunks))


class TestLiteral(unittest.TestCase):
    def test_quotes_and_backslashes_are_escaped(self):
        literal = gen_starterpack.c_literal('a "quote" and \\ a slash\n')
        self.assertIn('\\"', literal)
        self.assertIn("\\\\", literal)
        self.assertIn("\\n", literal)

    def test_no_raw_newline_reaches_the_source(self):
        self.assertNotIn("\n", gen_starterpack.c_literal("one\ntwo\n").strip('"'))

    def test_a_long_text_is_wrapped_into_many_literals(self):
        literal = gen_starterpack.c_literal("word " * 200)
        self.assertGreater(literal.count('"\n    "'), 1)


class TestZip(unittest.TestCase):
    def test_layout_is_one_bench_folder(self):
        entries = make_starterpack_zip.collect()
        for name in entries:
            self.assertTrue(name.startswith("Bench/"), name)
            parts = name.split("/")
            self.assertEqual(len(parts), 3, name)
            self.assertIn(parts[1], {"notes", "cards", "study", "formulas"})

    def test_every_folder_is_present(self):
        folders = {name.split("/")[1] for name in make_starterpack_zip.collect()}
        self.assertEqual(folders, {"notes", "cards", "study", "formulas"})

    def test_entries_are_plain_text(self):
        for name, data in make_starterpack_zip.collect().items():
            self.assertGreater(len(data), 0, name)
            text = data.decode("ascii")          # raises on anything else
            self.assertNotIn("\r", text, name)

    def test_the_same_sources_give_the_same_bytes(self):
        # Reproducible: fixed timestamps and sorted entries, so a diff in dist/
        # means the starter pack changed rather than that the clock did.
        first = make_starterpack_zip.build(make_starterpack_zip.collect())
        second = make_starterpack_zip.build(make_starterpack_zip.collect())
        self.assertEqual(first, second)

    def test_the_zip_is_what_an_unzip_produces(self):
        data = make_starterpack_zip.build(make_starterpack_zip.collect())
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            self.assertEqual(
                sorted(archive.namelist()),
                sorted(make_starterpack_zip.collect()),
            )
            self.assertIsNone(archive.testzip())

    def test_nothing_from_the_proofreading_folder_ships(self):
        for name in make_starterpack_zip.collect():
            self.assertNotIn("needs-human-proofreading", name)


class TestGeneratedFilesAreCurrent(unittest.TestCase):
    """The same checks CI runs, so a stale copy fails on a PC first."""

    def run_tool(self, *args):
        return subprocess.run(
            [sys.executable, *args],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )

    def test_embedded_starterpack_matches_its_sources(self):
        result = self.run_tool("tools/gen_starterpack.py", "--check")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_starterpack_zip_matches_its_sources(self):
        result = self.run_tool("tools/make_starterpack_zip.py", "--check")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_embedded_table_and_zip_list_the_same_files(self):
        source = (ROOT / "src" / "apps" / "starter_data.c").read_text()
        embedded = set()
        for line in source.splitlines():
            line = line.strip()
            if line.startswith('{ "') and line.endswith("},"):
                folder, name = line[3:].split('"')[0], line.split('"')[3]
                embedded.add("%s/%s/%s" % ("Bench", folder, name))
        self.assertEqual(embedded, set(make_starterpack_zip.collect()))

    def test_the_zip_in_dist_is_the_current_version(self):
        path = make_starterpack_zip.out_path()
        self.assertTrue(
            path.exists(),
            "run make starterpack-zip - dist/%s is missing" % path.name,
        )


if __name__ == "__main__":
    unittest.main()
