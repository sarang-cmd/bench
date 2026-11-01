import { Link } from "./router.jsx";

// Small building blocks. Tailwind only keeps classes it can see in the source,
// so every accent colour is spelled out here instead of being built from a
// string at runtime.

// bg / dot: the accent as a background; fg: readable text on top of it.
export const accents = {
  red: { bg: "bg-red", text: "text-red", dot: "bg-red", fg: "text-paper" },
  blue: { bg: "bg-blue", text: "text-blue", dot: "bg-blue", fg: "text-paper" },
  yellow: { bg: "bg-yellow", text: "text-ink", dot: "bg-yellow", fg: "text-ink" },
  teal: { bg: "bg-teal", text: "text-teal", dot: "bg-teal", fg: "text-paper" },
  violet: { bg: "bg-violet", text: "text-violet", dot: "bg-violet", fg: "text-paper" },
};

export function Section({ id, children, className = "" }) {
  return (
    <section id={id} className={`scroll-mt-20 px-5 sm:px-8 lg:px-12 ${className}`}>
      <div className="mx-auto w-full max-w-6xl">{children}</div>
    </section>
  );
}

export function Eyebrow({ children, accent = "red", tone = "dark" }) {
  return (
    <p
      className={
        "mb-3 flex items-center gap-2 font-mono text-xs uppercase tracking-[0.18em] " +
        (tone === "light" ? "text-paper/70" : "text-ink/70")
      }
    >
      <span className={`inline-block h-3 w-3 border border-current ${accents[accent].dot}`} />
      {children}
    </p>
  );
}

export function Heading({ children, className = "" }) {
  return (
    <h2
      className={`text-3xl font-black leading-[1.05] tracking-tight sm:text-4xl lg:text-5xl ${className}`}
    >
      {children}
    </h2>
  );
}

export function Blurb({ children, className = "" }) {
  return (
    <p className={`max-w-2xl text-base leading-relaxed text-ink/75 sm:text-lg ${className}`}>
      {children}
    </p>
  );
}

// The calculator screen: 396x224 framebuffer, captured at 2x, so the image is
// kept on a black ground inside a hard-edged frame and never blurred.
export function Screen({ file, alt, className = "" }) {
  return (
    <div
      className={`border-3 border-ink bg-ink shadow-brute ${className}`}
    >
      <img
        src={`images/${file}`}
        alt={alt}
        width={792}
        height={448}
        loading="lazy"
        decoding="async"
        className="pixel block h-auto w-full bg-ink"
      />
    </div>
  );
}

// tone instead of a className merge: Tailwind cannot resolve two competing
// background utilities on one element, and the first chip came out invisible
// (dark text on a paper background that won the cascade).
const chipTones = {
  paper: "bg-paper text-ink",
  ink: "bg-ink text-paper",
  yellow: "bg-yellow text-ink",
};

export function Card({ children, className = "" }) {
  return (
    <div className={"border-3 border-ink bg-paper p-5 shadow-brute-sm " + className}>{children}</div>
  );
}

export function Chip({ children, tone = "paper", className = "" }) {
  return (
    <span
      className={
        "inline-block border-3 border-ink px-3 py-1 font-mono text-xs uppercase tracking-widest " +
        (chipTones[tone] || chipTones.paper) +
        " " +
        className
      }
    >
      {children}
    </span>
  );
}

const tones = {
  yellow: "bg-yellow text-ink",
  paper: "bg-paper text-ink",
  ink: "bg-ink text-paper",
  red: "bg-red text-paper",
  teal: "bg-teal text-paper",
};

// The same shape as BruteButton, as an in-app link: a button that keeps the
// router awake instead of reloading the page.
export function BruteLink({ to, children, tone = "yellow", extra = "", onClick }) {
  return (
    <Link to={to} className={buttonClass(tone, extra)} onClick={onClick}>
      {children}
    </Link>
  );
}

function buttonClass(tone, extra) {
  return (
    "inline-flex items-center justify-center gap-2 border-3 border-ink px-5 py-3 text-base " +
    "font-black shadow-brute-sm transition-all hover:-translate-x-0.5 hover:-translate-y-0.5 " +
    "hover:shadow-brute active:translate-x-0 active:translate-y-0 active:shadow-none " +
    (tones[tone] || tones.yellow) +
    " " +
    extra
  );
}

export function BruteButton({ href, children, tone = "yellow", onClick, extra = "", type = "a", download = false }) {
  const className = buttonClass(tone, extra);

  if (type === "button") {
    return (
      <button type="button" onClick={onClick} className={className}>
        {children}
      </button>
    );
  }
  return (
    <a href={href} className={className} download={download || undefined}>
      {children}
    </a>
  );
}
