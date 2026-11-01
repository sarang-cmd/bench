// check-band.mjs - does the yellow band behind "in one" cover anything that is
// not its own text?
//
//   node scripts/check-band.mjs [--url http://localhost:4173]
//
// Measured, not argued. The band's own glyphs are hidden but its box stays
// where it is, its background is made transparent, and the pixels inside that
// box are counted: every pixel that is not paper is a neighbour's ink sitting
// where the yellow will be. That is the bug the eye sees as the band cutting
// the tail of the g in "eighteen" - a bounding-box check cannot see it, because
// the boxes never touch; the glyph overflows its box.
//
// It also prints the band's height and its clearance above and below, so a
// change can be judged rather than guessed at.
//
// Chromium comes from Playwright, which the width sweep already uses; in CI the
// same step runs both. Set CHROME to a browser binary to run it here, where
// `npx playwright install` cannot reach the download host.
import { chromium } from "playwright";

const args = process.argv.slice(2);
const url = args.includes("--url") ? args[args.indexOf("--url") + 1] : "http://localhost:4173";

const WIDTHS = [320, 360, 390, 768, 1024, 1280, 1440];
const PAPER = [244, 243, 238];

const browser = await chromium.launch({
  executablePath: process.env.CHROME || undefined,
  args: ["--no-sandbox"],
});

let failures = 0;

try {
  for (const width of WIDTHS) {
    const page = await browser.newPage({
      viewport: { width, height: 900 },
      deviceScaleFactor: 2,
    });
    await page.goto(url, { waitUntil: "networkidle" });

    const box = await page.evaluate(() => {
      const h1 = document.querySelector("h1");
      const band = h1 && [...h1.querySelectorAll("span")].find((s) => s.textContent.trim() === "in one");
      if (!band) return null;
      const b = band.getBoundingClientRect();
      // Its own glyphs out of the way; the box stays exactly where it is.
      band.style.visibility = "hidden";
      band.style.backgroundColor = "transparent";
      return { x: b.x, y: b.y, width: b.width, height: b.height };
    });

    if (!box) {
      console.log(`  FAIL ${width}px: no band in the heading`);
      failures++;
      await page.close();
      continue;
    }

    const png = await page.screenshot({
      clip: { x: box.x, y: box.y, width: Math.max(1, box.width), height: Math.max(1, box.height) },
    });
    const ink = await page.evaluate(
      async ({ data, paper }) => {
        const bytes = new Uint8Array(data);
        const blob = new Blob([bytes], { type: "image/png" });
        const bitmap = await createImageBitmap(blob);
        const canvas = new OffscreenCanvas(bitmap.width, bitmap.height);
        const ctx = canvas.getContext("2d");
        ctx.drawImage(bitmap, 0, 0);
        const pixels = ctx.getImageData(0, 0, bitmap.width, bitmap.height).data;
        let count = 0;
        for (let i = 0; i < pixels.length; i += 4) {
          const near =
            Math.abs(pixels[i] - paper[0]) < 26 &&
            Math.abs(pixels[i + 1] - paper[1]) < 26 &&
            Math.abs(pixels[i + 2] - paper[2]) < 26;
          if (!near) count++;
        }
        return count;
      },
      { data: [...png], paper: PAPER },
    );

    const geo = await page.evaluate(() => {
      const h1 = document.querySelector("h1");
      const band = [...h1.querySelectorAll("span")].find((s) => s.textContent.trim() === "in one");
      band.style.visibility = "";
      const b = band.getBoundingClientRect();
      const range = document.createRange();
      range.selectNodeContents(h1);
      const lines = [...range.getClientRects()].filter((r) => r.height > 4);
      const above = lines.filter((r) => r.bottom <= b.top + 1).map((r) => r.bottom);
      const below = lines.filter((r) => r.top >= b.bottom - 1).map((r) => r.top);
      return {
        bandHeight: Math.round(b.height),
        gapAbove: above.length ? Math.round(b.top - Math.max(...above)) : null,
        gapBelow: below.length ? Math.round(Math.min(...below) - b.bottom) : null,
      };
    });

    if (ink > 0 && process.env.BAND_DUMP) {
      const { writeFileSync } = await import("node:fs");
      writeFileSync(`${process.env.BAND_DUMP}-${width}.png`, png);
      console.log(`  wrote ${process.env.BAND_DUMP}-${width}.png`);
    }
    const ok = ink === 0;
    if (!ok) failures++;
    console.log(
      `  ${ok ? "ok  " : "FAIL"} ${String(width).padStart(4)}px  band ${geo.bandHeight}px  ` +
        `covered ink ${ink}px  gap above ${geo.gapAbove}px, below ${geo.gapBelow}px`,
    );
    await page.close();
  }
} finally {
  await browser.close();
}

if (failures) {
  console.log(`BAND CHECKS FAILED (${failures})`);
  process.exit(1);
}
console.log(`BAND CHECKS PASSED (${WIDTHS.length} widths: the band covers nothing but itself)`);
