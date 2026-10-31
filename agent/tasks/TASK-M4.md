# TASK M4: Uncertainty

Gate: M3 passed on the calculator. Start only after the user confirms the gate passed.
Spec: Plan section 6.2.
Version: 0.2.0.

Goal: build the Uncertainty tool.

Build:
- src/core/numfmt.c and numfmt.h: decimal formatter with no float printf and half-up rounding on decimal digits.
- src/core/uncert.c and uncert.h: all operations in section 6.2, chain mode, significant figure setting.
- tests for every acceptance row and every edge case.
- src/apps/app_uncert.c: input, operation pick, result screen with absolute and percentage uncertainty.

Output: Bench-0.2.0.g3a. Tag 0.2.0 in CHANGELOG.md.

Rules: follow AGENTS.md. Read the plan sections named below in docs/plan.md.
Done means:
- Native tests pass and fxsdk build-cg passes.
- You list every screen and every key the task uses.
- You list every file the task reads or writes.
- You list what needs the calculator to test.
- You name the output file Bench-<version>.g3a.
