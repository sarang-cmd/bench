# TASK M5: Cards

Gate: M4 passed on the calculator. Start only after the user confirms the gate passed.
Spec: Plan sections 4, 6.3.
Version: 0.3.0.

Goal: build the Cards tool.

Build:
- src/core/cards_parse.c: parse question | answer lines, line numbers on errors, limits from section 6.3.
- src/core/leitner.c: three boxes and session draw rules.
- Progress file keyed by a hash of the question text. Save at session end only.
- tests for parsing, box moves, hash stability, and a 500 card deck.
- src/apps/app_cards.c: deck list, session, summary, box stats.

Output: Bench-0.3.0.g3a. Tag 0.3.0 in CHANGELOG.md. Add a sample deck in docs/samples/cards.txt.

Rules: follow AGENTS.md. Read the plan sections named below in docs/plan.md.
Done means:
- Native tests pass and fxsdk build-cg passes.
- You list every screen and every key the task uses.
- You list every file the task reads or writes.
- You list what needs the calculator to test.
- You name the output file Bench-<version>.g3a.
