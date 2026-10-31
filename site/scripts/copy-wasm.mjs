// Copies the WebAssembly build, when there is one, into public/sim.
//
//   node scripts/copy-wasm.mjs
//
// The binary is produced by tools/wasm/build.sh (or by the optional CI job
// "WebAssembly simulator").  It is not in git: it is a build artifact, and the
// repository must stay buildable without emscripten.  When it is missing this
// script removes any stale copy, and the site's simulator page falls back to
// the screenshot gallery - which is why it prints what it did.
import { copyFileSync, existsSync, mkdirSync, readdirSync, rmSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const source = resolve(here, "../../build-wasm");
const target = resolve(here, "../public/sim");

rmSync(target, { recursive: true, force: true });

if (!existsSync(join(source, "bench.js"))) {
  console.log("no wasm build (build-wasm/bench.js); the simulator page shows the gallery");
  process.exit(0);
}

mkdirSync(target, { recursive: true });
for (const name of readdirSync(source)) {
  if (!/\.(js|wasm|data)$/.test(name)) continue;
  copyFileSync(join(source, name), join(target, name));
}
console.log(`copied the wasm build from ${source} to site/public/sim`);
