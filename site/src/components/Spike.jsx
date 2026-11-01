// The M1 diagnostic add-in, on the downloads page.
//
// Everything shown here comes from spike.generated.json, which
// scripts/fetch-spike.mjs writes after it has actually copied the files into
// public/. A download button therefore cannot exist unless the file behind it
// does, and the page never claims a build it does not have: when CI has not
// produced a spike binary yet, the card says so and offers the patch, which is
// in the repository and builds the same add-in with `fxsdk build-cg`.
import spike from "../spike.generated.json";
import { REPO } from "../data.js";
import { BruteButton, Card, Chip } from "../ui.jsx";

const kib = (bytes) => `${Math.round(bytes / 1024)} KiB`;

export default function Spike() {
  const build = spike.binary;

  return (
    <section className="mt-12">
      <div className="flex flex-wrap items-baseline gap-3">
        <h2 className="text-2xl font-black tracking-tight">The diagnostic add-in</h2>
        <Chip tone="yellow">not Bench</Chip>
      </div>
      <p className="mt-3 max-w-2xl text-sm leading-relaxed text-ink/75">
        A second, throwaway add-in. It runs ten measurements on a real calculator
        and prints the answers on screen: which file calls work, how paths look,
        whether float printing works, which font symbols draw, how much heap
        there is, how long a frame takes. It is how the code in Bench was written
        against measured facts rather than guesses. It is not part of Bench, it
        does not ship in any Release, and it is not needed to use anything here.
      </p>

      <div className="mt-5 grid gap-4 md:grid-cols-2">
        <Card>
          <p className="font-black">Bench-spike, the built add-in</p>
          {build ? (
            <>
              <p className="mt-2 text-sm leading-relaxed text-ink/75">
                Menu name <code className="font-mono text-[0.9em]">BenchSpk</code>.
                Copy it to the storage root exactly like Bench, open it from MENU,
                and read each answer off the screen.
              </p>
              <div className="mt-4 flex flex-wrap items-center gap-3">
                <BruteButton href={`./${build.file}`} tone="blue" download>
                  Download {build.file}
                </BruteButton>
                <Chip tone="paper">{kib(build.size)}</Chip>
                <Chip tone="paper">
                  {build.commit ? `commit ${build.commit.slice(0, 7)}` : "unstamped build"}
                </Chip>
              </div>
              <p className="mt-3 break-all font-mono text-[11px] leading-relaxed text-ink/55">
                sha256 {build.sha256}
              </p>
              {build.verified && (
                <p className="mt-1 font-mono text-[11px] text-ink/55">
                  matches dist/SPIKE.txt in the repository
                </p>
              )}
            </>
          ) : (
            <p className="mt-2 text-sm leading-relaxed text-ink/75">
              Not built yet. The add-in needs the SuperH toolchain, which CI has
              and this deployment does not, so this card fills in on the next
              build that puts <code className="font-mono text-[0.9em]">Bench-spike-*.g3a</code>{" "}
              into{" "}
              <a
                className="font-bold underline hover:text-red"
                href={`${REPO}/tree/main/dist`}
              >
                dist/
              </a>
              . Until then, the patch below is the whole thing: applying it and
              running <code className="font-mono text-[0.9em]">fxsdk build-cg</code>{" "}
              gives the same add-in.
            </p>
          )}
        </Card>

        <Card>
          <p className="font-black">spike.diff, the source as one patch</p>
          <p className="mt-2 text-sm leading-relaxed text-ink/75">
            <code className="font-mono text-[0.9em]">spike/main.c</code> and{" "}
            <code className="font-mono text-[0.9em]">spike/CMakeLists.txt</code> as a
            single patch, so the whole thing fits in a support thread or a
            toolchain container without the rest of the repository:
          </p>
          <pre className="mt-3 overflow-x-auto border-3 border-ink bg-paper p-3 font-mono text-xs leading-relaxed">
{`git apply spike.diff
cd spike && fxsdk build-cg`}
          </pre>
          {spike.patch && (
            <>
              <div className="mt-4 flex flex-wrap items-center gap-3">
                <BruteButton href="./spike.diff" download>
                  Download spike.diff
                </BruteButton>
                <Chip tone="paper">{kib(spike.patch.size)}</Chip>
              </div>
              <p className="mt-3 break-all font-mono text-[11px] leading-relaxed text-ink/55">
                sha256 {spike.patch.sha256}
              </p>
            </>
          )}
        </Card>
      </div>

      <p className="mt-4 text-sm leading-relaxed text-ink/70">
        Two <code className="font-mono text-[0.9em]">.g3a</code> files with similar
        names is a trap, so{" "}
        <a className="font-bold underline hover:text-red" href={`${REPO}/blob/main/docs/spike.md`}>
          docs/spike.md
        </a>{" "}
        says which is which. If all you want is the study add-in, you want the
        button at the top of this page.
      </p>
    </section>
  );
}
