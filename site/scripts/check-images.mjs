// Fails the build if the page points at a screenshot that does not exist.
// The v1 static page shipped "24_numbertheory_gcd.png" while the file on disk
// was "24_numtheory_gcd.png", so the Number theory picture never loaded.
import { existsSync, readdirSync, readFileSync, statSync } from "node:fs";
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

// The download button points at a file on this site, so that file has to be in
// public/. It is the release build, copied in here by hand (see README), not
// something a visitor should have to fetch from GitHub.
// The page links whatever `scripts/fetch-addin.mjs` copied in, which is not
// always the version in version.txt: between a version bump and the first
// tagged build for it, the site keeps handing out the newest build it has and
// names that version. So check the file the page actually names.
const version = readFileSync(join(root, "..", "version.txt"), "utf8").trim();
const info = JSON.parse(readFileSync(join(root, "src", "build-info.generated.json"), "utf8"));
const addin = join(root, "public", info.file);
if (!existsSync(addin)) {
  console.error(`missing add-in for the download button: public/${info.file}`);
  process.exit(1);
}
if (info.version !== version) {
  console.log(
    `download: public/${info.file} - the newest build there is; ` +
      `version.txt already says ${version}, which the next tagged build will produce`,
  );
}
console.log(`download: public/${info.file} (${Math.round(statSync(addin).size / 1024)} KiB)`);
