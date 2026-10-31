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
      <div aria-hidden="true" className="pointer-events-none absolute inset-0 hidden lg:block">
        <div className="absolute -right-24 -top-28 h-72 w-72 rotate-12 border-3 border-ink bg-yellow/60" />
      </div>

      <div className="relative mx-auto grid w-full max-w-6xl gap-10 px-5 py-10 sm:px-8 sm:py-14 lg:grid-cols-[1.05fr_1fr] lg:items-center lg:gap-14 lg:px-12 lg:py-16">
        <div>
          <div className="mb-5 flex flex-wrap items-center gap-2">
            <Chip tone="ink">Casio fx-CG 50</Chip>
            <Chip>OS 3.81</Chip>
            <Chip>MIT</Chip>
          </div>

          {/* leading-[0.95] made the highlight of "in one" taller than its own
              line box, so it painted over the line above and under the line
              below. 1.25 keeps the yellow band inside its line: no vertical
              padding is used, so the band is the glyph box and the line box
              has the spare height. */}
          <h1 className="text-balance text-[2rem] font-black leading-[1.25] tracking-tighter sm:text-5xl lg:text-[3.25rem] xl:text-6xl">
            {TOOL_COUNT_WORD} tools{" "}
            <span className="whitespace-nowrap bg-yellow px-1.5 [box-decoration-break:clone]">
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
