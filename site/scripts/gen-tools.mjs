// Joins the add-in's tool registry to the site's copy and screenshots.
//
// The registry is the only list of tools in the project (src/apps/registry.c;
// CI writes docs/tools.json from it). This script reads that file, merges in the
// hand-written copy from src/toolCopy.js and writes src/tools.generated.json.
//
// It fails when:
//   - docs/tools.json is missing or stale (run the add-in's generator first)
//   - a tool in the registry has no copy, so the site would understate the app
//   - the copy names a tool the add-in does not have
//   - a screenshot the copy points at is not in docs/images
//
// Run by "npm run dev" and "npm run build" before Vite, so the page can never
// describe a version of the add-in that no longer exists.
import { existsSync, readFileSync, readdirSync, writeFileSync } from "node:fs";
import { resolve } from "node:path";
import { pathToFileURL } from "node:url";

const root = resolve(import.meta.dirname, "..");
const registryPath = resolve(root, "../docs/tools.json");
const imagesDir = resolve(root, "../docs/images");
const outPath = resolve(root, "src/tools.generated.json");

if (!existsSync(registryPath)) {
  console.error(`no ${registryPath} - run "make sim && python3 tools/gen_tools_json.py"`);
  process.exit(1);
}

const { toolCopy, shellIds } = await import(pathToFileURL(resolve(root, "src/toolCopy.js")).href);

const registry = JSON.parse(readFileSync(registryPath, "utf8"));
const shots = new Set(readdirSync(imagesDir));
const problems = [];

const tools = [];
for (const entry of registry) {
  if (shellIds.includes(entry.id)) continue;

  const copy = toolCopy[entry.id];
  if (!copy) {
    problems.push(`registry tool "${entry.id}" (${entry.title}) has no copy in src/toolCopy.js`);
    continue;
  }
  if (copy.shots.length === 0) problems.push(`"${entry.id}" has no screenshots`);
  for (const [file] of copy.shots) {
    if (!shots.has(file)) problems.push(`"${entry.id}" points at docs/images/${file}, which does not exist`);
  }
  if (!shots.has(copy.lead)) problems.push(`"${entry.id}" lead image ${copy.lead} is missing`);

  tools.push({
    id: entry.id,
    name: entry.title,
    subtitle: entry.subtitle,
    category: entry.category,
    help: entry.help,
    ...copy,
  });
}

for (const id of Object.keys(toolCopy)) {
  if (!registry.some((entry) => entry.id === id)) {
    problems.push(`src/toolCopy.js describes "${id}", which is not in the registry`);
  }
}

// Unsplash-style counting for the stats strip: every PNG the add-in produced.
// Only the tour's captures count as screens: the brand and Open Graph cards are
// pictures of the project, not pictures of the add-in, and counting them made
// the number move whenever a social card was regenerated.
const screens = [...shots].filter(
  (name) => name.endsWith(".png") && !name.startsWith("brand-") && !name.startsWith("og")
).length;

if (problems.length) {
  console.error("site data does not match the add-in:\n  " + problems.join("\n  "));
  process.exit(1);
}

const categories = ["Study", "Math", "Science", "Tools"];
const grouped = categories
  .map((name) => {
    const inCategory = tools.filter((tool) => tool.category === name);
    return inCategory.length ? { name, tools: inCategory.map((tool) => tool.id) } : null;
  })
  .filter(Boolean);

// Category order, then the page order the copy asks for (`rank`), then the
// registry's own order. The registry decides which tools exist; only the reading
// order is a site decision.
const ordered = grouped.flatMap((group) =>
  group.tools
    .map((id) => tools.find((tool) => tool.id === id))
    .sort((a, b) => (a.rank ?? 100) - (b.rank ?? 100)),
);
for (const group of grouped) {
  group.tools = ordered.filter((tool) => tool.category === group.name).map((tool) => tool.id);
}

writeFileSync(
  outPath,
  JSON.stringify({ generatedAt: null, tools: ordered, groups: grouped, counts: { tools: tools.length, screens } }, null, 2) + "\n",
);

const described = tools.map((tool) => tool.id).join(", ");
console.log(`site data: ${tools.length} tools (${described}), ${screens} screenshots`);
