// smoke.mjs - run the WebAssembly build here, in Node, without a browser.
//
//   node tools/wasm/smoke.mjs [keys]        keys: "down,exe,f3" (default shown below)
//
// This is the check that the browser build is a build of the add-in and not
// just a file that exists: it loads bench.wasm with the same WASI shim the page
// uses, puts the starter pack content in the in-memory filesystem, sends a few keys and
// writes the resulting frames as PPM (same format tools/sim writes), which
// tools/sim/ppm2png.py or Pillow can turn into a PNG.
//
// Writes /tmp/bench-wasm-<n>.ppm unless --out is given.
import { readFileSync, writeFileSync, mkdirSync, existsSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

import {
  WASI,
  File,
  Directory,
  OpenFile,
  PreopenDirectory,
} from "./vendor/browser_wasi_shim/index.js";

const here = dirname(fileURLToPath(import.meta.url));
const root = resolve(here, "../..");
const wasmPath = join(root, "build-wasm/bench.wasm");

const args = process.argv.slice(2);
const keyList = (args[0] ?? "down,exe,exit").split(",").filter(Boolean);
const outDir = args.includes("--out") ? args[args.indexOf("--out") + 1] : "/tmp";

// The gint key codes tools/sim/include/gint/gint.h defines.
const KEYS = {
  up: 0x86, down: 0x75, left: 0x85, right: 0x76, exe: 0x15, exit: 0x74,
  menu: 0x84, del: 0x44, dot: 0x12, neg: 0x14, alpha: 0x71,
  f1: 0x91, f2: 0x92, f3: 0x93, f4: 0x94, f5: 0x95, f6: 0x96,
  0: 0x11, 1: 0x21, 2: 0x22, 3: 0x23, 4: 0x31, 5: 0x32, 6: 0x33,
  7: 0x41, 8: 0x42, 9: 0x43,
};

if (!existsSync(wasmPath)) {
  console.error(`smoke: no ${wasmPath}; run tools/wasm/build.sh first`);
  process.exit(1);
}

// The starter pack content the build put next to the wasm, as base64.
const pack = JSON.parse(readFileSync(join(root, "build-wasm/starterpack.json"), "utf8"));

// Build the tree the add-in expects under its root: notes/, cards/, study/.
const tree = new Map();
for (const [path, b64] of Object.entries(pack)) {
  const [folder, name] = path.split("/");
  if (!tree.has(folder)) tree.set(folder, new Directory(new Map()));
  tree.get(folder).contents.set(name, new File([...Buffer.from(b64, "base64")]));
}

const fds = [
  new OpenFile(new File([])), // stdin
  new OpenFile(new File([])), // stdout
  new OpenFile(new File([])), // stderr
  new PreopenDirectory("/benchfs", tree),
];
const wasi = new WASI([], ["BENCH_FS=/benchfs", "BENCH_TRACE=1"], fds, { debug: false });

let instance = null;
let frameNo = 0;
let keyAt = 0;

function dumpFrame(width, height) {
  const bytes = width * height * 4;
  const ptr = instance.exports.malloc(bytes);
  instance.exports.bench_wasm_frame(ptr);
  const pixels = new Uint8Array(instance.exports.memory.buffer, ptr, bytes);
  const out = Buffer.alloc(width * height * 3);
  for (let i = 0, j = 0; i < bytes; i += 4, j += 3) {
    out[j] = pixels[i];
    out[j + 1] = pixels[i + 1];
    out[j + 2] = pixels[i + 2];
  }
  const name = join(outDir, `bench-wasm-${String(frameNo).padStart(2, "0")}.ppm`);
  writeFileSync(name, Buffer.concat([Buffer.from(`P6\n${width} ${height}\n255\n`), out]));
  frameNo++;
  instance.exports.free(ptr);
  return name;
}

// The one import the page must provide.  Here a script plays the visitor.
function waitKey() {
  const width = instance.exports.bench_wasm_width();
  const height = instance.exports.bench_wasm_height();
  const shot = dumpFrame(width, height);

  if (keyAt >= keyList.length) {
    console.log(`smoke: ${keyAt} key(s) sent, ${frameNo} frame(s) (last: ${shot})`);
    process.exit(0);
  }
  const name = keyList[keyAt++];
  const code = KEYS[name];
  if (code === undefined) {
    console.error(`smoke: unknown key '${name}'`);
    process.exit(1);
  }
  console.log(`smoke: ${frameNo - 1} <- ${name} (0x${code.toString(16)})`);
  return code;
}

mkdirSync(outDir, { recursive: true });

const module = await WebAssembly.compile(readFileSync(wasmPath));
instance = await WebAssembly.instantiate(module, {
  wasi_snapshot_preview1: wasi.wasiImport,
  bench: { wait_key: waitKey },
});
// initialize() runs _initialize() for a reactor: constructors only, no main.
wasi.initialize(instance);
instance.exports.bench_wasm_run();
console.log("smoke: the add-in returned");
