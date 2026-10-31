// Puts Bench-<version>.g3a in public/ so the download button serves the add-in
// from this site instead of sending anyone to GitHub.
//
// The file is not committed: CI builds it, attaches it to the GitHub release,
// and the deploy fetches the same bytes from that release. A local build with
// the file already in public/ skips the network entirely.
import { existsSync, mkdirSync, readFileSync, statSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const data = readFileSync(join(root, "src/data.js"), "utf8");
const version = data.match(/VERSION = "([^"]+)"/)?.[1];
if (!version) throw new Error("no VERSION in src/data.js");

const name = `Bench-${version}.g3a`;
const target = join(root, "public", name);

if (existsSync(target)) {
  console.log(`download: using ${name} already in public/ (${Math.round(statSync(target).size / 1024)} KiB)`);
  process.exit(0);
}

const url = `https://github.com/sarang-cmd/bench/releases/download/v${version}/${name}`;
console.log(`download: fetching ${url}`);

const response = await fetch(url, { redirect: "follow" });
if (!response.ok) {
  console.error(`download: could not fetch ${name} (HTTP ${response.status})`);
  console.error("Build the release first, or drop the file into site/public/.");
  process.exit(1);
}

const bytes = Buffer.from(await response.arrayBuffer());
// A .g3a is a SuperH ELF behind a Casio header; a HTML error page is the usual
// way this goes wrong, so check what actually arrived.
if (bytes.length < 10_000 || bytes.subarray(0, 2000).includes(Buffer.from("<!DOCTYPE"))) {
  console.error(`download: ${name} does not look like an add-in (${bytes.length} bytes)`);
  process.exit(1);
}

mkdirSync(dirname(target), { recursive: true });
writeFileSync(target, bytes);
console.log(`download: wrote public/${name} (${Math.round(bytes.length / 1024)} KiB)`);
