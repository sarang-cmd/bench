// Copies the screenshots the add-in generated (docs/images) into the site's
// public folder. Run before dev and build; the PNGs are never duplicated in
// git, they keep one home.
import { copyFileSync, existsSync, mkdirSync, readdirSync, rmSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const source = resolve(here, "../../docs/images");
const target = resolve(here, "../public/images");

if (!existsSync(source)) {
  console.error(`no screenshots at ${source} - run "make shots" first`);
  process.exit(1);
}

rmSync(target, { recursive: true, force: true });
mkdirSync(target, { recursive: true });
// Only pictures the browser can use; the simulator's scratch frames stay out.
for (const name of readdirSync(source)) {
  if (!/\.(png|svg|jpe?g|webp|ico)$/i.test(name)) continue;
  copyFileSync(resolve(source, name), resolve(target, name));
}
console.log("copied screenshots from docs/images to site/public/images");
