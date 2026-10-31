import { useState } from "react";
import { tools } from "../data.js";
import { Eyebrow, Screen, Section, Heading, Blurb, accents } from "../ui.jsx";

function Thumb({ shot, active, accent, onSelect }) {
  const [file, caption] = shot;
  return (
    <button
      type="button"
      onClick={onSelect}
      aria-pressed={active}
      title={caption}
      className={
        "group relative border-3 border-ink bg-ink p-0.5 text-left transition-transform " +
        (active
          ? `-translate-y-0.5 ${accents[accent].dot} shadow-brute-sm`
          : "hover:-translate-y-0.5 hover:shadow-brute-sm")
      }
    >
      <img
        src={`images/${file}`}
        alt={caption}
        width={792}
        height={448}
        loading="lazy"
        decoding="async"
        className="pixel block h-auto w-full"
      />
      <span
        className={`absolute inset-x-0 bottom-0 hidden bg-ink/85 px-1.5 py-1 font-mono text-[10px] leading-tight text-paper sm:block ${
          active ? "" : "group-hover:block"
        }`}
      >
        {caption}
      </span>
    </button>
  );
}

export default function Tour() {
  const [toolIndex, setToolIndex] = useState(0);
  const [shotIndex, setShotIndex] = useState(0);
  const tool = tools[toolIndex];
  const [file, caption] = tool.shots[Math.min(shotIndex, tool.shots.length - 1)];
  const accent = accents[tool.accent];

  const pickTool = (index) => {
    setToolIndex(index);
    setShotIndex(0);
  };

  return (
    <Section id="tour" className="py-14 sm:py-20">
      <Eyebrow accent={tool.accent}>The tour</Eyebrow>
      <div className="flex flex-col gap-4 sm:flex-row sm:items-end sm:justify-between">
        <Heading>Twenty-six screens, every one of them real.</Heading>
        <Blurb className="max-w-md text-base sm:text-base">
          Every picture here was drawn by the add-in&rsquo;s own code, rendered on
          a PC by <code className="font-mono">make sim</code>. No mock-ups.
        </Blurb>
      </div>

      {/* Tool selector: a wrapping row of hard-edged tabs, never a scroller. */}
      <div role="tablist" aria-label="Tools" className="mt-8 flex flex-wrap gap-2">
        {tools.map((t, index) => {
          const active = index === toolIndex;
          return (
            <button
              key={t.id}
              role="tab"
              id={`tab-${t.id}`}
              aria-selected={active}
              aria-controls="tour-panel"
              onClick={() => pickTool(index)}
              className={
                "border-3 border-ink px-3 py-2 text-sm font-black sm:text-base " +
                (active
                  ? `${accents[t.accent].bg} ${accents[t.accent].fg} shadow-brute-sm`
                  : "bg-paper hover:bg-shade")
              }
            >
              {t.name}
            </button>
          );
        })}
      </div>

      <div
        id="tour-panel"
        role="tabpanel"
        aria-labelledby={`tab-${tool.id}`}
        className="mt-6 grid gap-6 lg:grid-cols-[1.35fr_1fr] lg:items-start"
      >
        <figure className="order-2 lg:order-1">
          <div className="relative">
            <div className={`absolute -inset-2 -z-10 translate-x-2 translate-y-2 border-3 border-ink ${accent.dot}`} aria-hidden="true" />
            <Screen file={file} alt={caption} />
          </div>
          <figcaption className="mt-4 flex items-start gap-3 border-l-3 border-ink pl-3 text-sm leading-snug text-ink/80">
            <span className={`mt-1 h-2.5 w-2.5 shrink-0 border border-ink ${accent.dot}`} aria-hidden="true" />
            {caption}
          </figcaption>
        </figure>

        <div className="order-1 lg:order-2">
          <h3 className="text-2xl font-black sm:text-3xl">{tool.name}</h3>
          <p className="mt-2 font-mono text-xs uppercase tracking-widest text-ink/60">{tool.tag}</p>
          <p className="mt-4 text-base leading-relaxed text-ink/80">{tool.summary}</p>
          <div className="mt-5 grid grid-cols-3 gap-2 sm:grid-cols-3">
            {tool.shots.map((shot, index) => (
              <Thumb
                key={shot[0]}
                shot={shot}
                accent={tool.accent}
                active={shot[0] === file}
                onSelect={() => setShotIndex(index)}
              />
            ))}
          </div>
        </div>
      </div>
    </Section>
  );
}
