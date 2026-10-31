// Small building blocks. Tailwind only keeps classes it can see in the source,
// so every accent colour is spelled out here instead of being built from a
// string at runtime.

export const accents = {
  red: { bg: "bg-red", border: "border-red", text: "text-red", dot: "bg-red" },
  blue: { bg: "bg-blue", border: "border-blue", text: "text-blue", dot: "bg-blue" },
  yellow: { bg: "bg-yellow", border: "border-yellow", text: "text-ink", dot: "bg-yellow" },
  teal: { bg: "bg-teal", border: "border-teal", text: "text-teal", dot: "bg-teal" },
  violet: { bg: "bg-violet", border: "border-violet", text: "text-violet", dot: "bg-violet" },
};

export function Section({ id, children, className = "" }) {
  return (
    <section id={id} className={`scroll-mt-20 px-5 sm:px-8 lg:px-12 ${className}`}>
      <div className="mx-auto w-full max-w-6xl">{children}</div>
    </section>
  );
}

export function Eyebrow({ children, accent = "red" }) {
  return (
    <p className="mb-3 flex items-center gap-2 font-mono text-xs uppercase tracking-[0.18em]">
      <span className={`inline-block h-3 w-3 border border-ink ${accents[accent].dot}`} />
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

export function Chip({ children, className = "" }) {
  return (
    <span
      className={`inline-block border-3 border-ink bg-paper px-3 py-1 font-mono text-xs uppercase tracking-widest ${className}`}
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

export function BruteButton({ href, children, tone = "yellow", onClick, extra = "", type = "a" }) {
  const className =
    "inline-flex items-center justify-center gap-2 border-3 border-ink px-5 py-3 text-base " +
    "font-black shadow-brute-sm transition-all hover:-translate-x-0.5 hover:-translate-y-0.5 " +
    "hover:shadow-brute active:translate-x-0 active:translate-y-0 active:shadow-none " +
    (tones[tone] || tones.yellow) +
    " " +
    extra;

  if (type === "button") {
    return (
      <button type="button" onClick={onClick} className={className}>
        {children}
      </button>
    );
  }
  return (
    <a href={href} className={className}>
      {children}
    </a>
  );
}
