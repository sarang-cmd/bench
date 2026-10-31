# TASK M3: Pacer

Gate: M2 passed on the calculator. Start only after the user confirms the gate passed.
Spec: Plan section 6.1.
Version: 0.1.0.

Goal: build the Pacer tool.

Build:
- src/core/pacer.c and pacer.h: minutes per mark, checkpoint times, gap to pace.
- tests/test_pacer.c with every acceptance row in section 6.1 and the edge cases.
- src/apps/app_pacer.c: setup, plan, run, done screens.

Output: Bench-0.1.0.g3a. Tag 0.1.0 in CHANGELOG.md.

Rules: follow AGENTS.md. Read the plan sections named below in docs/plan.md.
Done means:
- Native tests pass and fxsdk build-cg passes.
- You list every screen and every key the task uses.
- You list every file the task reads or writes.
- You list what needs the calculator to test.
- You name the output file Bench-<version>.g3a.
