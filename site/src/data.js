// Everything the page says about the add-in, in one place, so the copy and the
// screenshots stay together. Image files live in public/images, copied from
// docs/images by scripts/copy-images.mjs at build time.

export const VERSION = "1.0.0";

export const REPO = "https://github.com/sarang-cmd/bench";

// Served from this site, not from GitHub: the file sits in site/public and the
// build refuses to pass without it, so the button can only point at a file
// that is really there. The release carries the same bytes for anyone who
// wants them.
export const ADDIN_FILE = `Bench-${VERSION}.g3a`;
export const DOWNLOAD = `./${ADDIN_FILE}`;

export const tools = [
  {
    id: "pacer",
    name: "Pacer",
    tag: "0.5-90 minutes",
    accent: "red",
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
  {
    id: "uncertainty",
    name: "Uncertainty",
    tag: "error arithmetic",
    accent: "blue",
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
  {
    id: "cards",
    name: "Cards",
    tag: "Leitner drill",
    accent: "yellow",
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
  {
    id: "notes",
    name: "Notes",
    tag: "Markdown viewer",
    accent: "teal",
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
  {
    id: "number-theory",
    name: "Number theory",
    tag: "unsigned 64-bit",
    accent: "violet",
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
];

export const keys = [
  ["Arrows", "everywhere", "Move, scroll, change a value"],
  ["EXE", "everywhere", "Confirm, reveal a card, run"],
  ["EXIT", "everywhere", "One step back"],
  ["MENU", "everywhere", "Leave the add-in for the main menu"],
  ["F1", "Pacer run", "Log a mark"],
  ["F1 / F2", "Cards, answer shown", "Right / wrong"],
  ["F1 / F2", "Uncertainty result", "Chain the result / edit A"],
  ["F1 / F2", "Notes", "Search / next hit"],
  ["F1", "Number theory", "Run the function"],
  ["F3 / F4", "Notes", "Heading list / jump to the top"],
  ["F5 / F6", "Home", "Settings / About"],
  ["digits . -", "Number fields", "Type a value, DEL removes"],
];

export const specs = [
  ["Target", "Casio fx-CG 50 (fx-CG 20 too), OS 3.81"],
  ["Screen", "384\u00d7216 visible window, 396\u00d7224 framebuffer"],
  ["Built with", "fxSDK + gint 2.10, C11"],
  ["Version", VERSION],
  ["Licence", "MIT"],
  ["Install", "One .g3a in the storage root"],
];
