"""Tests for md2bench.py.  Run with: make py-test"""

import io
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import md2bench  # noqa: E402


class TestCharacterMapping(unittest.TestCase):
    def test_clean_text_is_untouched(self):
        text = "# Title\n\nPlain ASCII body, 2 + 2 = 4.\n"
        out, report = md2bench.clean(text)
        self.assertEqual(out, text)
        self.assertTrue(report.is_clean())

    def test_smart_quotes_and_dashes(self):
        out, report = md2bench.clean("\u201chi\u201d it\u2019s --\n")
        self.assertEqual(out, '"hi" it\'s --\n')
        self.assertEqual(report.replacements["\u201c"], 1)
        self.assertEqual(report.replacements["\u201d"], 1)
        self.assertEqual(report.replacements["\u2019"], 1)

    def test_ellipsis_and_en_dash(self):
        out, report = md2bench.clean("wait\u2026 1\u20132\n")
        self.assertEqual(out, "wait... 1-2\n")
        self.assertEqual(report.replacements["\u2026"], 1)
        self.assertEqual(report.replacements["\u2013"], 1)

    def test_em_dash_and_bullet(self):
        out, _ = md2bench.clean("a\u2014b\n\u2022 item\n")
        self.assertEqual(out, "a--b\n* item\n")

    def test_symbols(self):
        out, report = md2bench.clean("20\u00b11 \u00b0C, 3\u00d74, 6\u00f72, 5\u00b5m\n")
        self.assertEqual(out, "20+/-1 degC, 3x4, 6/2, 5um\n")
        self.assertEqual(report.replacements["\u00b1"], 1)
        self.assertEqual(report.replacements["\u00d7"], 1)
        self.assertEqual(report.replacements["\u00f7"], 1)

    def test_mathematical_symbols(self):
        out, _ = md2bench.clean(
            "2 \u2264 x \u2264 5, x\u2260y, \u221a2, \u221e, \u03c0, \u0394v\n")
        self.assertEqual(out, "2 <= x <= 5, x!=y, sqrt2, inf, pi, deltav\n")

    def test_superscripts(self):
        out, _ = md2bench.clean("m/s\u00b2 and x\u00b3\n")
        self.assertEqual(out, "m/s^2 and x^3\n")

    def test_non_breaking_space(self):
        out, _ = md2bench.clean("a\u00a0b\n")
        self.assertEqual(out, "a b\n")

    def test_output_is_ascii_when_everything_maps(self):
        text = "\u201cq\u201d \u00b1 \u00d7 \u00b0 \u2026\n"
        out, report = md2bench.clean(text)
        self.assertTrue(all(ord(ch) < 0x80 for ch in out), out)
        self.assertFalse(report.unsupported_chars)


class TestUnsupported(unittest.TestCase):
    def test_unmapped_characters_are_reported_and_kept(self):
        out, report = md2bench.clean("caf\u00e9 \u03b6\n")
        self.assertIn("caf\u00e9", out)
        self.assertEqual(report.unsupported_chars["\u00e9"], 1)
        self.assertEqual(report.unsupported_chars["\u03b6"], 1)
        self.assertFalse(report.is_clean())

    def test_table_is_reported(self):
        out, report = md2bench.clean("| a | b |\n|---|---|\n")
        self.assertEqual(report.unsupported_blocks,
                         [(1, "table"), (2, "table")])
        # Reported, not removed: the viewer shows it as plain text.
        self.assertIn("| a | b |", out)

    def test_footnote_is_reported(self):
        _, report = md2bench.clean("text [^1]\n\n[^1]: the note\n")
        self.assertEqual(report.unsupported_blocks, [(3, "footnote")])


class TestStructureStripping(unittest.TestCase):
    def test_front_matter_removed(self):
        text = "---\ntitle: x\ntags: y\n---\n# Body\n"
        out, report = md2bench.clean(text)
        self.assertEqual(out, "# Body\n")
        self.assertEqual(report.front_matter_lines, 4)

    def test_front_matter_only_at_the_start(self):
        text = "# Body\n\n---\n\nmore\n"
        out, report = md2bench.clean(text)
        self.assertEqual(report.front_matter_lines, 0)
        self.assertIn("---", out)  # a horizontal rule stays a rule

    def test_html_tags_removed_text_kept(self):
        out, report = md2bench.clean("<b>bold</b> and <br/> text\n")
        self.assertEqual(out, "bold and  text\n")
        self.assertEqual(report.html_tags, 3)

    def test_html_comment_and_doctype(self):
        out, report = md2bench.clean("<!DOCTYPE html>\n<!-- c -->\n")
        self.assertEqual(out, "\n\n")
        self.assertEqual(report.html_tags, 2)

    def test_image_line_removed(self):
        out, report = md2bench.clean("before\n![a picture](p.png)\nafter\n")
        self.assertEqual(out, "before\nafter\n")
        self.assertEqual(report.removed_lines, [2])
        self.assertEqual(report.images, 1)

    def test_inline_image_becomes_alt_text(self):
        out, report = md2bench.clean("see ![the graph](g.png) here\n")
        self.assertEqual(out, "see the graph here\n")
        self.assertEqual(report.images, 1)
        self.assertFalse(report.removed_lines)

    def test_indented_image_line_removed(self):
        out, report = md2bench.clean("  ![x](y.png)  \n")
        self.assertEqual(out, "")
        self.assertEqual(report.removed_lines, [1])

    def test_crlf_normalised(self):
        out, _ = md2bench.clean("a\r\nb\r\n")
        self.assertEqual(out, "a\nb\n")

    def test_output_ends_with_newline(self):
        out, _ = md2bench.clean("no newline")
        self.assertTrue(out.endswith("\n"))

    def test_blank_line_runs_collapsed(self):
        text = "a\n\n\n\n\nb\n"
        out, _ = md2bench.clean(text)
        self.assertEqual(out, "a\n\nb\n")

    def test_empty_input(self):
        out, report = md2bench.clean("")
        self.assertEqual(out, "")
        self.assertTrue(report.is_clean())


class TestCommandLine(unittest.TestCase):
    def test_convert_file_writes_into_the_output_folder(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "note.md"
            src.write_text("\u201chello\u201d\n", encoding="utf-8")
            out_dir = Path(tmp) / "out"
            target, report = md2bench.convert_file(src, out_dir)
            self.assertEqual(target, out_dir / "note.md")
            self.assertEqual(target.read_text(encoding="utf-8"), '"hello"\n')
            self.assertFalse(report.is_clean())

    def test_main_check_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            dirty = Path(tmp) / "dirty.md"
            dirty.write_text("\u2026\n", encoding="utf-8")
            clean = Path(tmp) / "clean.md"
            clean.write_text("plain\n", encoding="utf-8")

            with redirect_stdout(io.StringIO()):
                self.assertEqual(md2bench.main([str(dirty), "--check"]), 1)
                self.assertEqual(md2bench.main([str(clean), "--check"]), 0)

    def test_main_writes_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "in.md"
            src.write_text("1 \u2013 2\n", encoding="utf-8")
            out_dir = Path(tmp) / "notes"
            with redirect_stdout(io.StringIO()):
                code = md2bench.main([str(src), "-o", str(out_dir), "-q"])
            self.assertEqual(code, 0)
            self.assertEqual((out_dir / "in.md").read_text(encoding="utf-8"),
                             "1 - 2\n")

    def test_main_stdout_mode_writes_nothing(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "in.md"
            src.write_text("a\u2026b\n", encoding="utf-8")
            out_dir = Path(tmp) / "notes"
            buf = io.StringIO()
            with redirect_stdout(buf):
                code = md2bench.main([str(src), "--stdout", "-q"])
            self.assertEqual(code, 0)
            self.assertEqual(buf.getvalue(), "a...b\n")
            self.assertFalse(out_dir.exists())

    def test_missing_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            with redirect_stdout(io.StringIO()):
                code = md2bench.main([str(Path(tmp) / "nope.md")])
            self.assertEqual(code, 2)


if __name__ == "__main__":
    unittest.main()
