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

// "Bench-1.10.0.g3a" as [1, 10, 0], so 1.10 sorts above 1.9 and 1.2.1 above
// 1.2.0.  Anything unreadable becomes -1 and sinks to the bottom.
function versionKey(file) {
  return file
    .replace(/^Bench-/, "")
    .replace(/\.g3a$/, "")
    .split(".")
    .map((part) => (/^\d+$/.test(part) ? Number(part) : -1));
}

function compareVersions(a, b) {
  for (let i = 0; i < Math.max(a.length, b.length); i++) {
    const left = a[i] ?? 0;
    const right = b[i] ?? 0;
    if (left !== right) return left - right;
  }
  return 0;
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
    if (["commit", "built", "sha256", "version", "file"].includes(key)) out[key] = rest.join(" ");
  }
  return out;
}

mkdirSync(dirname(infoPath), { recursive: true });

let bytes;
let source;
let dirFile = name;      /* the file the page will name, when that is not `name` */
let publicPath = target; /* where the bytes ended up */

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
    /* A network the build cannot reach is a source that is not there, not a
     * reason to fail: the next source may still work. */
    let response;
    try {
      response = await fetch(url, { redirect: "follow" });
    } catch (err) {
      console.log(`download: ${url} unreachable (${err.cause?.code ?? err.message})`);
      continue;
    }
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

  /* 5. the newest build that is already in dist/, whatever version it is.
   *    This is what keeps a deployment working in the window between a version
   *    bump and the first tagged build for it: the page then names the version
   *    of the file it is actually handing out, not the one in version.txt. */
  if (!bytes) {
    /* Newest by version number, not by modification time.  dist/ is a git
     * checkout, so every file in it is "written" at the same moment and their
     * order is whatever the filesystem hands back: sorting by mtime published
     * 1.0.0 while 1.1.0 was sitting right next to it. */
    const older = existsSync(distDir)
      ? readdirSync(distDir)
          .filter((f) => /^Bench-.+\.g3a$/.test(f))
          .map((f) => ({ f, key: versionKey(f) }))
          .sort((a, b) => compareVersions(b.key, a.key))
      : [];

    for (const { f } of older) {
      const candidate = readFileSync(join(distDir, f));
      if (!looksLikeAddin(candidate)) continue;
      bytes = candidate;
      dirFile = f;
      source = "dist-newest";
      mkdirSync(join(root, "public"), { recursive: true });
      copyFileSync(join(distDir, f), join(root, "public", f));
      publicPath = join(root, "public", f);
      console.log(
        `download: no build for ${repoVersion} yet, using ${f} from dist/ ` +
          `(the page will name ${f.replace(/^Bench-|\.g3a$/g, "")})`,
      );
      break;
    }
  }

  if (!bytes) {
    fail(`could not fetch ${name}`,
         "Build the release first, or drop the file into site/public/ or dist/.");
  }
}

if (!looksLikeAddin(bytes)) fail(`${name} does not look like an add-in (${bytes.length} bytes)`);

const build = provenance();
const fileVersion = dirFile === name ? repoVersion : dirFile.replace(/^Bench-/, "").replace(/\.g3a$/, "");
/* BUILD.txt describes one file: the newest build CI wrote.  When the bytes came
 * from an older file in dist/, its commit and date belong to that other build,
 * so they are left out rather than printed against the wrong download. */
if (build.file && build.file !== dirFile) {
  delete build.commit;
  delete build.built;
}
/* The demo zip is optional on purpose: a checkout that has not run
 * `make demo-zip` (or a release that predates 1.2.6) simply has no second card
 * on the downloads page, rather than a broken build. */
/* Prefer the demo for the version being served, then for the version the tree
 * says, then the newest one in dist/: the demo files do not change between
 * patch releases, and a page that hands out 1.2.3 while the demo zip says 1.2.6
 * is still handing out the right notes. */
const demoCandidates = [
  `Bench-demo-${fileVersion}.zip`,
  `Bench-demo-${repoVersion}.zip`,
  ...(existsSync(distDir)
    ? readdirSync(distDir)
        .filter((f) => /^Bench-demo-.+\.zip$/.test(f))
        .map((f) => ({
          f,
          key: f
            .replace(/^Bench-demo-/, "")
            .replace(/\.zip$/, "")
            .split(".")
            .map((part) => (/^\d+$/.test(part) ? Number(part) : -1)),
        }))
        .sort((a, b) => compareVersions(b.key, a.key))
        .map(({ f }) => f)
    : []),
];
const demoName = demoCandidates[0];
const demoSource = demoCandidates
  .flatMap((file) => [join(root, "public", file), join(distDir, file)])
  .find((path) => existsSync(path));
const demoFile = demoSource ? demoSource.split(/[\\/]/).pop() : null;
let demo = null;
if (demoSource) {
  const demoPublic = join(root, "public", demoFile);
  if (demoSource !== demoPublic) {
    mkdirSync(join(root, "public"), { recursive: true });
    copyFileSync(demoSource, demoPublic);
  }
  const demoBytes = readFileSync(demoPublic);
  demo = {
    file: demoFile,
    size: demoBytes.length,
    sha256: createHash("sha256").update(demoBytes).digest("hex"),
  };
}

const info = {
  file: dirFile,
  version: fileVersion,
  expected: repoVersion,
  size: statSync(publicPath).size,
  sha256: createHash("sha256").update(bytes).digest("hex"),
  commit: build.commit || null,
  date: build.built || null,
  source,
  demo,
};
writeFileSync(infoPath, JSON.stringify(info, null, 2) + "\n");

if (demo) console.log(`download: ${demo.file}, ${demo.size} bytes`);

console.log(
  `download: ${dirFile}, ${Math.round(info.size / 1024)} KiB, sha256 ${info.sha256.slice(0, 12)}...` +
    (info.commit ? `, commit ${info.commit.slice(0, 7)}` : "") +
    (info.date ? `, built ${info.date}` : ""),
);
