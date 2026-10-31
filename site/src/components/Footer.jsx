import { REPO, VERSION } from "../data.js";

export default function Footer() {
  return (
    <footer className="border-t-3 border-ink bg-paper">
      <div className="mx-auto grid w-full max-w-6xl gap-8 px-5 py-10 sm:px-8 md:grid-cols-3 lg:px-12">
        <div>
          <img src="images/brand-wordmark.png" alt="Bench" width={594} height={152} className="h-10 w-auto" />
          <p className="mt-4 max-w-xs text-sm leading-relaxed text-ink/70">
            Five study tools for the Casio fx-CG 50. Built with fxSDK and gint,
            tested on a PC frame buffer and on the calculator itself.
          </p>
        </div>

        <nav aria-label="Footer" className="space-y-2 text-sm font-bold">
          <p className="font-mono text-xs uppercase tracking-widest text-ink/50">Bench {VERSION}</p>
          <a className="block hover:text-red" href="#tour">Screen tour</a>
          <a className="block hover:text-red" href="#tools">The five tools</a>
          <a className="block hover:text-red" href="#install">Install</a>
          <a className="block hover:text-red" href="#keys">Keys</a>
        </nav>

        <div className="space-y-2 text-sm">
          <p className="font-mono text-xs uppercase tracking-widest text-ink/50">Elsewhere</p>
          <a className="block font-bold hover:text-red" href={REPO} rel="noreferrer">
            Source &amp; issues
          </a>
          <a className="block font-bold hover:text-red" href={`${REPO}/releases`} rel="noreferrer">
            Releases
          </a>
          <a className="block font-bold hover:text-red" href={`${REPO}/blob/main/CHANGELOG.md`} rel="noreferrer">
            Changelog
          </a>
        </div>
      </div>

      <div className="border-t-3 border-ink">
        <div className="mx-auto flex w-full max-w-6xl flex-col gap-2 px-5 py-4 font-mono text-[11px] uppercase tracking-widest text-ink/60 sm:flex-row sm:items-center sm:justify-between sm:px-8 lg:px-12">
          <p>MIT licence &middot; not affiliated with Casio</p>
          <p>Drawn on a 396&times;224 framebuffer</p>
        </div>
      </div>
    </footer>
  );
}
