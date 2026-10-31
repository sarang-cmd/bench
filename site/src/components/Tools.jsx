import { tools } from "../data.js";
import { Eyebrow, Heading, Blurb, Screen, Section, accents } from "../ui.jsx";

export default function Tools() {
  return (
    <Section id="tools" className="border-t-3 border-ink py-14 sm:py-20">
      <Eyebrow accent="blue">What is inside</Eyebrow>
      <Heading>Five tools, each one a screen away.</Heading>
      <Blurb className="mt-4">
        The menu is the whole interface: pick a tile, answer its prompts, press
        EXIT to come back. Nothing is hidden behind a manual.
      </Blurb>

      <div className="mt-12 space-y-12 sm:space-y-16">
        {tools.map((tool, index) => {
          const accent = accents[tool.accent];
          const lead = tool.shots[tool.shots.length - 1];
          return (
            <article
              key={tool.id}
              className="grid gap-6 border-3 border-ink bg-paper p-5 shadow-brute sm:p-7 lg:grid-cols-[1fr_1.1fr] lg:items-center lg:gap-10"
            >
              <div className={index % 2 ? "lg:order-2" : ""}>
                <div className="mb-3 flex items-center gap-3">
                  <span className={`flex h-9 w-9 items-center justify-center border-3 border-ink ${accent.dot} font-mono text-sm font-bold`}>
                    {String(index + 1).padStart(2, "0")}
                  </span>
                  <h3 className="text-2xl font-black sm:text-3xl">{tool.name}</h3>
                </div>
                <p className="font-mono text-xs uppercase tracking-widest text-ink/60">{tool.tag}</p>
                <p className="mt-4 text-base leading-relaxed text-ink/80">{tool.body}</p>
                <ul className="mt-5 space-y-2">
                  {tool.points.map((point) => (
                    <li key={point} className="flex gap-3 text-sm leading-snug">
                      <span className={`mt-1.5 h-2 w-2 shrink-0 border border-ink ${accent.dot}`} aria-hidden="true" />
                      <span className="text-ink/85">{point}</span>
                    </li>
                  ))}
                </ul>
              </div>

              <div className={index % 2 ? "lg:order-1" : ""}>
                <figure>
                  <div className="relative">
                    <div className={`absolute -inset-1.5 -z-10 translate-x-1.5 translate-y-1.5 border-3 border-ink ${accent.dot}`} aria-hidden="true" />
                    <Screen file={lead[0]} alt={lead[1]} />
                  </div>
                  <figcaption className="mt-3 font-mono text-[11px] uppercase tracking-wider text-ink/60">
                    {lead[1]}
                  </figcaption>
                </figure>
                <div className="mt-3 grid grid-cols-3 gap-2">
                  {tool.shots.slice(0, 3).map(([file, caption]) => (
                    <img
                      key={file}
                      src={`images/${file}`}
                      alt={caption}
                      width={792}
                      height={448}
                      loading="lazy"
                      decoding="async"
                      className="pixel block h-auto w-full border-3 border-ink bg-ink"
                    />
                  ))}
                </div>
              </div>
            </article>
          );
        })}
      </div>
    </Section>
  );
}
