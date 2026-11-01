// worker-host.mjs - runs ../worker.js inside a Node worker_thread.
//
// Only the browser furniture is stubbed: `self`, postMessage, atob and fetch
// over file URLs.  The worker code itself, the WebAssembly module and the
// SharedArrayBuffer protocol are the ones the page uses, so
// tools/wasm/worker-smoke.mjs is a real test of all three without a browser.
import { parentPort, workerData } from "node:worker_threads";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";

globalThis.self = globalThis;
globalThis.postMessage = (message) => parentPort.postMessage(message);
globalThis.atob = (b64) => Buffer.from(b64, "base64").toString("latin1");
globalThis.fetch = async (url) => {
  const bytes = readFileSync(fileURLToPath(url));
  return {
    ok: true,
    arrayBuffer: async () =>
      bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength),
    json: async () => JSON.parse(bytes.toString("utf8")),
  };
};

// worker.js sets self.onmessage once it has loaded; hold the start message
// until then, the way the page does by posting after the worker exists.
let handler = null;
const queued = [];
Object.defineProperty(globalThis, "onmessage", {
  set(fn) {
    handler = fn;
    for (const message of queued.splice(0)) fn({ data: message });
  },
  get() {
    return handler;
  },
  configurable: true,
});

await import("../worker.js");

if (handler) handler({ data: workerData.message });
else queued.push(workerData.message);
