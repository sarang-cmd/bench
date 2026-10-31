# TASK M1: Spike add-in

Gate: M0 passed on the calculator. Start only after the user confirms the gate passed.
Spec: Plan section 7, M1 spike questions.
Version: 0.0.2.

Goal: build a throwaway add-in to answer ten questions on the real calculator. The user runs the add-in and reports the screen output. Do not guess answers.

Questions:
1. Which file calls list, read, write, and delete files?
2. How do paths look? What is the folder separator?
3. Does printf with floats work? If not, which formatter works?
4. Does the default font cover degrees, plus-minus, times, and micro? Which Greek letters work?
5. How do you read ALPHA letters from the keyboard?
6. Which timer or RTC call gives one tick per second?
7. How much heap does malloc give? What does the extra arena add?
8. What is the exact drawable width and height?
9. How long does a full-screen redraw take?
10. What size is the empty add-in?

Method:
- One menu with ten entries. Each entry runs one test and prints short result lines for the user to photograph.
- Use only functions found in the gint headers and docs. Quote the header file name for each call.
- Create the test file under a Bench folder and delete the file after the test.

Output: Bench-spike-0.0.2.g3a and docs/spike.md with the questions, the calls you used, and a column for the user's result.

Rules: follow AGENTS.md. Read the plan sections named below in docs/plan.md.
Done means:
- Native tests pass and fxsdk build-cg passes.
- You list every screen and every key the task uses.
- You list every file the task reads or writes.
- You list what needs the calculator to test.
- You name the output file Bench-<version>.g3a.
