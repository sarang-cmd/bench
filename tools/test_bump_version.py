"""Tests for bump_version.py.  Run with: make py-test"""

import shutil
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import bump_version  # noqa: E402


class TestBump(unittest.TestCase):
    def test_each_part(self):
        self.assertEqual(bump_version.bumped("1.2.0", "patch"), "1.2.1")
        self.assertEqual(bump_version.bumped("1.2.0", "minor"), "1.3.0")
        self.assertEqual(bump_version.bumped("1.2.0", "major"), "2.0.0")

    def test_patch_then_minor_from_a_full_version(self):
        self.assertEqual(bump_version.bumped("1.9.9", "patch"), "1.9.10")
        self.assertEqual(bump_version.bumped("1.9.9", "minor"), "1.10.0")

    def test_unknown_part_is_refused(self):
        with self.assertRaises(SystemExit):
            bump_version.bumped("1.2.0", "sideways")

    def test_bad_version_is_refused(self):
        for bad in ("1.2", "v1.2.0", "1.2.0-rc1", "", "one.two.three"):
            with self.assertRaises(SystemExit):
                bump_version.parse(bad)


class TestTree(unittest.TestCase):
    """The real tree, not a fixture: the check exists to catch the tree drifting."""

    def test_repository_is_consistent(self):
        self.assertEqual(bump_version.problems(), [])

    def test_version_matches_the_readme_badge(self):
        version = bump_version.read_version()
        readme = (bump_version.ROOT / "README.md").read_text()
        self.assertIn(f"version-{version}-", readme)

    def test_changelog_has_a_section_for_it(self):
        version = bump_version.read_version()
        changelog = (bump_version.ROOT / "CHANGELOG.md").read_text()
        self.assertIn(f"## {version}", changelog)


class TestWrite(unittest.TestCase):
    """write_version() on a copy of the files, so the tests never edit the repo."""

    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp())
        (self.tmp / "src").mkdir()
        self._saved = (
            bump_version.VERSION_FILE,
            bump_version.HEADER,
            bump_version.CMAKE,
        )
        bump_version.VERSION_FILE = self.tmp / "version.txt"
        bump_version.HEADER = self.tmp / "src" / "version.h"
        bump_version.CMAKE = self.tmp / "CMakeLists.txt"
        bump_version.VERSION_FILE.write_text("1.2.0\n")
        bump_version.HEADER.write_text(
            '/* a header */\n#ifndef BENCH_VERSION\n'
            '#define BENCH_VERSION "1.2.0"   /* keep in step with version.txt */\n'
            "#endif\n"
        )
        bump_version.CMAKE.write_text(
            "#   fxsdk build-cg                      -> Bench-1.2.0.g3a\n"
            "#   fxsdk build-cg -DBENCH_VERSION=0.0.3\n"
        )

    def tearDown(self):
        (
            bump_version.VERSION_FILE,
            bump_version.HEADER,
            bump_version.CMAKE,
        ) = self._saved
        shutil.rmtree(self.tmp)

    def test_all_three_files_are_rewritten(self):
        touched = bump_version.write_version("1.3.0")
        self.assertEqual(
            touched, ["version.txt", "src/version.h", "CMakeLists.txt"]
        )
        self.assertEqual(bump_version.VERSION_FILE.read_text().strip(), "1.3.0")
        self.assertIn('BENCH_VERSION "1.3.0"', bump_version.HEADER.read_text())
        self.assertIn("Bench-1.3.0.g3a", bump_version.CMAKE.read_text())

    def test_the_override_example_is_not_touched(self):
        bump_version.write_version("1.3.0")
        self.assertIn("-DBENCH_VERSION=0.0.3", bump_version.CMAKE.read_text())

    def test_rewriting_the_same_version_changes_nothing(self):
        self.assertEqual(bump_version.write_version("1.2.0"), [])

    def test_a_bad_version_never_reaches_a_file(self):
        with self.assertRaises(SystemExit):
            bump_version.write_version("1.3")
        self.assertEqual(bump_version.VERSION_FILE.read_text().strip(), "1.2.0")

    def test_a_drifting_header_is_reported(self):
        bump_version.HEADER.write_text('#define BENCH_VERSION "0.9.9"\n')
        bump_version.CMAKE.write_text(
            "#   fxsdk build-cg                      -> Bench-0.9.9.g3a\n"
        )
        found = bump_version.problems()
        self.assertTrue(any("src/version.h says 0.9.9" in item for item in found))
        self.assertTrue(any("CMakeLists.txt" in item for item in found))


if __name__ == "__main__":
    unittest.main()
