# TASK M6: Notes, plain text

Gate: M5 passed on the calculator. Start only after the user confirms the gate passed.
Spec: Plan sections 4, 6.4 (viewer features and search only).
Version: 0.4.0.

Goal: build the Notes viewer for plain text. Markdown comes in M7.

Build:
- File list from Bench/notes with .md and .txt files.
- Wrap text to the screen width with the font metrics from the platform layer.
- Scroll by line and by page. Show a footer with position.
- src/core/textsearch.c: case-insensitive substring search with tests.
- Search field and jump to the next match.
- Empty state for no files and for an empty file.
- Cap file size per section 6.4. Show an error line for larger files.

Output: Bench-0.4.0.g3a. Tag 0.4.0 in CHANGELOG.md. Add a sample note in docs/samples/note.txt.

Rules: follow AGENTS.md. Read the plan sections named below in docs/plan.md.
Done means:
- Native tests pass and fxsdk build-cg passes.
- You list every screen and every key the task uses.
- You list every file the task reads or writes.
- You list what needs the calculator to test.
- You name the output file Bench-<version>.g3a.
