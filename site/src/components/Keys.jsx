import { keys } from "../data.js";
import { Eyebrow, Heading, Blurb, Section } from "../ui.jsx";

const shots = [
  ["25_about.png", "About: version, licence, build"],
  ["26_settings.png", "Settings: three values, saved on change"],
];

export default function Keys() {
  return (
    <Section id="keys" className="border-t-3 border-ink py-14 sm:py-20">
      <Eyebrow accent="teal">Keys and settings</Eyebrow>
      <Heading>Six function keys, the arrows and EXE. That is the whole legend.</Heading>
      <Blurb className="mt-4">
        Text entry never asks for ALPHA: the search field opens a small
        on-screen keyboard and you move with the arrows.
      </Blurb>

      <div className="mt-10 grid gap-8 lg:grid-cols-[1.15fr_1fr] lg:gap-12">
        <div className="overflow-x-auto border-3 border-ink shadow-brute-sm">
          <table className="w-full min-w-[34rem] border-collapse text-left text-sm">
            <thead>
              <tr className="border-b-3 border-ink bg-yellow">
                <th scope="col" className="px-3 py-2 font-mono text-xs uppercase tracking-widest">Key</th>
                <th scope="col" className="px-3 py-2 font-mono text-xs uppercase tracking-widest">Where</th>
                <th scope="col" className="px-3 py-2 font-mono text-xs uppercase tracking-widest">What it does</th>
              </tr>
            </thead>
            <tbody>
              {keys.map(([key, where, what], index) => (
                <tr
                  key={key + where}
                  className={"border-b border-ink/25 last:border-b-0 " + (index % 2 ? "bg-paper" : "bg-shade/40")}
                >
                  <td className="whitespace-nowrap px-3 py-2.5 font-mono font-bold">{key}</td>
                  <td className="px-3 py-2.5 text-ink/70">{where}</td>
                  <td className="px-3 py-2.5 text-ink/85">{what}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>

        <div className="space-y-5">
          {shots.map(([file, caption], index) => (
            <figure key={file}>
              <img
                src={`images/${file}`}
                alt={caption}
                width={792}
                height={448}
                loading="lazy"
                decoding="async"
                className={"pixel block h-auto w-full border-3 border-ink bg-ink " + (index === 0 ? "shadow-brute-sm" : "")}
              />
              <figcaption className="mt-2 font-mono text-[11px] uppercase tracking-wider text-ink/60">
                {caption}
              </figcaption>
            </figure>
          ))}
        </div>
      </div>
    </Section>
  );
}
