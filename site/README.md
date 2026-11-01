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
GitHub page. `scripts/fetch-addin.mjs` runs before every build and takes the
newest build it can find, in this order:

1. `public/Bench-<version>.g3a`, if a local build already put it there
2. `dist/` in this repository, which CI refreshes on every tagged and automatic
   build and which records its commit, date and SHA-256 in `dist/BUILD.txt`
3. the rolling `auto` release, rebuilt every few days
4. the tagged release `v<version>`

The version comes from `version.txt`, never from the page, so the file name and
the version shown cannot disagree. The script also writes
`src/build-info.generated.json`; the page shows the commit, the date, the size
and the SHA-256 from it, so the bytes behind the button can be checked.

If all four fail the build stops: a page that offers a download it does not have
is worse than a page that refuses to build.

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

## When the site deploys

`vercel.json` sets an ignored build step, `scripts/should-deploy.mjs`, so a
commit to `main` does not rebuild and re-deploy the site every time. It deploys
when either:

- the commit message contains `[deploy]` - the manual switch; or
- at least 20 commits have landed since the last deployment
  (`VERCEL_GIT_PREVIOUS_SHA`).

Pull request previews are unaffected. To publish now: put `[deploy]` in a commit
message, or trigger a deployment from the Vercel dashboard.
