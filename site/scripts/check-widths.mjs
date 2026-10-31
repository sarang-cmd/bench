// check-widths.mjs - the browser-width sweep the plan asks for.
//
//   node scripts/check-widths.mjs [--url http://localhost:4173]
//
// Loads every route at 320, 390, 768, 1024 and 1440 pixels wide and fails on:
//   - horizontal scrolling (the page is wider than the viewport)
//   - any element whose box crosses the right edge
//   - any element wider than the viewport
//
// Needs Playwright with Chromium: `npm i -D playwright && npx playwright install
// chromium`.  CI runs it in the "Site build and page checks" job.
import { chromium } from "playwright";

const ROUTES = [
  "/",
  "/features",
  "/simulator",
  "/tutorial",
  "/install",
  "/downloads",
  "/changelog",
  "/about",
];
const WIDTHS = [320, 390, 768, 1024, 1440];
const HEIGHT = 900;

const arg = process.argv.indexOf("--url");
const base = arg > 0 ? process.argv[arg + 1] : "http://localhost:4173";

const browser = await chromium.launch();
let failures = 0;

for (const width of WIDTHS) {
  const page = await browser.newPage({ viewport: { width, height: HEIGHT } });

  for (const route of ROUTES) {
    await page.goto(base + route, { waitUntil: "networkidle" });

    const report = await page.evaluate(() => {
      const problems = [];
      const doc = document.documentElement;
      const viewport = window.innerWidth;

      if (doc.scrollWidth > viewport + 1) {
        problems.push(`page scrolls sideways: ${doc.scrollWidth} > ${viewport}`);
      }

      for (const el of document.querySelectorAll("body *")) {
        const style = getComputedStyle(el);
        if (style.display === "none" || style.visibility === "hidden") continue;
        const box = el.getBoundingClientRect();
        if (box.width === 0 && box.height === 0) continue;
        const name =
          el.tagName.toLowerCase() +
          (el.className && typeof el.className === "string"
            ? "." + el.className.split(/\s+/).filter(Boolean).slice(0, 2).join(".")
            : "");
        if (box.right > viewport + 1)
          problems.push(`${name} crosses the right edge: right=${Math.round(box.right)} of ${viewport}`);
        if (box.width > viewport + 1)
          problems.push(`${name} is wider than the viewport: ${Math.round(box.width)} > ${viewport}`);
      }
      return [...new Set(problems)].slice(0, 6);
    });

    if (report.length) {
      failures++;
      console.log(`  FAIL ${width}px ${route}`);
      for (const line of report) console.log(`       ${line}`);
    } else {
      console.log(`  ok   ${width}px ${route}`);
    }
  }

  await page.close();
}

await browser.close();

if (failures) {
  console.log(`WIDTH CHECKS FAILED (${failures} page/width pairs)`);
  process.exit(1);
}
console.log(`WIDTH CHECKS PASSED (${WIDTHS.length} widths x ${ROUTES.length} routes)`);
