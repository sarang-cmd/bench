// CHANGELOG.md -> src/changelog.generated.json, so the /changelog page can
// never disagree with the file.  Only the two shapes the file uses are parsed:
// "## <version> - <date>" headings and "- **Title**: body" bullets.
import { readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, "..", "..");

const text = readFileSync(join(root, "CHANGELOG.md"), "utf8");
const releases = [];
let release = null;
let entry = null;

for (const raw of text.split("\n")) {
  const line = raw.trimEnd();

  const heading = line.match(/^## (.+)$/);
  if (heading) {
    const [version, ...rest] = heading[1].split(" - ");
    release = { version, date: rest.join(" - ") || "", entries: [] };
    releases.push(release);
    entry = null;
    continue;
  }
  if (!release) continue;

  const bullet = line.match(/^- \*\*(.+?)\*\*:?\s*(.*)$/);
  if (bullet) {
    entry = { title: bullet[1], body: bullet[2] };
    release.entries.push(entry);
    continue;
  }
  if (entry && /^\s+\S/.test(raw)) {
    entry.body = `${entry.body} ${line.trim()}`.trim();
    continue;
  }
  if (line.trim() === "") entry = null;
}

writeFileSync(
  join(here, "..", "src", "changelog.generated.json"),
  JSON.stringify({ releases }, null, 2) + "\n"
);
console.log(`site changelog: ${releases.length} releases, ${releases.reduce((n, r) => n + r.entries.length, 0)} entries`);
