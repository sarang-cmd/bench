import { BUILD, TOOL_COUNT, TOOL_COUNT_WORD } from "../data.js";
import { BruteButton, Chip, Screen } from "../ui.jsx";

// Counted from the registry, never typed: adding a tool changes this number.
const stats = () => [
  [String(TOOL_COUNT), "tools in one menu"],
  ["384\u00d7216", "pixels of screen"],
  ["0", "extra dependencies"],
];

export default function Hero({ version }) {
  return (
    <div id="top" className="relative overflow-hidden border-b-3 border-ink bg-paper">
      {/* Flat blocks, no gradients: the house style. They sit behind everything
          and only appear once there is room for them, so they never cross the
          text on a phone. */}
      {/* Kept clear of the right edge: a rotated box reports the corners of its
          bounding box, and `site/scripts/check-widths.mjs` fails a rectangle
          that crosses the viewport even when nothing is painted there. */}
      <div aria-hidden="true" className="pointer-events-none absolute inset-0 hidden lg:block">
        <div className="absolute -top-20 right-12 h-56 w-56 rotate-12 border-3 border-ink bg-yellow/60" />
      </div>

      <div className="relative mx-auto grid w-full max-w-6xl gap-10 px-5 py-10 sm:px-8 sm:py-14 lg:grid-cols-[1.05fr_1fr] lg:items-center lg:gap-14 lg:px-12 lg:py-16">
        <div>
          <div className="mb-5 flex flex-wrap items-center gap-2">
            <Chip tone="ink">Casio fx-CG 50</Chip>
            <Chip>OS 3.81</Chip>
            <Chip>MIT</Chip>
          </div>

          {/* The yellow band is exactly the words it sits behind, and nothing
              about the heading's settings may push it onto its neighbours:

              - tracking-normal inside the band. The heading is tracked tight
                (-0.025em), so each glyph paints a little past its own advance;
                a band laid out with tight tracking therefore starts life a few
                pixels left of the words it is meant to cover, and at heading
                sizes that is enough for it to sit on the end of the word
                before it.
              - no vertical padding, and leading 1.3. The band is the glyph
                box, and the line box has the spare height, so it cannot reach
                into the line above or below.
              - padding only to the sides, which the words inside the band can
                never collide with. */}
          <h1 className="text-balance text-[2rem] font-black leading-[1.3] tracking-tight sm:text-5xl lg:text-[3.25rem] xl:text-6xl">
            {TOOL_COUNT_WORD} tools{" "}
            <span className="whitespace-nowrap bg-yellow px-2 tracking-normal [box-decoration-break:clone]">
              in one
            </span>{" "}
            add-in.
          </h1>

          <p className="mt-5 max-w-xl text-base leading-relaxed text-ink/80 sm:mt-6 sm:text-lg">
            Pacer times a practice paper, Uncertainty does the error arithmetic,
            Cards drills flashcards and Notes reads Markdown. Behind them: an
            expression engine, a root finder, complex numbers, statistics and
            number theory to CRT. One{" "}
            <code className="font-mono text-[0.9em]">.g3a</code>, one menu,
            nothing else to install.
          </p>

          <div className="mt-7 flex flex-col gap-3 sm:flex-row sm:items-center">
            <BruteButton href="#install" extra="w-full sm:w-auto">
              Get Bench {version}
            </BruteButton>
            <BruteButton href="#tour" tone="paper" extra="w-full sm:w-auto">
              See every screen &rarr;
            </BruteButton>
          </div>

          {/* Which build this page is handing out: the file is rebuilt by CI, so
              "version" alone does not say which bytes are behind the button. */}
          <p className="mt-4 break-words font-mono text-[11px] uppercase tracking-widest text-ink/55">
            {BUILD.commit ? `commit ${BUILD.commit.slice(0, 7)}` : "local build"}
            {BUILD.date ? ` \u00b7 ${BUILD.date}` : ""}
            {` \u00b7 ${Math.round(BUILD.size / 1024)} KiB`}
          </p>

          <dl className="mt-8 grid max-w-lg gap-x-6 gap-y-3 border-t-3 border-ink pt-5 sm:grid-cols-3">
            {stats().map(([value, label]) => (
              <div key={label} className="flex items-baseline gap-3 sm:block">
                <dt className="order-2 font-mono text-[11px] uppercase tracking-widest text-ink/60 sm:order-none sm:mt-1 sm:text-xs">
                  {label}
                </dt>
                <dd className="order-1 text-xl font-black sm:text-2xl lg:text-3xl">
                  {value}
                </dd>
              </div>
            ))}
          </dl>
        </div>

        <div className="relative">
          <div
            className="absolute -inset-2 -z-10 translate-x-2 translate-y-2 border-3 border-ink bg-red"
            aria-hidden="true"
          />
          <Screen
            file="01_menu.png"
            alt="The Bench home screen: three tiles a row, a page of the Study category."
          />
          <p className="mt-3 font-mono text-[11px] uppercase tracking-widest text-ink/60">
            The home screen &mdash; a real frame, drawn by the add-in
          </p>
        </div>
      </div>
    </div>
  );
}
