// The seven pages behind the landing page.  Each one reuses the sections the
// landing page already had (Tools, Keys, Tour, Install) instead of a second copy
// of the same copy, so a change to a tool card or a key row shows up everywhere.
//
// Content rules: no invented screenshots, no invented numbers.  Every count
// comes from src/stats.generated.json, src/tools.generated.json or the build
// info, all generated at build time from the repository.
import { BUILD, DOWNLOAD, REPO, SHA256, VERSION, tools } from "./data.js";
import stats from "./stats.generated.json";
import changelog from "./changelog.generated.json";
import Tools from "./components/Tools.jsx";
import Keys from "./components/Keys.jsx";
import Tour from "./components/Tour.jsx";
import Install from "./components/Install.jsx";
import WasmSim from "./components/WasmSim.jsx";
import Spike from "./components/Spike.jsx";
import { Link } from "./router.jsx";
import { Blurb, BruteButton, BruteLink, Card, Chip, Eyebrow, Heading, Screen, Section } from "./ui.jsx";

function Page({ eyebrow, accent = "red", title, lede, children }) {
  return (
    <Section className="py-12 sm:py-16">
      <Eyebrow accent={accent}>{eyebrow}</Eyebrow>
      <Heading>{title}</Heading>
      <Blurb className="mt-4">{lede}</Blurb>
      {children}
    </Section>
  );
}

// ---------------------------------------------------------------- features --

export function FeaturesPage() {
  return (
    <>
      <Tools />
      <Keys />
      <Section className="py-12">
        <div className="flex flex-wrap gap-4">
          <BruteLink to="/simulator">See the screens</BruteLink>
          <BruteLink to="/install" tone="paper">
            Install it
          </BruteLink>
        </div>
      </Section>
    </>
  );
}

// --------------------------------------------------------------- simulator --

export function SimulatorPage() {
  return (
    <>
      <Page
        eyebrow="In the browser"
        accent="violet"
        title="The screens, without the calculator."
        lede="Every picture below came out of the add-in itself: the same C, compiled against a stand-in for gint, driven by a key script and captured from its frame buffer. Above them is that same binary compiled to WebAssembly: press the buttons and the calculator answers."
      >
        <div className="mt-8 grid gap-4 sm:grid-cols-3">
          <Card>
            <p className="font-mono text-xs uppercase tracking-widest text-ink/60">Working</p>
            <p className="mt-2 text-sm leading-relaxed">
              The tour gallery. {stats.screens} captures from {stats.tourSteps} key scripts, one
              per screen and per empty state.
            </p>
          </Card>
          <Card>
            <p className="font-mono text-xs uppercase tracking-widest text-ink/60">Interactive</p>
            <p className="mt-2 text-sm leading-relaxed">
              The add-in itself runs above, in WebAssembly: no re-implementation, the same C, keys
              from the buttons, on an in-memory filesystem holding the demo notes and decks. Move
              with the arrows, open a tool with EXE.
            </p>
          </Card>
          <Card>
            <p className="font-mono text-xs uppercase tracking-widest text-ink/60">Why it matters</p>
            <p className="mt-2 text-sm leading-relaxed">
              The simulator is the same source as the calculator build, so a screen that looks
              wrong here looks wrong on the hardware.
            </p>
          </Card>
        </div>
        <WasmSim />
      </Page>
      <Tour />
    </>
  );
}

// ---------------------------------------------------------------- tutorial --

const tutorial = [
  {
    title: "1. The home screen",
    shot: "01_menu.png",
    alt: "Bench home screen with four category tiles",
    body: "F1 to F4 are the categories, arrows move the cursor, EXE opens the tile under it and F5 explains it. The strip under the title counts what is on the card: how many decks, how many notes.",
    keys: "F1-F4 categories, arrows, EXE open, F5 help",
  },
  {
    title: "2. A tool is a screen, not a dialog",
    shot: "44_cplx_mul.png",
    alt: "Complex numbers screen with two values and a product",
    body: "Each tool keeps everything it needs on one screen: the fields, the operation and the answer. F1 to F4 are the operations inside a tool, EXE applies, EXIT goes back to the menu.",
    keys: "F1 clear, F2 operation, arrows, EXE",
  },
  {
    title: "3. Put a note in Bench/notes",
    shot: "18_notes_markdown.png",
    alt: "Notes reader showing a Markdown page",
    body: "Copy .md or .txt files into Bench/notes in USB Flash mode. Notes draws headings, lists, quotes, bold and code in the palette and wraps long lines; ALPHA searches inside the file.",
    keys: "UP/DOWN scroll, EXE page, ALPHA search",
  },
  {
    title: "4. Run a card session",
    shot: "12_cards_deck.png",
    alt: "Cards deck screen showing a deck and its boxes",
    body: "One deck is one text file in Bench/cards, one card per line as question | answer. A session deals the cards due today; right answers move a card to the next box, wrong ones send it back. Progress is written once, at the end.",
    keys: "F1 start, F2 mixed order, EXE reveal, F1 right, F2 wrong",
  },
  {
    title: "5. Log the minutes",
    shot: "70_log_entries.png",
    alt: "Study log screen showing totals, days and a streak",
    body: "The study log is a plain text file, one session per line. The screen totals the minutes, counts the days and shows the longest run; nothing is written until you press EXIT, and then once.",
    keys: "F1 add, F2 delete the last entry, LEFT/RIGHT minutes",
  },
  {
    title: "6. Make it yours",
    shot: "26_settings.png",
    alt: "Settings screen with five rows",
    body: "Settings holds the uncertainty figure rule, the pacer checkpoint size, the notes wrap width, the unit Triangle opens in and whether the first-run line has been read. Every value is one line in Bench/state/settings.cfg and is written only when it changes.",
    keys: "UP/DOWN row, LEFT/RIGHT change, EXIT back",
  },
];

export function TutorialPage() {
  return (
    <Page
      eyebrow="Ten minutes"
      accent="teal"
      title="From first launch to a study session."
      lede="Six steps, in the order you will actually do them. The screens are the real ones; the same text is in docs/TUTORIAL.md in the repository."
    >
      <ol className="mt-10 space-y-10">
        {tutorial.map((step) => (
          <li key={step.title} className="grid gap-6 md:grid-cols-[minmax(0,1fr)_320px] md:items-start">
            <div>
              <h3 className="text-xl font-black">{step.title}</h3>
              <p className="mt-2 max-w-2xl text-base leading-relaxed text-ink/75">{step.body}</p>
              <p className="mt-3 inline-block border-3 border-ink bg-paper px-2 py-1 font-mono text-xs uppercase tracking-wider">
                {step.keys}
              </p>
            </div>
            <Screen file={step.shot} alt={step.alt} />
          </li>
        ))}
      </ol>
    </Page>
  );
}

// ----------------------------------------------------------------- install --

export function InstallPage() {
  return (
    <>
      <Install />
      <Page
        eyebrow="Before you press anything"
        accent="yellow"
        title="Three things that stop an add-in."
        lede="Bench is a normal .g3a: it launches from MENU like any built-in application. When it does not appear, it is almost always one of these."
      >
        <div className="mt-8 grid gap-4 md:grid-cols-3">
          <Card>
            <p className="font-black">The file is not in the storage root</p>
            <p className="mt-2 text-sm leading-relaxed">
              It goes in the top folder of the calculator memory, next to @MainMem, not inside a
              subfolder. USB Flash mode, drag, drop, done.
            </p>
          </Card>
          <Card>
            <p className="font-black">Exam Mode is on</p>
            <p className="mt-2 text-sm leading-relaxed">
              Add-ins are blocked while Exam Mode is active. Bench never touches that mode; turn it
              off the way you turned it on.
            </p>
          </Card>
          <Card>
            <p className="font-black">The download was cut short</p>
            <p className="mt-2 text-sm leading-relaxed">
              Compare the SHA-256 on the downloads page with the file you saved. A partial file
              still copies and still refuses to start.
            </p>
          </Card>
        </div>
        <p className="mt-6 text-sm text-ink/70">
          Full detail, including what each screen does, is in{" "}
          <a className="font-bold underline hover:text-red" href={`${REPO}/blob/main/README.md`}>
            the README
          </a>
          .
        </p>
      </Page>
    </>
  );
}

// --------------------------------------------------------------- downloads --

const places = [
  ["This site", DOWNLOAD, "One click, no account. The file sits in the deployment, not on GitHub."],
  ["GitHub release", `${REPO}/releases/tag/v${VERSION}`, "The same bytes attached to the tag of the same name."],
  ["dist/ in the repository", `${REPO}/blob/main/dist/Bench-${VERSION}.g3a`, "A normal file in a normal clone, with dist/BUILD.txt naming the commit and the hash."],
  ["The builds branch", `${REPO}/tree/builds`, "git fetch origin builds && git show origin/builds:Bench-" + VERSION + ".g3a > Bench-" + VERSION + ".g3a, for when release assets are blocked."],
];

export function DownloadsPage() {
  return (
    <Page
      eyebrow="Four doors, one file"
      accent="blue"
      title="Get the add-in without a detour."
      lede="Bench never asks for an account and never sends you through an installer. Pick whichever of these reaches you."
    >
      <div className="mt-8 flex flex-wrap items-center gap-4">
        <BruteButton href={DOWNLOAD} download>
          Download {VERSION} <span className="font-mono text-xs">.g3a</span>
        </BruteButton>
        <Chip tone="paper">{Math.round(BUILD.size / 1024)} KiB</Chip>
        <Chip tone="paper">version {VERSION}</Chip>
      </div>

      <ul className="mt-10 grid gap-4 md:grid-cols-2">
        {places.map(([where, href, why]) => (
          <li key={where}>
            <Card>
              <p className="font-black">{where}</p>
              <p className="mt-2 text-sm leading-relaxed text-ink/75">{why}</p>
              <a
                className="mt-3 inline-block break-all font-mono text-xs underline hover:text-red"
                href={href}
              >
                {href}
              </a>
            </Card>
          </li>
        ))}
      </ul>

      <div className="mt-10 border-3 border-ink bg-ink p-5 text-paper">
        <p className="font-mono text-xs uppercase tracking-widest text-paper/60">
          SHA-256 of Bench-{VERSION}.g3a
        </p>
        <p className="mt-2 break-all font-mono text-sm">{SHA256}</p>
        <p className="mt-3 font-mono text-xs text-paper/70">
          sha256sum Bench-{VERSION}.g3a
        </p>
      </div>

      <Spike />

      <p className="mt-6 text-sm text-ink/70">
        Installing it is three steps; the{" "}
        <Link to="/install" className="font-bold underline hover:text-red">
          install page
        </Link>{" "}
        has them, plus the three things that stop a launch.
      </p>
    </Page>
  );
}

// --------------------------------------------------------------- changelog --

export function ChangelogPage() {
  return (
    <Page
      eyebrow="Every version"
      accent="red"
      title="What changed, newest first."
      lede="Generated from CHANGELOG.md at build time, so this page cannot disagree with the file. Versions that were never tagged are marked as milestones."
    >
      <div className="mt-10 space-y-8">
        {changelog.releases.map((release) => (
          <article key={release.version} className="border-t-3 border-ink pt-6">
            <h3 className="flex flex-wrap items-baseline gap-3 text-xl font-black">
              {release.version}
              {release.date && (
                <span className="font-mono text-xs uppercase tracking-widest text-ink/50">
                  {release.date}
                </span>
              )}
            </h3>
            <ul className="mt-4 space-y-4">
              {release.entries.map((entry) => (
                <li key={entry.title} className="max-w-3xl">
                  <p className="font-bold">{entry.title}</p>
                  <p className="mt-1 text-sm leading-relaxed text-ink/75">{entry.body}</p>
                </li>
              ))}
            </ul>
          </article>
        ))}
      </div>
    </Page>
  );
}

// ------------------------------------------------------------------- about --

const nots = [
  "Not a CAS. It does the arithmetic a paper asks for, and shows the working.",
  "Not a cloud service. No account, no sync, no network call from the calculator.",
  "Not a replacement for a textbook. The demo content is small on purpose.",
  "Not finished. Nothing after 1.0.0 has been on a calculator yet: docs/HARDWARE.md lists what the next session has to run.",
];

export function AboutPage() {
  const figures = [
    [String(stats.tools), "tools in the menu"],
    [String(stats.screens), "captured screens"],
    [String(stats.tourSteps), "tour scripts"],
    [String(stats.testPrograms), "native test programs"],
    [stats.sourceLines.toLocaleString("en"), "lines of C"],
    [VERSION, "version"],
  ];

  return (
    <Page
      eyebrow="About"
      accent="violet"
      title="One add-in, built to be read."
      lede="Bench is a study toolkit for the Casio fx-CG 50, written in C against gint, with the arithmetic split out into a core that runs on a PC so it can be tested without a calculator."
    >
      <dl className="mt-10 grid grid-cols-2 gap-4 sm:grid-cols-3 lg:grid-cols-6">
        {figures.map(([value, label]) => (
          <div key={label} className="border-3 border-ink bg-paper p-4 shadow-brute-sm">
            <dt className="font-mono text-xs uppercase tracking-widest text-ink/60">{label}</dt>
            <dd className="mt-1 text-2xl font-black">{value}</dd>
          </div>
        ))}
      </dl>

      <div className="mt-12 grid gap-8 md:grid-cols-2">
        <div>
          <h3 className="text-xl font-black">What it is not</h3>
          <ul className="mt-4 space-y-3">
            {nots.map((line) => (
              <li key={line} className="border-l-3 border-red pl-3 text-sm leading-relaxed">
                {line}
              </li>
            ))}
          </ul>
        </div>
        <div>
          <h3 className="text-xl font-black">What has been checked</h3>
          <ul className="mt-4 space-y-3 text-sm leading-relaxed">
            <li className="border-l-3 border-teal pl-3">
              Arithmetic: {stats.testPrograms} native test programs, run by <code>make test</code>{" "}
              on every push.
            </li>
            <li className="border-l-3 border-teal pl-3">
              Screens: every one is rendered by the simulator and checked for overlapping text at
              396x224.
            </li>
            <li className="border-l-3 border-yellow pl-3">
              Hardware: version 1.0.0 was run on a real fx-CG 50 by the person who asked for it.
              Everything after that - all of V2, including these pages and the browser simulator -
              is marked "unverified on device" until it has been run there too.
            </li>
          </ul>
        </div>
      </div>

      <div className="mt-12 border-3 border-ink bg-ink p-5 text-paper">
        <p className="font-mono text-xs uppercase tracking-widest text-paper/60">This deployment</p>
        <p className="mt-2 break-all font-mono text-sm">
          {BUILD.file} - {Math.round(BUILD.size / 1024)} KiB
          {BUILD.commit ? ` - commit ${BUILD.commit.slice(0, 7)}` : ""}
          {BUILD.date ? ` - ${BUILD.date}` : ""}
        </p>
        <p className="mt-3 break-all font-mono text-xs text-paper/70">sha256 {SHA256}</p>
        <p className="mt-3 text-sm text-paper/80">
          Bench is one add-in and {tools.length} tools; the source, the tests and every screenshot
          are in{" "}
          <a className="font-bold underline" href={REPO}>
            the repository
          </a>
          .
        </p>
      </div>
    </Page>
  );
}
