// Render every route on the server, in Node, so a page that throws or forgets a
// field fails a command instead of failing in someone's browser.  This is not a
// test of layout - the eye does that - it is a test that the page exists.
//
//   node scripts/check-pages.mjs
//
// Vite compiles the JSX for us (ssrLoadModule), react-dom/server draws it, and
// each route must contain the words it promises.
import { createServer } from "vite";
import { renderToString } from "react-dom/server";
import { createElement } from "react";
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const root = dirname(dirname(fileURLToPath(import.meta.url)));

const ROUTES = [
  ["/", ["Bench", "tools"]],
  ["/features", ["tool"]],
  ["/simulator", ["WebAssembly"]],
  ["/tutorial", ["home screen"]],
  ["/install", ["USB Flash mode"]],
  ["/downloads", ["SHA-256"]],
  ["/changelog", ["1.1.0"]],
  ["/about", ["unverified on device"]],
];

// The router reads window.location, and the meta tags read document.head.  On a
// server neither exists, so each route gets a stand-in for the two.
function routeGlobals(path) {
  const head = [];
  globalThis.window = {
    location: { pathname: path },
    history: { pushState() {} },
    addEventListener() {},
    removeEventListener() {},
    dispatchEvent() {},
    scrollTo() {},
  };
  globalThis.document = {
    head: {
      querySelector: () => null,
      appendChild: (tag) => head.push(tag),
    },
    createElement: (name) => ({ name, setAttribute() {} }),
    title: "",
  };
  globalThis.PopStateEvent = class {};
}

const server = await createServer({ server: { middlewareMode: true }, appType: "custom" });
let failures = 0;

try {
  const { default: App } = await server.ssrLoadModule("/src/App.jsx");

  for (const [path, phrases] of ROUTES) {
    routeGlobals(path);
    let html = "";
    try {
      html = renderToString(createElement(App));
    } catch (error) {
      failures++;
      console.log(`  FAIL ${path}: threw ${error.message}`);
      continue;
    }
    const text = html.replace(/<[^>]+>/g, " ").replace(/\s+/g, " ");
    const missing = phrases.filter((phrase) => !text.toLowerCase().includes(phrase.toLowerCase()));
    if (missing.length) {
      failures++;
      console.log(`  FAIL ${path}: no ${missing.join(", ")}`);
    } else {
      console.log(`  ok   ${path} (${text.trim().length} chars)`);
    }
  }
  // The browser simulator is only allowed to exist on a cross-origin isolated
  // page, so the deployment has to send the two headers on every route.  A
  // vercel.json edit that drops them would fail silently in the browser
  // (SharedArrayBuffer undefined, the gallery instead of the add-in), which is
  // exactly the kind of thing to catch here.
  const vercel = JSON.parse(readFileSync(join(root, "vercel.json"), "utf8"));
  const global = (vercel.headers ?? []).find((rule) => rule.source === "/(.*)");
  const served = new Set((global?.headers ?? []).map((h) => h.key));
  for (const key of ["Cross-Origin-Opener-Policy", "Cross-Origin-Embedder-Policy"]) {
    if (!served.has(key)) {
      failures++;
      console.log(`  FAIL vercel.json: no ${key} on every route`);
    }
  }
  console.log(`  ok   vercel.json sends the isolation headers`);
} finally {
  await server.close();
}

if (failures) {
  console.log(`PAGE CHECKS FAILED (${failures})`);
  process.exit(1);
}
console.log(`PAGE CHECKS PASSED (${ROUTES.length} routes)`);
