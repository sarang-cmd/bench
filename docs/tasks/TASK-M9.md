# TASK M9: Polish and release

Gate: M8 passed on the calculator. Start only after the user confirms the gate passed.
Spec: Plan section 10.
Version: 1.0.0.

Goal: prepare Bench 1.0.0.

Build:
- Icons in assets-cg for the unselected and selected states, in the Bauhaus palette.
- Version screen with the Bench version, build date, and OS version.
- Remove debug output.
- Readme with keys, screens, file formats, folder layout, and known problems.
- Release notes stating which parts an AI agent wrote.

The user runs the Exam Mode test and the Cemetech post. Do not write code for either.

Output: Bench-1.0.0.g3a. Tag 1.0.0 in CHANGELOG.md.

Rules: follow AGENTS.md. Read the plan sections named below in docs/plan.md.
Done means:
- Native tests pass and fxsdk build-cg passes.
- You list every screen and every key the task uses.
- You list every file the task reads or writes.
- You list what needs the calculator to test.
- You name the output file Bench-<version>.g3a.
