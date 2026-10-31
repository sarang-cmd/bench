# docs/

Where the written record of Bench lives. Everything here is meant to be read by
a person, and by the next agent that picks the project up.

| File | What it is | State |
|------|------------|-------|
| `plan.md` | the implementation plan, v1 | complete, with a "what changed" section at the end |
| `STATUS.md` | the resume point: what is done, what is tested, what needs a calculator | current |
| `toolchain.md` | how to install the fxSDK, build the `.g3a`, run the native tests, and what blocked the first build | complete |
| `spike.md` | the ten M1 questions, the exact calls used, and the result column | result column **empty on purpose**: it needs the calculator |
| `tasks/TASK-M0.md` ... `TASK-M9.md` | the per-milestone specs, copied from `agent/tasks/` | complete |
| `samples/markdown.md` | one Markdown file using every syntax row the viewer supports | complete |
| `samples/cards.txt` | a ten-card physics deck, with one deliberately broken line | complete |
| `samples/note.txt` | a plain-text note for the viewer | complete |
| `images/` | 26 screenshots of the real screens, plus the brand card and wordmark | regenerate with `make shots` |

## The order to read them in

1. `STATUS.md` - where the project stands.
2. `plan.md` sections 4 to 6 - the storage layout and what each tool does.
3. `toolchain.md` - how to build and test.
4. `spike.md` - the questions only the calculator can answer.

## What is not here

- No OS, bootloader or flash code, and nothing about Exam Mode beyond "add-ins
  do not run in it".
- No generated binaries. `Bench-<version>.g3a` comes from
  `.github/workflows/build.yml` or from `fxsdk build-cg`.
- The screenshots are PPM files converted to PNG by `tools/sim/ppm2png.py`; the
  PPM originals are not committed (they are in `.gitignore`).
