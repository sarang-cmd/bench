// fetch-spike.mjs - the M1 diagnostic add-in, on the site.
//
//   node scripts/fetch-spike.mjs
//
// Two files, and they are different things:
//
//   spike.diff  the patch that builds it.  It lives in the repository
//               (spike/spike.diff, generated from spike/), so it is always
//               copied and always downloadable.
//   Bench-spike-<version>.g3a  the built diagnostic add-in.  A C build needs the
//               SuperH toolchain, which no deploy has, so the bytes come from
//               dist/ - where CI puts them next to Bench's own .g3a - or from
//               spike/ after a local `fxsdk build-cg`.
//
// src/spike.generated.json says which of them exist and what their hashes are.
// The page renders the card from that file and never from a guess, so a
// deployment can never offer a download that 404s.
import { createHash } from "node:crypto";
import {
  copyFileSync,
  existsSync,
  mkdirSync,
  readdirSync,
  readFileSync,
  statSync,
  writeFileSync,
} from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const repo = resolve(root, "..");
const publicDir = join(root, "public");
const infoPath = join(root, "src", "spike.generated.json");

mkdirSync(publicDir, { recursive: true });

const sha256 = (bytes) => createHash("sha256").update(bytes).digest("hex");

/* The patch: always there, straight from the repository. */
const patchSource = join(repo, "spike", "spike.diff");
const info = { patch: null, binary: null };

if (existsSync(patchSource)) {
  copyFileSync(patchSource, join(publicDir, "spike.diff"));
  const bytes = readFileSync(patchSource);
  info.patch = { file: "spike.diff", size: bytes.length, sha256: sha256(bytes) };
  console.log(`spike: spike.diff (${Math.round(bytes.length / 1024)} KiB)`);
}

/* The binary: whatever a real build has left behind, in order of preference. */
const candidates = [
  { dir: publicDir, from: "public" },
  { dir: join(repo, "dist"), from: "dist" },
  { dir: join(repo, "spike"), from: "spike" },
];

for (const { dir, from } of candidates) {
  if (!existsSync(dir)) continue;
  const name = readdirSync(dir)
    .filter((f) => /^Bench-spike-.+\.g3a$/.test(f))
    .sort()
    .pop();
  if (!name) continue;

  const source = join(dir, name);
  const bytes = readFileSync(source);
  /* A 404 page saved as a file, or a truncated copy, must not be offered. */
  if (bytes.length < 10_000 || bytes.subarray(0, 2000).includes(Buffer.from("<!DOCTYPE"))) {
    console.log(`spike: ${from}/${name} does not look like an add-in, skipping`);
    continue;
  }

  if (from !== "public") copyFileSync(source, join(publicDir, name));
  info.binary = {
    file: name,
    version: name.replace(/^Bench-spike-/, "").replace(/\.g3a$/, ""),
    size: statSync(join(publicDir, name)).size,
    sha256: sha256(bytes),
    source: from,
  };
  console.log(
    `spike: ${name} from ${from}/ (${Math.round(bytes.length / 1024)} KiB, ` +
      `sha256 ${info.binary.sha256.slice(0, 12)}...)`,
  );
  break;
}

/* dist/SPIKE.txt is written beside the build it describes; a missing file is a
 * missing answer, and the page says so rather than inventing a date. Its commit
 * only counts if it names the same file that was just copied. */
const provenance = join(repo, "dist", "SPIKE.txt");
if (info.binary && existsSync(provenance)) {
  const fields = {};
  for (const line of readFileSync(provenance, "utf8").split("\n")) {
    const [key, ...rest] = line.trim().split(" ");
    if (key) fields[key] = rest.join(" ");
  }
  if (fields.file === info.binary.file) {
    if (fields.commit) info.binary.commit = fields.commit;
    if (fields.built) info.binary.built = fields.built;
    const expected = fields.sha256;
    if (expected && expected !== info.binary.sha256) {
      console.log(
        `spike: dist/SPIKE.txt names sha256 ${expected.slice(0, 12)}... but the file ` +
          `hashes to ${info.binary.sha256.slice(0, 12)}...; the provenance is dropped`,
      );
    } else if (expected) {
      info.binary.verified = true;
    }
  }
}

if (!info.binary) {
  console.log("spike: no built Bench-spike-*.g3a anywhere; the page will offer the patch only");
}

writeFileSync(infoPath, JSON.stringify(info, null, 2) + "\n");
