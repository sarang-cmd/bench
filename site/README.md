# Website

React 18 + Vite + Tailwind CSS. Neobrutalist on purpose: flat colour, 3 px black
outlines, hard offset shadows, no gradients. Deployed on Vercel from this
directory (`root_directory: site`).

```sh
npm install
npm run dev      # http://localhost:5173, bound to 0.0.0.0
npm run build    # -> dist/, what Vercel serves
npm run preview  # serve dist/ on port 4173
```

## The screenshots

`docs/images/` is their only home: the add-in's own code draws each frame
(`make sim && make shots` in the repository root). `scripts/copy-images.mjs`
copies them into `public/images/` before every dev run and build, so the site
cannot drift from the code.

`scripts/check-images.mjs` fails the build if the page names a picture that is
not in `docs/images/`. It exists because v1 of the page asked for
`24_numbertheory_gcd.png` while the file is `24_numtheory_gcd.png`, and the
Number theory screenshot silently 404'd.

## The add-in the page hands out

The download button serves `Bench-<version>.g3a` from this site, not from a
GitHub page. `scripts/fetch-addin.mjs` (first step of `npm run build`) takes the
file from the release CI made and writes it into `public/`; it is not committed,
and with the file already there the build skips the network.

If that script fails, the release does not exist yet: build it with a `v*` tag,
then deploy again.

## The tool list comes from the add-in

`src/apps/registry.c` is the only list of tools in the project; CI turns it into
`docs/tools.json`. `scripts/gen-tools.mjs` (first step of `npm run dev` and
`npm run build`) joins that file to the copy and screenshots in `src/toolCopy.js`
and writes `src/tools.generated.json`, which the page imports.

It fails the build when:

- a tool in the registry has no copy, so the page would quietly describe an
  older, smaller app
- `src/toolCopy.js` describes a tool the add-in no longer has
- a screenshot named by the copy is not in `docs/images/`

The counts on the page (tools, screenshots, the word in the headline) all come
from that generated file, so "five tools" cannot survive a sixth tool.

## Content

Copy and the version live in `src/data.js` and `src/toolCopy.js`; sections are in
`src/components/`. Only classes Tailwind can see in the source get emitted, so
accent colours come from the static maps in `src/ui.jsx`, never from a template
string.

## Checking it

Rendered in a headless Chromium at 320, 390, 834 and 1440 px: no horizontal
overflow, every lazy image decoded, no console errors.
