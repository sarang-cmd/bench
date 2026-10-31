# AGENTS.md

Target: Casio fx-CG50, OS 3.81, 384x216 display.
Project: Bench, one add-in with five tools. Read docs/plan.md before any task.
Toolchain: fxSDK with gint. Build with: fxsdk build-cg

Rules:
- Write C. Handle one module or screen per task.
- Keep src/core free of gint includes. Test every function in src/core.
- Never write code for the OS, bootloader, or flash.
- Never read, change, or work around Examination Mode.
- Call only functions found in docs/ or the gint headers. Never invent a function name.
- Run native tests and the build before every commit.
- Avoid dynamic allocation inside loops.
- Write files only at session end or on a settings change.
- Add no dependency. Add no feature outside the task.
- Note every storage layout change in CHANGELOG.md.
- Ask the user when a header or doc does not answer a question.

Done means:
- Native tests pass and the build passes.
- You list every screen and every key the task uses.
- You list every file the task reads or writes.
- You list what needs the calculator to test.

## Environment notes
- The user runs Windows. fxlink does not work inside WSL.
- The user copies Bench-<version>.g3a to the calculator by hand in USB Flash mode.
- Work on one task file at a time. Never start a later task early.
