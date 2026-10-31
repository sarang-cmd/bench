// The words the site puts next to each tool. One entry per registry id.
//
// The registry (src/apps/registry.c -> docs/tools.json) says which tools exist,
// what they are called and which category they sit in; this file only adds the
// long-form copy and the screenshots. scripts/gen-tools.mjs joins the two and
// fails the build when a tool has no entry here, so adding a tool to the add-in
// cannot leave the site quietly describing an older version of it.
//
// Keys are registry ids, not titles: "numtheory", not "Number theory".

export const toolCopy = {
  pacer: {
    rank: 1,
    accent: "red",
    tag: "0.5-90 minutes",
    summary:
      "Minutes per mark, checkpoint times, and a live gap line: \u201cBehind by 1m20s\u201d.",
    body:
      "Enter the length of the paper and how many marks it carries. Bench splits " +
      "it into checkpoints and then runs the clock against them, four redraws a " +
      "second so the battery is not asked for more.",
    points: [
      "Setup: total minutes, total marks, marks per checkpoint.",
      "90 minutes and 80 marks gives 1.125 min/mark and eight checkpoints.",
      "Run: F1 logs a mark, F2 pauses, F3 finishes. The gap line is the point.",
      "Done: total time and the final gap, nothing to write down.",
    ],
    lead: "06_pacer_done.png",
    leadCaption: "Finished: total time and final gap",
    shots: [
      ["02_pacer_setup.png", "Setup: minutes, marks, checkpoint size"],
      ["03_pacer_plan.png", "The plan: min/mark and every checkpoint time"],
      ["04_pacer_run.png", "Running: clock, marks logged, gap line"],
      ["05_pacer_paused.png", "Paused mid-paper"],
      ["06_pacer_done.png", "Finished: total time and final gap"],
    ],
  },

  cards: {
    rank: 2,
    accent: "yellow",
    tag: "Leitner drill",
    summary:
      "A flashcard drill over a plain text deck, spaced across three boxes.",
    body:
      "Decks are text you can write anywhere: one card a line, question and " +
      "answer split by a vertical bar. Right moves a card up a box, wrong sends " +
      "it back to box 1, and boxes 2 and 3 come round less often.",
    points: [
      "\u201cWhat is the unit of impulse? | N s\u201d is a whole card.",
      "500 cards a deck, 120 characters a side; a broken line is reported with its number.",
      "Progress is keyed by a hash of the question, so editing an answer keeps its box.",
      "Saved once, when the session ends, never inside the loop.",
    ],
    lead: "14_cards_answer.png",
    leadCaption: "The answer, and F1 / F2 to grade it",
    shots: [
      ["11_cards_list.png", "Decks found in Bench/cards"],
      ["12_cards_deck.png", "A deck: cards per box and when it is due"],
      ["13_cards_question.png", "The question"],
      ["14_cards_answer.png", "The answer, and F1 / F2 to grade it"],
      ["15_cards_summary.png", "Session summary: right, wrong, time"],
      ["16_cards_boxes.png", "Where the cards ended up"],
    ],
  },

  notes: {
    rank: 3,
    accent: "teal",
    tag: "Markdown viewer",
    summary:
      "Read text and a Markdown subset from the calculator's storage, with search.",
    body:
      "Drop .md and .txt files into Bench/notes and read them on the calculator. " +
      "Headings, bold, italic, code, lists, quotes and checkboxes are styled with " +
      "colour and rules rather than extra fonts, so nothing has to be installed.",
    points: [
      "Up to 32 KiB a file, long lines wrapped to the screen or to a set width.",
      "F1 search, F2 next hit, F3 the heading list, F4 back to the top.",
      "Characters the calculator font lacks are drawn as ? and counted in the footer.",
      "tools/md2bench.py maps typographic symbols down to ASCII before you copy.",
    ],
    lead: "18_notes_markdown.png",
    leadCaption: "A Markdown note: headings, bold, a list",
    shots: [
      ["17_notes_list.png", "The notes folder"],
      ["18_notes_markdown.png", "A Markdown note: headings, bold, a list"],
      ["19_notes_headings.png", "Jump to any heading with F3"],
      ["20_notes_scrolled.png", "Scrolling a long note by page"],
      ["21_notes_keyboard.png", "The on-screen keyboard for search"],
    ],
  },

  function: {
    rank: 1,
    accent: "blue",
    tag: "expression engine",
    summary:
      "One engine for x, pi, e and twenty functions, behind a table and a probe.",
    body:
      "Type an expression once on the on-screen keyboard and keep it in one of six " +
      "slots: f, g, h, p, q, r. A shunting-yard parser holds its operator stack in " +
      "a fixed 32 entries, so a long expression cannot overflow the calculator's " +
      "stack, and an error names the column it failed at.",
    points: [
      "2^10 = 1024, 3x at x = 4 gives 12, sin(pi/6) = 0.5.",
      "Implicit multiplication: 3x, 2(x+1), (x+1)(x-1), 2pi.",
      "A table of x and f(x) from a start value, a step and a count; F2 probes one value.",
      "Six expressions live in RAM and reach Bench/state/exprs.txt when you leave the tool.",
    ],
    lead: "32_function_table.png",
    leadCaption: "Six expressions, and a table of one of them",
    shots: [
      ["31_function_list.png", "The six slots and their expressions"],
      ["32_function_table.png", "A table of x against f(x)"],
      ["33_function_probe.png", "One value, probed"],
    ],
  },

  triangle: {
    rank: 2,
    accent: "yellow",
    tag: "SSS to SSA",
    summary:
      "Any three of the six values in, every side, angle, the area and the perimeter out.",
    body:
      "SSS, SAS, ASA, AAS and SSA go through the law of cosines and the law of " +
      "sines. The SSA case says when there are two triangles instead of quietly " +
      "picking one, and the six boxes sit in the shape of the triangle: a b c over " +
      "A B C, each angle under the side it is opposite.",
    points: [
      "3, 4, 5 gives C = 90\u00b0, A = 36.87\u00b0, area 6 and perimeter 12.",
      "SSA with a = 10, b = 15, A = 30\u00b0 reports two triangles; F2 swaps between them.",
      "Worked lines for the cosine rule and the sine rule.",
      "F3 degrees, F4 radians, and the unit is in the title bar.",
    ],
    lead: "40_triangle_sss.png",
    leadCaption: "3, 4, 5: the right angle, the area, the perimeter",
    shots: [
      ["40_triangle_sss.png", "3, 4, 5: the right angle, the area, the perimeter"],
      ["41_triangle_ssa.png", "The SSA case, with both triangles available"],
    ],
  },

  poly: {
    rank: 3,
    accent: "violet",
    tag: "to degree 8",
    summary:
      "Quadratics, expansion and division, with the discriminant, the vertex and the factors.",
    body:
      "Three screens over one fixed-size polynomial type: a quadratic that shows " +
      "the discriminant, both roots, the vertex and the factor pair; an expansion " +
      "of (ax + b)^n term by term; and a coefficient list whose rational roots are " +
      "hunted by the rational root theorem.",
    points: [
      "x\u00b2 - 5x + 6: discriminant 1, roots 3 and 2, vertex (2.5, -0.25).",
      "x\u00b3 - 6x\u00b2 + 11x - 6 factors as (x - 3)(x - 2)(x - 1).",
      "Divide x\u00b3 - 2x\u00b2 - 4 by x - 3: quotient x\u00b2 + x + 3, remainder 5.",
      "Coefficients are typed rather than parsed, because the keypad has no letters for x\u00b2.",
    ],
    lead: "37_poly_quad.png",
    leadCaption: "A quadratic: discriminant, roots, vertex, factor pair",
    shots: [
      ["37_poly_quad.png", "A quadratic: discriminant, roots, vertex, factor pair"],
      ["38_poly_expand.png", "(ax + b)^n, term by term"],
      ["39_poly_roots.png", "Roots from a coefficient list"],
    ],
  },

  roots: {
    rank: 4,
    accent: "red",
    tag: "bisection, Newton",
    summary:
      "f(x) = 0 one step at a time, with every iterate in a table and marked on the curve.",
    body:
      "Bisection, Newton and the secant method over an expression from the " +
      "expression engine. Newton takes its slope from a central difference with a " +
      "step of 1e-6, so no derivative has to be typed, and the table of n, x, f(x) " +
      "and the step size shows the error falling rather than asserting that it does.",
    points: [
      "Newton on x\u00b2 - 2 from 1: 1.5, 1.416667, 1.414216, root 1.414214 in four steps.",
      "Bisection on [1, 2] reaches the same root in twenty.",
      "No sign change and a zero slope are refused with a sentence, not guessed at.",
      "Radians always, said in the title bar, with a 50-step cap.",
    ],
    lead: "49_roots_newton.png",
    leadCaption: "Newton: four iterates and the plot that marks them",
    shots: [
      ["49_roots_newton.png", "Newton: four iterates and the plot that marks them"],
      ["50_roots_bisect.png", "Bisection: twenty halvings of [1, 2]"],
      ["51_roots_fail.png", "A bracket with no sign change"],
    ],
  },

  seq: {
    rank: 5,
    accent: "teal",
    tag: "terms and sums",
    summary:
      "Arithmetic and geometric sequences, the sum to infinity, and binomial coefficients.",
    body:
      "Terms and partial sums for both kinds of sequence, with the first few terms " +
      "listed under the answer so the pattern is visible. The binomial screen works " +
      "through Pascal's row, one term of (ax + b)^n, and a coefficient at a chosen " +
      "power.",
    points: [
      "u\u2081 = 3, d = 4: u\u2081\u2080 = 39 and S\u2081\u2080 = 210.",
      "u\u2081 = 2, r = 3: u\u2085 = 162 and S\u2085 = 242; with r = 0.5 the sum to infinity of 8 is 16.",
      "C(n,k) cancels before it multiplies, so C(62,31) still fits in 64 bits.",
      "A coefficient that would pass 64 bits is refused rather than printed wrong.",
    ],
    lead: "42_seq_arith.png",
    leadCaption: "Arithmetic: u\u2081\u2080 and S\u2081\u2080, with the terms underneath",
    shots: [
      ["42_seq_arith.png", "Arithmetic: u\u2081\u2080 and S\u2081\u2080, with the terms underneath"],
      ["43_seq_binom.png", "Pascal's row and a binomial coefficient"],
    ],
  },

  cplx: {
    rank: 6,
    accent: "violet",
    tag: "a + bi",
    summary:
      "Rectangular, modulus-and-argument and polar form on one screen.",
    body:
      "Two numbers, one operation at a time, and the answer written three ways so " +
      "it is obvious they are the same number. Addition, subtraction, " +
      "multiplication, division and integer powers, with the argument taken in the " +
      "angle mode you picked.",
    points: [
      "(3 + 4i)(1 - 2i) = 11 - 2i, and (3 + 4i)/(1 - 2i) = -1 + 2i.",
      "1 + i has modulus 1.414214 and argument 45\u00b0; (1 + i)\u2074 = -4.",
      "Dividing by 0 + 0i says \u201cnot a number\u201d instead of printing an infinity.",
      "F2 cycles the operation, F3 and F4 pick degrees or radians.",
    ],
    lead: "44_cplx_mul.png",
    leadCaption: "A product in all three forms",
    shots: [
      ["44_cplx_mul.png", "A product in all three forms"],
      ["45_cplx_div.png", "A quotient: -1 + 2i"],
      ["46_cplx_pow.png", "(1 + i)\u2074 = -4, with the argument at 180\u00b0"],
    ],
  },

  numtheory: {
    rank: 7,
    accent: "violet",
    tag: "unsigned 64-bit",
    summary:
      "gcd to CRT, in 64-bit integer arithmetic: no floats, no rounding surprises.",
    body:
      "Nine functions behind one menu, with the working shown where the working " +
      "is the point, such as Euclid's steps for gcd or an extended gcd. Inputs up " +
      "to 18 digits are exact.",
    points: [
      "gcd, lcm, extended gcd with B\u00e9zout coefficients, modular inverse.",
      "modpow by square and multiply with mulmod, so 2^64 mod 1000 is exact.",
      "Deterministic Miller-Rabin primality test for the whole 64-bit range.",
      "Factor by trial division then Pollard rho, with a progress bar and EXIT to cancel.",
      "Euler's totient and the Chinese remainder theorem, one pair or a whole list.",
    ],
    lead: "24_numtheory_gcd.png",
    leadCaption: "gcd(84, 36) = 12, Euclid's steps underneath",
    shots: [
      ["22_numtheory_list.png", "The nine functions"],
      ["23_numtheory_form.png", "Arguments, with live validation"],
      ["24_numtheory_gcd.png", "gcd(84, 36) = 12, Euclid's steps underneath"],
    ],
  },

  uncert: {
    rank: 1,
    accent: "blue",
    tag: "error arithmetic",
    summary:
      "Add, subtract, multiply, divide and raise measured values, errors included.",
    body:
      "Two values with their errors, one operation, and a result in the shape a " +
      "physics teacher wants: value, absolute uncertainty, percentage. F1 feeds " +
      "the result back in as A and you keep going.",
    points: [
      "A + B and A - B add the absolute errors; A * B and A / B add the relative ones.",
      "A ^ n and k * A are covered too, with the exact rule for each.",
      "Rounding is half-up on decimal digits, never on the binary float: 0.15 goes to 0.2.",
      "One figure for the uncertainty, or two when its first digit is 1.",
    ],
    lead: "10_uncert_result.png",
    leadCaption: "15.0 \u00b1 0.3, which is 2.0 %",
    shots: [
      ["07_uncert_a.png", "First value and its error"],
      ["08_uncert_op.png", "Choosing the operation and the second operand"],
      ["09_uncert_b.png", "Second value and its error"],
      ["10_uncert_result.png", "15.0 \u00b1 0.3, which is 2.0 %"],
    ],
  },

  sigfig: {
    rank: 2,
    accent: "blue",
    tag: "count, round, form",
    summary:
      "Significant figures counted on the text rather than the value, plus standard form and SI prefixes.",
    body:
      "Six modes: count the figures, round to a number of figures, round to a " +
      "number of decimals, standard form, SI prefix, and a practice mode that asks " +
      "and marks. Figures are counted on the characters you typed, which is why " +
      "1200 has two and 1200. has four.",
    points: [
      "0.004560 has 4 figures and is 4.560 \u00d7 10\u207b\u00b3.",
      "1234.5 to 3 figures is 1230, written 1.23 \u00d7 10\u00b3.",
      "4560 with a prefix is 4.56 k; 1e-9 is 1.00 n.",
      "Rounding is half-up on the decimal digits, never on the binary float.",
    ],
    lead: "35_sigfig_modes.png",
    leadCaption: "Count the figures, then round a third",
    shots: [
      ["34_sigfig.png", "Counting the figures in a typed number"],
      ["35_sigfig_modes.png", "Count the figures, then round a third"],
      ["36_sigfig_practice.png", "Practice: a question, marked"],
    ],
  },
};

// Settings and About are registry entries too, but they are the shell around the
// tools rather than tools; the site mentions them in the key list instead.
export const shellIds = ["settings", "about"];
