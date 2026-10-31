import { DOWNLOAD, REPO, VERSION, specs } from "../data.js";
import { BruteButton, Eyebrow, Heading, Blurb, Section } from "../ui.jsx";

const steps = [
  [
    "Download the add-in",
    "One file, Bench-" + VERSION + ".g3a. It is built from the tagged source by CI, not by hand.",
  ],
  [
    "Copy it into the storage root",
    "Connect the calculator in USB Flash mode and drop the file next to the @MainMem folder. Nothing else to extract.",
  ],
  [
    "Press MENU",
    "Bench appears as a new add-in next to the built-in applications. Run it once and it makes its own Bench/ folder.",
  ],
];

const tree = [
  ["fls0/", ""],
  ["  Bench/", ""],
  ["    notes/", "*.md, *.txt - your notes"],
  ["    cards/", "*.txt - decks, question | answer"],
  ["    state/", "cards.sav, settings.cfg"],
];

export default function Install() {
  return (
    <Section id="install" className="border-t-3 border-ink bg-ink py-14 text-paper sm:py-20">
      <Eyebrow accent="yellow" tone="light">Install</Eyebrow>
      <div className="grid gap-10 lg:grid-cols-[1.05fr_1fr] lg:gap-14">
        <div>
          <Heading className="text-paper">Three steps, no companion app.</Heading>
          <Blurb className="mt-4 text-paper/75">
            Bench is a single add-in for OS 3.81. Exam Mode disables add-ins
            entirely and Bench never reads, changes or works around it: if the
            calculator is in Exam Mode, the add-in simply does not run.
          </Blurb>

          <ol className="mt-8 space-y-4">
            {steps.map(([title, text], index) => (
              <li key={title} className="flex gap-4 border-3 border-paper/25 bg-paper/5 p-4">
                <span className="flex h-9 w-9 shrink-0 items-center justify-center border-3 border-paper bg-yellow font-mono text-sm font-bold text-ink">
                  {index + 1}
                </span>
                <div>
                  <h3 className="text-lg font-black">{title}</h3>
                  <p className="mt-1 text-sm leading-relaxed text-paper/70">{text}</p>
                </div>
              </li>
            ))}
          </ol>

          <div className="mt-8 flex flex-wrap items-center gap-3">
            <BruteButton href={DOWNLOAD} tone="yellow" extra="w-full sm:w-auto">
              Download Bench-{VERSION}.g3a
            </BruteButton>
            <BruteButton href={REPO} tone="ink" extra="border-paper bg-paper/10 text-paper hover:bg-paper/20 w-full sm:w-auto">
              Source on GitHub
            </BruteButton>
          </div>
        </div>

        <div className="space-y-6">
          <div className="border-3 border-paper bg-paper/5 p-5">
            <h3 className="font-mono text-xs uppercase tracking-[0.18em] text-paper/60">On the calculator</h3>
            <pre className="mt-3 overflow-x-auto font-mono text-xs leading-relaxed text-paper/85 sm:text-sm">
              {tree.map(([path, note]) => (
                <div key={path}>
                  <span className="text-yellow">{path}</span>
                  <span className="text-paper/50"> {note}</span>
                </div>
              ))}
            </pre>
            <p className="mt-4 text-xs leading-relaxed text-paper/60">
              Settings are plain text and safe to edit: one value per line, a
              reason for each one next to it. Everything is written once, at the
              end of a session or on a settings change, never on a timer.
            </p>
          </div>

          <dl className="divide-y-3 divide-paper/20 border-3 border-paper">
            {specs.map(([key, value]) => (
              <div key={key} className="flex flex-wrap items-baseline justify-between gap-2 px-4 py-3">
                <dt className="font-mono text-xs uppercase tracking-widest text-paper/55">{key}</dt>
                <dd className="text-sm font-bold">{value}</dd>
              </div>
            ))}
          </dl>
        </div>
      </div>
    </Section>
  );
}
