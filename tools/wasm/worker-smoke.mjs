// worker-smoke.mjs - drive the page's worker protocol end to end.
//
//   node tools/wasm/worker-smoke.mjs [keys]      keys: "down,exe,f2" (default)
//
// Same three parts as the browser: a SharedArrayBuffer laid out the way
// worker.js documents, a worker thread running worker.js, and a visitor
// pressing keys.  It fails if the module does not boot, if a press produces no
// new frame, or if the frame is blank.  Each frame is written to
// /tmp/bench-worker-<n>.ppm for the eye.
import { Worker } from "node:worker_threads";
import { writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = resolve(here, "../..");
const sim = join(root, "site/public/sim");

const KEYS = {
  up: 0x86, down: 0x75, left: 0x85, right: 0x76, exe: 0x15, exit: 0x74,
  menu: 0x84, del: 0x44, dot: 0x12, neg: 0x14, alpha: 0x71,
  f1: 0x91, f2: 0x92, f3: 0x93, f4: 0x94, f5: 0x95, f6: 0x96,
  0: 0x11, 1: 0x21, 2: 0x22, 3: 0x23, 4: 0x31, 5: 0x32, 6: 0x33,
  7: 0x41, 8: 0x42, 9: 0x43,
};

const WIDTH = 396;
const HEIGHT = 224;
const HEADER_BYTES = 32;
const keys = (process.argv[2] ?? "down,exe,f2").split(",").filter(Boolean);

const sab = new SharedArrayBuffer(HEADER_BYTES + WIDTH * HEIGHT * 4);
const header = new Int32Array(sab, 0, 8);
const pixels = new Uint8ClampedArray(sab, HEADER_BYTES, WIDTH * HEIGHT * 4);
header[3] = WIDTH;
header[4] = HEIGHT;

const fileUrl = (name) => new URL(`file://${join(sim, name)}`).href;

const worker = new Worker(new URL("./test/worker-host.mjs", import.meta.url), {
  workerData: {
    message: {
      type: "start",
      sab,
      wasm: fileUrl("bench.wasm"),
      starterpack: fileUrl("starterpack.json"),
      shim: fileUrl("browser_wasi_shim/index.js"),
      width: WIDTH,
      height: HEIGHT,
    },
  },
});

let frameNo = 0;
let failures = 0;
let lastDigest = 0;

function writeFrame() {
  const out = Buffer.alloc(WIDTH * HEIGHT * 3);
  for (let i = 0, j = 0; i < pixels.length; i += 4, j += 3) {
    out[j] = pixels[i];
    out[j + 1] = pixels[i + 1];
    out[j + 2] = pixels[i + 2];
  }
  const name = `/tmp/bench-worker-${String(frameNo).padStart(2, "0")}.ppm`;
  writeFileSync(name, Buffer.concat([Buffer.from(`P6\n${WIDTH} ${HEIGHT}\n255\n`), out]));
  frameNo++;
  return name;
}

// How much of the frame is not its most common colour: a blank frame is a
// module that did nothing, which must not pass as a screenshot.  The paper
// colour comes back from 16-bit colour, so it is not the source constant;
// the modal pixel is the honest baseline.
function inkPercent() {
  const counts = new Map();
  for (let i = 0; i < pixels.length; i += 4) {
    const key = (pixels[i] << 16) | (pixels[i + 1] << 8) | pixels[i + 2];
    counts.set(key, (counts.get(key) ?? 0) + 1);
  }
  let modal = 0;
  for (const n of counts.values()) modal = Math.max(modal, n);
  return ((WIDTH * HEIGHT - modal) / (WIDTH * HEIGHT)) * 100;
}

// FNV-1a over the pixels: cheap, and only compared with itself.
function hashFrame() {
  let hash = 0x811c9dc5;
  for (let i = 0; i < pixels.length; i++) {
    hash ^= pixels[i];
    hash = Math.imul(hash, 0x01000193) >>> 0;
  }
  return hash;
}

function waitForFrame(from, timeoutMs = 20000) {
  const deadline = Date.now() + timeoutMs;
  const sleeper = new Int32Array(new SharedArrayBuffer(4));

  while (Date.now() < deadline) {
    const seq = Atomics.load(header, 2);
    if (seq !== from) return seq;
    Atomics.wait(sleeper, 0, 0, 5);
  }
  return -1;
}

worker.on("message", (message) => {
  if (message.type === "failed") {
    console.error(`worker-smoke: the module failed: ${message.error}`);
    process.exit(1);
  }
  if (message.type !== "ready") return;

  console.log(`worker-smoke: ready at ${message.width}x${message.height}`);

  let seen = waitForFrame(Atomics.load(header, 2));
  if (seen < 0) {
    console.error("worker-smoke: no frame published on start-up");
    process.exit(1);
  }
  const ink = inkPercent();
  console.log(`worker-smoke: start-up ${writeFrame()}, ${ink.toFixed(1)}% ink`);
  lastDigest = hashFrame();
  if (ink < 2) {
    console.error("worker-smoke: the start-up frame is blank");
    failures++;
  }

  for (const name of keys) {
    const code = KEYS[name];
    if (code === undefined) {
      console.error(`worker-smoke: unknown key '${name}'`);
      failures++;
      continue;
    }
    Atomics.store(header, 1, code);
    Atomics.add(header, 0, 1);
    Atomics.notify(header, 0);

    const next = waitForFrame(seen);
    if (next < 0) {
      console.error(`worker-smoke: no frame after '${name}'`);
      failures++;
      break;
    }
    seen = next;
    const file = writeFrame();
    const digest = hashFrame();
    console.log(`worker-smoke: ${name} -> ${file}, ${inkPercent().toFixed(1)}% ink`);
    // A press that leaves the screen identical is a key that never arrived.
    if (digest === lastDigest) {
      console.error(`worker-smoke: '${name}' did not change the screen`);
      failures++;
    }
    lastDigest = digest;
  }

  worker.terminate();
  if (failures) {
    console.error(`worker-smoke: FAILED (${failures})`);
    process.exit(1);
  }
  console.log(`worker-smoke: PASSED (${frameNo} frames)`);
  process.exit(0);
});

worker.on("error", (err) => {
  console.error(`worker-smoke: ${err.message}`);
  process.exit(1);
});
