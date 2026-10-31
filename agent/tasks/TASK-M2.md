# TASK M2: Shell

Gate: M1 done and docs/spike.md filled in with the user's results. Start only after the user confirms the gate passed.
Spec: Plan sections 3, 4, 5.
Version: 0.0.3.

Goal: build the app shell.

Build:
- src/ui/theme.h with the paper, ink, and red colors in the calculator's color format.
- src/ui/widgets: list, number field, modal, softkey bar.
- src/platform: screen, input, store, mem. Base every call on docs/spike.md.
- src/apps/app_menu.c: home screen with five tiles. Tools show a "not built" screen.
- Settings file: load at start, save on change only.
- store.h interface hides every file call from the rest of the code.

Tests: native tests for any logic in src/core, such as settings parsing.
Output: Bench-0.0.3.g3a. Tag 0.0.3 in CHANGELOG.md.

Rules: follow AGENTS.md. Read the plan sections named below in docs/plan.md.
Done means:
- Native tests pass and fxsdk build-cg passes.
- You list every screen and every key the task uses.
- You list every file the task reads or writes.
- You list what needs the calculator to test.
- You name the output file Bench-<version>.g3a.
