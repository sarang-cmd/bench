// Fails the build if the page points at a screenshot that does not exist.
// The v1 static page shipped "24_numbertheory_gcd.png" while the file on disk
// was "24_numtheory_gcd.png", so the Number theory picture never loaded.
import { readdirSync, readFileSync, statSync } from "node:fs";
import { join, resolve } from "node:path";

const root = resolve(import.meta.dirname, "..");
const imageDir = resolve(root, "../docs/images");
const present = new Set(readdirSync(imageDir));
const referenced = new Set();

function scan(path) {
  if (!/\.(jsx?|html)$/.test(path)) return;
  const text = readFileSync(path, "utf8");
  for (const match of text.matchAll(/images\/([A-Za-z0-9_.-]+\.png)/g)) {
    referenced.add(match[1]);
  }
  for (const match of text.matchAll(/\[\s*"([0-9]{2}_[A-Za-z0-9_.-]+\.png)"/g)) {
    referenced.add(match[1]);
  }
}

function walk(dir) {
  for (const entry of readdirSync(dir)) {
    const path = join(dir, entry);
    if (statSync(path).isDirectory()) walk(path);
    else scan(path);
  }
}

walk(join(root, "src"));
scan(join(root, "index.html"));

const missing = [...referenced].filter((name) => !present.has(name)).sort();
console.log(`images referenced: ${referenced.size}, missing: ${missing.length}`);
if (missing.length) {
  console.error("missing screenshots:\n  " + missing.join("\n  "));
  process.exit(1);
}
