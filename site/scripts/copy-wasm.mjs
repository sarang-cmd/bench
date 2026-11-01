// copy-wasm.mjs - put the WebAssembly build where the site can serve it.
//
//   node scripts/copy-wasm.mjs
//
// Two sources, in order:
//
//   1. build-wasm/ - a build made here with tools/wasm/build.sh.  The .wasm is
//      small enough to keep in the repository, so what it produces is committed
//      to site/public/sim/ and the deployed site serves a working simulator
//      without a browser-side build step.
//   2. site/public/sim/ already holding a binary - nothing to do, it is served
//      as it is.  This is the normal case on Vercel, where nothing compiles C.
//
// The worker and the WASI shim come from tools/wasm/ every time: they are hand-
// written or vendored, not build output, so they must not go stale.
import { copyFileSync, existsSync, mkdirSync, readdirSync, statSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const repo = resolve(here, "../..");
const buildDir = join(repo, "build-wasm");
const target = join(here, "../public/sim");
const shimDir = join(repo, "tools/wasm/vendor/browser_wasi_shim");

mkdirSync(target, { recursive: true });

// The worker, and the shim it imports.  Always refreshed.
copyFileSync(join(repo, "tools/wasm/worker.js"), join(target, "worker.js"));
mkdirSync(join(target, "browser_wasi_shim"), { recursive: true });
for (const name of readdirSync(shimDir)) {
  if (!/\.js$/.test(name)) continue;
  copyFileSync(join(shimDir, name), join(target, "browser_wasi_shim", name));
}

const fresh = existsSync(join(buildDir, "bench.wasm"));
let served = join(target, "bench.wasm");
let source = "public/sim (committed)";

if (fresh) {
  copyFileSync(join(buildDir, "bench.wasm"), served);
  copyFileSync(join(buildDir, "demo.json"), join(target, "demo.json"));
  source = "build-wasm (just built)";
}

if (!existsSync(served)) {
  // No binary anywhere: the simulator page says so in one line and shows the
  // screenshots instead of a canvas.
  console.log("no wasm build; the simulator page will show the gallery");
  process.exit(0);
}

const size = Math.round(statSync(served).size / 1024);
console.log(`sim: ${source} -> site/public/sim (${size} KiB, worker + WASI shim refreshed)`);
