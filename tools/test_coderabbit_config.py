#!/usr/bin/env python3
"""Tests for tools/check_coderabbit.py.

Each case is a configuration with one thing wrong in it, because the failure
this file guards against is a setting that looks right and silently does
nothing: a misspelt key, a path rule for a directory that was renamed, a tool
for a language the repository does not contain.
"""

import tempfile
import unittest
from pathlib import Path

import check_coderabbit as cr

MINIMAL = """\
language: en-US
reviews:
  profile: chill
  path_filters:
    - "src/**"
  path_instructions:
    - path: "src/**"
      instructions: |
        Look at the C.
  tools:
    markdownlint:
      enabled: true
"""


def tree(*files):
    tmp = tempfile.mkdtemp()
    for name in files:
        path = Path(tmp) / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("x\n")
    return Path(tmp)


class ParseTest(unittest.TestCase):
    def test_booleans_are_not_strings(self):
        config = cr._lists(cr.parse("reviews:\n  poem: false\n  enabled: true\n"))
        self.assertIs(config["reviews"]["poem"], False)
        self.assertIs(config["reviews"]["enabled"], True)

    def test_list_of_maps_keeps_its_instructions(self):
        config = cr._lists(
            cr.parse(
                "reviews:\n"
                '  path_instructions:\n'
                '    - path: "a/**"\n'
                "      instructions: |\n"
                "        one\n"
                "        two\n"
                '    - path: "b/**"\n'
                "      instructions: |\n"
                "        three\n"
            )
        )
        entries = config["reviews"]["path_instructions"]
        self.assertEqual([e["path"] for e in entries], ["a/**", "b/**"])
        self.assertEqual(entries[0]["instructions"], "one\ntwo")
        self.assertEqual(entries[1]["instructions"], "three")

    def test_block_scalar_stops_at_dedent(self):
        config = cr._lists(
            cr.parse("a:\n  b: |\n    text\n  c: 1\n")
        )
        self.assertEqual(config["a"]["b"], "text")
        self.assertEqual(config["a"]["c"], 1)

    def test_tab_is_refused_with_a_line_number(self):
        with self.assertRaises(cr.ConfigError) as caught:
            cr.parse("a: 1\n\tb: 2\n")
        self.assertIn("line 2", str(caught.exception))

    def test_carriage_return_is_refused(self):
        with self.assertRaises(cr.ConfigError):
            cr.parse("a: 1\r\n")

    def test_scalar_list_items(self):
        config = cr._lists(cr.parse('a:\n  - "x"\n  - y\n'))
        self.assertEqual(config["a"], ["x", "y"])


class ProblemsTest(unittest.TestCase):
    def setUp(self):
        self.base = tree("src/main.c", "tests/test_a.c", "site/src/app.jsx")

    def check(self, text):
        return cr.problems(text, self.base)

    def test_minimal_config_passes(self):
        self.assertEqual(self.check(MINIMAL), [])

    def test_unknown_top_level_key(self):
        found = self.check("language: en-US\nreviewz:\n  profile: chill\n")
        self.assertTrue(any("unknown top-level key 'reviewz'" in f for f in found))

    def test_unknown_reviews_key(self):
        found = self.check(MINIMAL.replace("  profile: chill", "  profile: chill\n  pome: false"))
        self.assertTrue(any("'pome'" in f for f in found))

    def test_unknown_auto_review_key(self):
        found = self.check(
            MINIMAL.replace("  profile: chill", "  profile: chill\n  auto_review:\n    enabled: false\n    drats: false")
        )
        self.assertTrue(any("'drats'" in f for f in found))

    def test_profile_must_be_one_of_three(self):
        found = self.check(MINIMAL.replace("profile: chill", "profile: aggressive"))
        self.assertTrue(any("quiet, chill or assertive" in f for f in found))

    def test_missing_reviews_section(self):
        self.assertIn("reviews: missing", self.check("language: en-US\n"))

    def test_empty_filters(self):
        found = self.check(MINIMAL.replace('    - "src/**"\n', ""))
        self.assertTrue(any("path_filters is empty" in f for f in found))

    def test_filter_that_matches_nothing(self):
        found = self.check(MINIMAL.replace('    - "src/**"', '    - "app/**"'))
        self.assertTrue(any("path filter 'app/**' matches no file" in f for f in found))

    def test_negated_filter_may_match_nothing(self):
        # Excluding a directory that is not there yet is harmless; excluding one
        # that is there is the point, and neither is an error.
        found = self.check(MINIMAL.replace('    - "src/**"', '    - "src/**"\n    - "!build/**"'))
        self.assertEqual(found, [])

    def test_path_instruction_for_a_renamed_directory(self):
        found = self.check(MINIMAL.replace('path: "src/**"', 'path: "source/**"'))
        self.assertTrue(any("matches no file" in f for f in found))

    def test_path_instruction_without_text(self):
        found = self.check(MINIMAL.replace("        Look at the C.\n", ""))
        self.assertTrue(any("has no instructions" in f for f in found))

    def test_tool_for_a_language_not_in_the_repository(self):
        found = self.check(
            MINIMAL.replace("    markdownlint:\n      enabled: true",
                            "    markdownlint:\n      enabled: true\n    phpstan:\n      enabled: true")
        )
        self.assertTrue(any("'phpstan'" in f for f in found))

    def test_disabled_tool_is_not_a_problem(self):
        found = self.check(
            MINIMAL.replace("    markdownlint:\n      enabled: true",
                            "    markdownlint:\n      enabled: true\n    phpstan:\n      enabled: false")
        )
        self.assertEqual(found, [])

    def test_automatic_approvals_are_refused(self):
        found = self.check(MINIMAL.replace("  profile: chill", "  profile: chill\n  auto_approvals:\n    enabled: true"))
        self.assertTrue(any("auto_approvals is on" in f for f in found))

    def test_language_is_required(self):
        found = self.check(MINIMAL.replace("language: en-US\n", ""))
        self.assertTrue(any("language is missing" in f for f in found))


class RepositoryTest(unittest.TestCase):
    """The real file, not a fixture: this is what CI reads."""

    def setUp(self):
        self.text = (cr.ROOT / cr.CONFIG).read_text()
        self.config = cr._lists(cr.parse(self.text))

    def test_the_configuration_in_the_repository_is_valid(self):
        self.assertEqual(cr.problems(self.text), [])

    def test_it_is_assertive_because_small_mistakes_ship_here(self):
        self.assertEqual(self.config["reviews"]["profile"], "assertive")

    def test_no_poem_and_no_sequence_diagram(self):
        self.assertFalse(self.config["reviews"]["poem"])
        self.assertFalse(self.config["reviews"]["sequence_diagrams"])

    def test_it_reviews_main_only(self):
        self.assertEqual(
            self.config["reviews"]["auto_review"]["base_branches"], ["^main$"]
        )

    def test_generated_files_are_out_of_scope(self):
        filters = self.config["reviews"]["path_filters"]
        for pattern in (
            "!**/*.generated.json",
            "!docs/tools.json",
            "!spike/spike.diff",
        ):
            self.assertIn(pattern, filters)

    def test_the_core_rules_name_the_things_that_break(self):
        entries = {
            e["path"]: e["instructions"]
            for e in self.config["reviews"]["path_instructions"]
        }
        core = entries["src/core/**"]
        for word in ("gint", "libm", "test"):
            self.assertIn(word, core)

    def test_the_rules_forbid_editing_generated_lists(self):
        docs = {
            e["path"]: e["instructions"]
            for e in self.config["reviews"]["path_instructions"]
        }["docs/**"]
        self.assertIn("generated", docs)


if __name__ == "__main__":
    unittest.main()
