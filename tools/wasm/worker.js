// worker.js - the Bench add-in, running off the page's main thread.
//
// The WebAssembly build (tools/wasm/build.sh) is the add-in and the PC
// stand-in from tools/sim, compiled together.  Its getkey() blocks, and that is
// fine here: this file is a Web Worker, so blocking a worker costs nothing.  A
// SharedArrayBuffer carries keys in and frames out:
//
//   header, Int32Array on the shared buffer
//     [0] keySeq    bumped by the page on every press; Atomics.wait waits on it
//     [1] key       the gint key code for that press
//     [2] frameSeq  bumped by this worker after it has written a frame
//     [3] width     framebuffer width in pixels
//     [4] height    framebuffer height in pixels
//   pixels, Uint8ClampedArray right after the header: RGBA, top row first
//
// No Asyncify, no emscripten runtime, and the worker never returns to the page
// mid-call: the only cross-thread calls are Atomics.
//
// Messages in:  { type: "start", sab, wasm, demo, shim, width, height }
// Messages out: { type: "ready", width, height } | { type: "failed", error }

/* global SharedArrayBuffer */

let hdr = null;
let pixels = null;

function publishFrame(module) {
  const width = hdr[3];
  const height = hdr[4];
  const bytes = width * height * 4;
  const ptr = module.exports.malloc(bytes);

  module.exports.bench_wasm_frame(ptr);
  pixels.set(new Uint8Array(module.exports.memory.buffer, ptr, bytes));
  module.exports.free(ptr);

  Atomics.store(hdr, 2, Atomics.load(hdr, 2) + 1);
  Atomics.notify(hdr, 2);
}

// The one import the module has beyond WASI: run until a key arrives.  Called
// from inside bench_wasm_run(), which is parked in the add-in's event loop.
function waitKey(module) {
  publishFrame(module);

  const seen = Atomics.load(hdr, 0);
  // Blocks this worker thread until the page bumps keySeq.  Spurious wakes are
  // impossible with Atomics.wait, but a loop costs nothing and survives a 0.
  while (Atomics.load(hdr, 0) === seen) {
    Atomics.wait(hdr, 0, seen);
  }
  return Atomics.load(hdr, 1);
}

async function boot({ sab, wasm, demo, shim, width, height }) {
  hdr = new Int32Array(sab, 0, 8);
  hdr[3] = width;
  hdr[4] = height;
  pixels = new Uint8ClampedArray(sab, 32, width * height * 4);

  const { WASI, File, Directory, OpenFile, PreopenDirectory } = await import(shim);

  // The add-in expects notes/, cards/, study/ under its root.  The wasm has no
  // real disk, so the demo content the build put next to it goes in here; the
  // add-in then sees a calculator with something on it, not empty folders.
  const demoFiles = await (await fetch(demo)).json();
  const tree = new Map();
  for (const [path, b64] of Object.entries(demoFiles)) {
    const [folder, name] = path.split("/");
    if (!tree.has(folder)) tree.set(folder, new Directory(new Map()));
    const raw = atob(b64);
    const bytes = new Uint8Array(raw.length);
    for (let i = 0; i < raw.length; i++) bytes[i] = raw.charCodeAt(i);
    tree.get(folder).contents.set(name, new File([...bytes]));
  }

  const fds = [
    new OpenFile(new File([])), // stdin
    new OpenFile(new File([])), // stdout
    new OpenFile(new File([])), // stderr
    new PreopenDirectory("/benchfs", tree),
  ];
  const wasi = new WASI([], ["BENCH_FS=/benchfs"], fds, { debug: false });

  // arrayBuffer() rather than compileStreaming(): 300 KiB compiles instantly,
  // and the same file then runs under Node, where there is no stream to compile
  // from.  tools/wasm/worker-smoke.mjs does exactly that.
  const module = await WebAssembly.compile(await (await fetch(wasm)).arrayBuffer());
  let instance = null;
  instance = await WebAssembly.instantiate(module, {
    wasi_snapshot_preview1: wasi.wasiImport,
    bench: { wait_key: () => waitKey(instance) },
  });
  wasi.initialize(instance);

  self.postMessage({ type: "ready", width, height });
  instance.exports.bench_wasm_run();
}

self.onmessage = (event) => {
  const message = event.data;

  if (message && message.type === "start") {
    boot(message).catch((err) => {
      self.postMessage({ type: "failed", error: String(err && err.message ? err.message : err) });
    });
  }
};
