// Puts Bench-<version>.g3a in public/ so the download button serves the add-in
// from this site instead of sending anyone to GitHub, and writes
// src/build-info.generated.json so the page can name the build it is handing out.
//
// Where the bytes come from, in order:
//
//   1. public/Bench-<version>.g3a, if it is already there (a local build reuses it)
//   2. dist/ in this repository - the copy CI refreshes on every tagged and
//      automatic build, with its BUILD.txt recording the commit and the checksum.
//      This is the newest green build and needs no network at all.
//   3. the rolling `auto` release, which CI rebuilds every few days
//   4. the tagged release `v<version>`
//
// The version is read from version.txt, not from the page, so the file name and
// the version on the page cannot disagree.
import { createHash } from "node:crypto";
import { copyFileSync, existsSync, mkdirSync, readFileSync, statSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const repoVersion = readFileSync(join(root, "..", "version.txt"), "utf8").trim();
const name = `Bench-${repoVersion}.g3a`;
const target = join(root, "public", name);
const distDir = join(root, "..", "dist");
const infoPath = join(root, "src", "build-info.generated.json");

function fail(message, hint) {
  console.error(`download: ${message}`);
  if (hint) console.error(hint);
  process.exit(1);
}

function looksLikeAddin(bytes) {
  return (
    bytes.length > 10_000 &&
    !bytes.subarray(0, 2000).includes(Buffer.from("<!DOCTYPE"))
  );
}

// BUILD.txt is written by CI next to the file it describes; missing values are
// left out rather than invented.
function provenance() {
  const file = join(distDir, "BUILD.txt");
  const out = {};
  if (!existsSync(file)) return out;
  for (const line of readFileSync(file, "utf8").split("\n")) {
    const [key, ...rest] = line.trim().split(" ");
    if (["commit", "built", "sha256", "version"].includes(key)) out[key] = rest.join(" ");
  }
  return out;
}

mkdirSync(dirname(infoPath), { recursive: true });

let bytes;
let source;

if (existsSync(target)) {
  bytes = readFileSync(target);
  source = "public";
  console.log(`download: using ${name} already in public/ (${Math.round(bytes.length / 1024)} KiB)`);
} else if (existsSync(join(distDir, name))) {
  copyFileSync(join(distDir, name), target);
  bytes = readFileSync(target);
  source = "dist";
  console.log(`download: took ${name} from dist/ (${Math.round(bytes.length / 1024)} KiB)`);
} else {
  const urls = [
    `https://github.com/sarang-cmd/bench/releases/download/auto/${name}`,
    `https://github.com/sarang-cmd/bench/releases/download/v${repoVersion}/${name}`,
  ];

  for (const url of urls) {
    console.log(`download: fetching ${url}`);
    const response = await fetch(url, { redirect: "follow" });
    if (!response.ok) {
      console.log(`download: ${response.status} from ${url}`);
      continue;
    }
    bytes = Buffer.from(await response.arrayBuffer());
    if (!looksLikeAddin(bytes)) {
      console.log(`download: ${url} did not return an add-in (${bytes.length} bytes)`);
      continue;
    }
    source = url.includes("/auto/") ? "auto" : "release";
    mkdirSync(join(root, "public"), { recursive: true });
    writeFileSync(target, bytes);
    break;
  }

  if (!bytes) {
    fail(`could not fetch ${name}`,
         "Build the release first, or drop the file into site/public/ or dist/.");
  }
}

if (!looksLikeAddin(bytes)) fail(`${name} does not look like an add-in (${bytes.length} bytes)`);

const build = provenance();
const info = {
  file: name,
  version: repoVersion,
  size: statSync(target).size,
  sha256: createHash("sha256").update(bytes).digest("hex"),
  commit: build.commit || null,
  date: build.built || null,
  source,
};
writeFileSync(infoPath, JSON.stringify(info, null, 2) + "\n");

console.log(
  `download: ${name}, ${Math.round(info.size / 1024)} KiB, sha256 ${info.sha256.slice(0, 12)}...` +
    (info.commit ? `, commit ${info.commit.slice(0, 7)}` : "") +
    (info.date ? `, built ${info.date}` : ""),
);
