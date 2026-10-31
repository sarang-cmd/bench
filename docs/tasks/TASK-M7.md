# TASK M7: Markdown

Gate: M6 passed on the calculator. Start only after the user confirms the gate passed.
Spec: Plan section 6.4 (Markdown subset, parser design, style Phase A, md2bench).
Version: 0.5.0.

Goal: add Markdown rendering to Notes.

Build:
- src/core/md_parse.c: block classify, inline spans, no recursion.
- src/core/md_layout.c: wrap to display lines with span records.
- Style Phase A: color and underline only. Map styles in theme.h.
- Unsupported syntax shows as plain text.
- Characters outside the font show as ? and the footer lists the count.
- tools/md2bench.py: smart quote and symbol mapping, front matter and HTML strip, change report.
- tests with a note using every row of the syntax table.

Output: Bench-0.5.0.g3a. Tag 0.5.0 in CHANGELOG.md. Add docs/samples/markdown.md.

Rules: follow AGENTS.md. Read the plan sections named below in docs/plan.md.
Done means:
- Native tests pass and fxsdk build-cg passes.
- You list every screen and every key the task uses.
- You list every file the task reads or writes.
- You list what needs the calculator to test.
- You name the output file Bench-<version>.g3a.
