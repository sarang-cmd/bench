// Counts for the About page and the footer strip: every number on the site is
// read from the repository at build time, never typed.
//
//   node scripts/gen-stats.mjs
//
// Writes src/stats.generated.json.  Runs before Vite, next to gen-tools.mjs.
import { readdirSync, readFileSync, statSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, "..", "..");
const site = join(here, "..");

function files(dir, match) {
  try {
    return readdirSync(dir)
      .map((name) => join(dir, name))
      .filter((path) => statSync(path).isFile() && match(path));
  } catch {
    return [];
  }
}

// Lines of C in src/, so the "how big is it" claim is the real number.
function sourceLines() {
  let total = 0;
  const walk = (dir) => {
    for (const name of readdirSync(dir)) {
      const path = join(dir, name);
      if (statSync(path).isDirectory()) walk(path);
      else if (/\.(c|h)$/.test(name)) {
        total += readFileSync(path, "utf8").split("\n").length - 1;
      }
    }
  };
  walk(join(root, "src"));
  return total;
}

const tests = files(join(root, "tests"), (p) => /\.c$/.test(p));
const tour = files(join(root, "tools", "sim", "tour"), (p) => /\.txt$/.test(p));
const images = files(join(root, "docs", "images"), (p) => /\.png$/.test(p));
const tools = JSON.parse(readFileSync(join(root, "docs", "tools.json"), "utf8"));
// Settings and About are screens, not tools; the site counts tools the same way
// gen-tools.mjs does.
const toolRows = tools.filter((t) => t.id !== "settings" && t.id !== "about");

const version = readFileSync(join(root, "version.txt"), "utf8").trim();

const stats = {
  version,
  tools: toolRows.length,
  screens: images.filter((p) => !p.includes("brand-")).length,
  tourSteps: tour.length,
  testPrograms: tests.length,
  sourceLines: sourceLines(),
};

writeFileSync(join(site, "src", "stats.generated.json"), JSON.stringify(stats, null, 2) + "\n");

// shields.io endpoint documents, so a README badge can be live instead of a
// number someone forgot to update. See [tools] and [screens] in the README.
const badge = (label, message, color) =>
  JSON.stringify({ schemaVersion: 1, label, message, color }, null, 2) + "\n";
writeFileSync(join(site, "public", "badge-tools.json"), badge("tools", String(stats.tools), "d92d27"));
writeFileSync(join(site, "public", "badge-screens.json"), badge("screens", String(stats.screens), "1f9c8a"));
console.log(`site stats: ${stats.tools} tools, ${stats.screens} screens, ${stats.testPrograms} test programs`);
