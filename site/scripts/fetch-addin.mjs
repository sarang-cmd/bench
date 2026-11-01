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
    const older = existsSync(distDir)
      ? readdirSync(distDir)
          .filter((f) => /^Bench-.+\.g3a$/.test(f))
          .map((f) => ({ f, mtime: statSync(join(distDir, f)).mtimeMs }))
          .sort((a, b) => b.mtime - a.mtime)
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
const info = {
  file: dirFile,
  version: fileVersion,
  expected: repoVersion,
  size: statSync(publicPath).size,
  sha256: createHash("sha256").update(bytes).digest("hex"),
  commit: build.commit || null,
  date: build.built || null,
  source,
};
writeFileSync(infoPath, JSON.stringify(info, null, 2) + "\n");

console.log(
  `download: ${dirFile}, ${Math.round(info.size / 1024)} KiB, sha256 ${info.sha256.slice(0, 12)}...` +
    (info.commit ? `, commit ${info.commit.slice(0, 7)}` : "") +
    (info.date ? `, built ${info.date}` : ""),
);
