// Everything the page says about the add-in, in one place, so the copy and the
// screenshots stay together. Image files live in public/images, copied from
// docs/images by scripts/copy-images.mjs at build time.
//
// The list of tools is NOT written here. scripts/gen-tools.mjs reads
// docs/tools.json (generated from src/apps/registry.c) and joins it to the copy
// in src/toolCopy.js, writing src/tools.generated.json before Vite runs. Adding
// a tool to the add-in therefore either appears here or fails the build.
import generated from "./tools.generated.json";

export const VERSION = "1.0.0";

export const REPO = "https://github.com/sarang-cmd/bench";

// Served from this site, not from GitHub: the file sits in site/public and the
// build refuses to pass without it, so the button can only point at a file
// that is really there. The release carries the same bytes for anyone who
// wants them.
export const ADDIN_FILE = `Bench-${VERSION}.g3a`;
export const DOWNLOAD = `./${ADDIN_FILE}`;

export const tools = generated.tools;
export const toolGroups = generated.groups;

// Derived, never typed: the hero, the headings and the stats strip all count the
// same list, so changing the registry changes all of them at once.
export const TOOL_COUNT = generated.counts.tools;
export const SCREEN_COUNT = generated.counts.screens;
export const TOOL_NAMES = tools.map((tool) => tool.name);

// The headline says "Twelve", not "12", so the count needs a word.
const WORDS = ["no", "one", "two", "three", "four", "five", "six", "seven",
  "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen",
  "fifteen", "sixteen", "seventeen", "eighteen", "nineteen", "twenty"];
export const TOOL_COUNT_WORD = WORDS[TOOL_COUNT] || String(TOOL_COUNT);

export const keys = [
  ["F1..F4", "home", "The four categories: Study, Math, Science, Tools"],
  ["LEFT / RIGHT", "home", "Turn the page inside a category"],
  ["F5", "home", "Help for the selected tile"],
  ["F5", "any tool", "That tool's own page: keys, a worked example"],
  ["F6", "home", "About: version, build, licence"],
  ["EXE", "everywhere", "Open a tile, confirm, reveal a card, solve"],
  ["EXIT", "everywhere", "One step back, up to the home screen"],
  ["MENU", "everywhere", "Leave the add-in for the calculator's menu"],
  ["Arrows", "everywhere", "Move, scroll, change a value"],
  ["F1 / F2", "Cards, answer shown", "Right / wrong"],
  ["F1 / F2", "Uncertainty result", "Chain the result / edit A"],
  ["F1 / F3", "Root finder", "Clear / solve"],
  ["F2", "Root finder", "Bisection, Newton, secant"],
  ["F2 / F3 / F4", "Complex numbers", "Operation / degrees / radians"],
  ["F3 / F4", "Triangle, Function table", "Degrees / radians"],
  ["F1 / F3", "Number theory", "Run the function / clear"],
  ["F1..F4", "Sig figs", "Clear / practice / next mode / previous mode"],
  ["digits . -", "number fields", "Type a value; DEL removes a digit"],
];

export const specs = [
  ["Target", "Casio fx-CG 50 (fx-CG 20 too), OS 3.81"],
  ["Screen", "384\u00d7216 visible window, 396\u00d7224 framebuffer"],
  ["Built with", "fxSDK + gint 2.10, C11"],
  ["Version", VERSION],
  ["Licence", "MIT"],
  ["Install", "One .g3a in the storage root"],
];
