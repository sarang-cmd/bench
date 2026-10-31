import { BruteButton, Chip, Screen } from "../ui.jsx";

const stats = [
  ["5", "tools, one menu"],
  ["384\u00d7216", "pixels, no scrolling chrome"],
  ["0", "dependencies beyond gint"],
];

export default function Hero({ version }) {
  return (
    <div id="top" className="relative overflow-hidden border-b-3 border-ink bg-paper">
      {/* Flat blocks, no gradients: the house style. */}
      <div aria-hidden="true" className="pointer-events-none absolute inset-0">
        <div className="absolute -right-16 -top-20 h-56 w-56 rotate-12 border-3 border-ink bg-yellow/70 sm:h-72 sm:w-72" />
        <div className="absolute bottom-10 left-1/2 hidden h-24 w-24 rotate-45 border-3 border-ink bg-teal/50 lg:block" />
      </div>

      <div className="relative mx-auto grid w-full max-w-6xl gap-10 px-5 py-12 sm:px-8 sm:py-16 lg:grid-cols-[1.05fr_1fr] lg:items-center lg:gap-14 lg:px-12 lg:py-20">
        <div>
          <div className="mb-5 flex flex-wrap items-center gap-2">
            <Chip className="bg-ink text-paper">Casio fx-CG 50</Chip>
            <Chip>OS 3.81</Chip>
            <Chip>MIT</Chip>
          </div>

          <h1 className="text-5xl font-black leading-[0.95] tracking-tighter sm:text-6xl lg:text-7xl">
            Five study tools
            <br />
            <span className="bg-yellow px-1 [box-decoration-break:clone]">in one</span> add-in.
          </h1>

          <p className="mt-6 max-w-xl text-lg leading-relaxed text-ink/80">
            Pacer times a practice paper. Uncertainty does the error arithmetic.
            Cards drills flashcards on a Leitner schedule. Notes reads Markdown.
            Number theory goes from gcd to CRT. One <code className="font-mono text-base">.g3a</code>,
            one menu, nothing else to install.
          </p>

          <div className="mt-8 flex flex-wrap items-center gap-3">
            <BruteButton href="#install">
              Get Bench {version}
            </BruteButton>
            <BruteButton href="#tour" tone="paper">
              See every screen &rarr;
            </BruteButton>
          </div>

          <dl className="mt-10 grid max-w-lg grid-cols-3 gap-3 border-t-3 border-ink pt-5">
            {stats.map(([value, label]) => (
              <div key={label}>
                <dt className="font-mono text-xs uppercase tracking-widest text-ink/60">{label}</dt>
                <dd className="mt-1 text-2xl font-black sm:text-3xl">{value}</dd>
              </div>
            ))}
          </dl>
        </div>

        <div className="relative">
          <div className="absolute -inset-2 -z-10 translate-x-2 translate-y-2 border-3 border-ink bg-red" aria-hidden="true" />
          <Screen file="01_menu.png" alt="The Bench home screen: six tiles for Pacer, Uncertainty, Cards, Notes, Number theory and Settings." />
          <p className="mt-3 font-mono text-xs uppercase tracking-widest text-ink/60">
            The home screen &mdash; a real frame, drawn by the add-in
          </p>
        </div>
      </div>
    </div>
  );
}
