# TASK M0: Toolchain and blank add-in

Gate: none.
Spec: Plan sections 2, 3, 7.
Version: 0.0.1.

Goal: build a blank add-in named Bench with fxSDK and gint in this container.

Steps:
1. Read the fxSDK README at https://gitea.planet-casio.com/Lephenixnoir/fxsdk and follow its install steps.
2. Install the SuperH cross compiler, fxSDK, and gint as the README describes.
3. Run: fxsdk new bench
4. Clear the screen to paper (#f4f3ee) and draw one red (#d92d27) square.
5. Wait for a key press, then exit.
6. Run: fxsdk build-cg
7. Write every install command into docs/toolchain.md so the image rebuilds from the file.
8. Output Bench-0.0.1.g3a and report the file size.

Write no file, timer, or text entry code. Those belong to M1.

Rules: follow AGENTS.md. Read the plan sections named below in docs/plan.md.
Done means:
- Native tests pass and fxsdk build-cg passes.
- You list every screen and every key the task uses.
- You list every file the task reads or writes.
- You list what needs the calculator to test.
- You name the output file Bench-<version>.g3a.
