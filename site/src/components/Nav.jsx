import { useEffect, useState } from "react";

const links = [
  ["Tour", "#tour"],
  ["Tools", "#tools"],
  ["Install", "#install"],
  ["Keys", "#keys"],
];

export default function Nav({ version }) {
  const [open, setOpen] = useState(false);

  // Close the sheet when the window grows past the breakpoint, so a resize does
  // not leave an orphaned panel hanging under the bar.
  useEffect(() => {
    const onResize = () => setOpen(false);
    window.addEventListener("resize", onResize);
    return () => window.removeEventListener("resize", onResize);
  }, []);

  return (
    <header className="sticky top-0 z-40 border-b-3 border-ink bg-paper/95 backdrop-blur-sm">
      <div className="mx-auto flex w-full max-w-6xl items-center justify-between gap-4 px-5 py-3 sm:px-8 lg:px-12">
        <a href="#top" className="flex items-center gap-3" aria-label="Bench, to the top">
          <img
            src="images/brand-wordmark.png"
            alt="Bench"
            width={594}
            height={152}
            className="h-9 w-auto sm:h-10"
          />
        </a>

        <nav className="hidden items-center gap-6 md:flex" aria-label="Sections">
          {links.map(([label, href]) => (
            <a
              key={href}
              href={href}
              className="border-b-3 border-transparent pb-0.5 text-sm font-bold uppercase tracking-wide hover:border-red"
            >
              {label}
            </a>
          ))}
          <a
            href="#install"
            className="border-3 border-ink bg-yellow px-3 py-1.5 font-mono text-xs font-bold shadow-brute-sm hover:-translate-y-0.5"
          >
            v{version}
          </a>
        </nav>

        <button
          type="button"
          className="flex h-10 w-10 flex-col items-center justify-center gap-1 border-3 border-ink bg-paper md:hidden"
          aria-expanded={open}
          aria-controls="mobile-menu"
          aria-label={open ? "Close menu" : "Open menu"}
          onClick={() => setOpen((v) => !v)}
        >
          <span className={`block h-0.5 w-5 bg-ink transition-transform ${open ? "translate-y-1.5 rotate-45" : ""}`} />
          <span className={`block h-0.5 w-5 bg-ink ${open ? "opacity-0" : ""}`} />
          <span className={`block h-0.5 w-5 bg-ink transition-transform ${open ? "-translate-y-1.5 -rotate-45" : ""}`} />
        </button>
      </div>

      {open && (
        <div id="mobile-menu" className="border-t-3 border-ink bg-paper md:hidden">
          <nav className="mx-auto flex w-full max-w-6xl flex-col px-5 py-2" aria-label="Sections">
            {links.map(([label, href]) => (
              <a
                key={href}
                href={href}
                onClick={() => setOpen(false)}
                className="border-b-3 border-ink/10 py-3 text-lg font-bold last:border-b-0"
              >
                {label}
              </a>
            ))}
            <a href="#install" onClick={() => setOpen(false)} className="py-3 font-mono text-sm uppercase">
              Download v{version}
            </a>
          </nav>
        </div>
      )}
    </header>
  );
}
