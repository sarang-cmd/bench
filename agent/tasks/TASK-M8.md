# TASK M8: Number theory

Gate: M7 passed on the calculator. Start only after the user confirms the gate passed.
Spec: Plan section 6.5.
Version: 0.6.0.

Goal: build the Number theory tool.

Build:
- src/core/numtheory.c: gcd, lcm, ext gcd, modular inverse, modpow, Miller-Rabin, trial division plus Pollard rho, totient, CRT.
- mulmod with 32-bit limbs or double-and-add. No 128-bit integer.
- Cancel long jobs with EXIT.
- tests for every acceptance row and every edge case.
- src/apps/app_numtheory.c: function list, input form, result view with steps for gcd and ext gcd, progress indicator.

Output: Bench-0.6.0.g3a. Tag 0.6.0 in CHANGELOG.md.

Rules: follow AGENTS.md. Read the plan sections named below in docs/plan.md.
Done means:
- Native tests pass and fxsdk build-cg passes.
- You list every screen and every key the task uses.
- You list every file the task reads or writes.
- You list what needs the calculator to test.
- You name the output file Bench-<version>.g3a.
