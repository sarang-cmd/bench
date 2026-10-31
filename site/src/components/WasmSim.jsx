// The add-in itself, compiled to WebAssembly and driven from this page.
//
// Nothing here re-implements a screen: the C in src/ is the same C the
// calculator runs, tools/sim/sim.c is the same stand-in the screenshots come
// from, and emscripten only replaces "keys from a script" with "keys from a
// button" and "write a PPM" with "hand back the framebuffer".
//
// If the binary is not deployed, this component says so in one line and the
// simulator page carries on with the real screenshots.  That is the normal case
// in a clone: build-wasm/ is not in git.
import { useCallback, useEffect, useRef, useState } from "react";

// The gint key codes tools/sim/include/gint/gint.h defines, so a press here is
// the same event the calculator sends.
const KEYS = {
  up: 0x86,
  down: 0x75,
  left: 0x85,
  right: 0x76,
  exe: 0x15,
  exit: 0x74,
  menu: 0x84,
  del: 0x44,
  dot: 0x12,
  neg: 0x14,
  alpha: 0x71,
  f1: 0x91,
  f2: 0x92,
  f3: 0x93,
  f4: 0x94,
  f5: 0x95,
  f6: 0x96,
  0: 0x11,
  1: 0x21,
  2: 0x22,
  3: 0x23,
  4: 0x31,
  5: 0x32,
  6: 0x33,
  7: 0x41,
  8: 0x42,
  9: 0x43,
};

const PAD = [
  [["F1", "f1"], ["F2", "f2"], ["F3", "f3"], ["F4", "f4"], ["F5", "f5"], ["F6", "f6"]],
  [["7", "7"], ["8", "8"], ["9", "9"], ["DEL", "del"], ["AC/EXIT", "exit"]],
  [["4", "4"], ["5", "5"], ["6", "6"], ["x", "neg"], ["MENU", "menu"]],
  [["1", "1"], ["2", "2"], ["3", "3"], ["ALPHA", "alpha"], ["EXE", "exe"]],
  [["0", "0"], [".", "dot"], ["<-", "left"], ["^", "up"], ["->", "right"]],
  [["DOWN", "down"]],
];

function loadScript(url) {
  return new Promise((resolve, reject) => {
    const existing = document.querySelector(`script[src="${url}"]`);
    if (existing) {
      if (window.BenchSim) resolve(window.BenchSim);
      else existing.addEventListener("load", () => resolve(window.BenchSim));
      return;
    }
    const script = document.createElement("script");
    script.src = url;
    script.async = true;
    script.onload = () => resolve(window.BenchSim);
    script.onerror = () => reject(new Error(`could not load ${url}`));
    document.head.appendChild(script);
  });
}

export default function WasmSim() {
  const [state, setState] = useState("loading");
  const canvasRef = useRef(null);
  const moduleRef = useRef(null);
  const frameRef = useRef(0);

  // One rAF loop copies the framebuffer to the canvas while the add-in is idle.
  const pump = useCallback(() => {
    const module = moduleRef.current;
    const canvas = canvasRef.current;

    if (module && canvas) {
      const width = module._bench_wasm_width();
      const height = module._bench_wasm_height();
      const bytes = width * height * 4;
      const ptr = module._malloc(bytes);

      module._bench_wasm_frame(ptr);
      const pixels = new Uint8ClampedArray(module.HEAPU8.buffer, ptr, bytes);
      const context = canvas.getContext("2d");
      const image = context.createImageData(width, height);

      image.data.set(pixels);
      context.putImageData(image, 0, 0);
      module._free(ptr);
    }
    frameRef.current = requestAnimationFrame(pump);
  }, []);

  useEffect(() => {
    let cancelled = false;
    const base = new URL("sim/", document.baseURI).href;

    fetch(new URL("bench.js", base).href, { method: "HEAD" })
      .then((response) => {
        if (!response.ok) throw new Error("no build");
        return loadScript(new URL("bench.js", base).href);
      })
      .then((factory) => factory({ locateFile: (name) => new URL(name, base).href }))
      .then((module) => {
        if (cancelled) return;
        moduleRef.current = module;
        // The add-in's event loop parks inside getkey(); with Asyncify this
        // call returns to the browser and resumes on the next key press.
        module.ccall("bench_wasm_run", "number", [], []);
        setState("ready");
        frameRef.current = requestAnimationFrame(pump);
      })
      .catch(() => {
        if (!cancelled) setState("absent");
      });

    return () => {
      cancelled = true;
      cancelAnimationFrame(frameRef.current);
    };
  }, [pump]);

  const press = (name) => {
    const module = moduleRef.current;

    if (module && KEYS[name] !== undefined)
      module.ccall("bench_wasm_key", "number", ["number"], [KEYS[name]]);
  };

  if (state === "absent") {
    return (
      <div className="mt-8 border-3 border-ink bg-paper p-5">
        <p className="font-mono text-xs uppercase tracking-widest text-ink/60">
          Interactive build
        </p>
        <p className="mt-2 max-w-2xl text-sm leading-relaxed">
          Not deployed in this build. The WebAssembly binary is produced by{" "}
          <code>tools/wasm/build.sh</code> (emscripten), or by the optional CI job, and it is
          deliberately not in git. Everything below is still real output from the same sources.
        </p>
      </div>
    );
  }

  return (
    <div className="mt-8">
      <div className="inline-block border-3 border-ink bg-ink p-1 shadow-brute">
        <canvas
          ref={canvasRef}
          width={396}
          height={224}
          aria-label="The Bench add-in running in the browser"
          className="block h-auto w-full max-w-[396px]"
          style={{ imageRendering: "pixelated" }}
        />
      </div>

      <div className="mt-4 grid max-w-[420px] grid-cols-5 gap-1.5">
        {PAD.flat().map(([label, key]) => (
          <button
            key={key}
            type="button"
            onClick={() => press(key)}
            className={
              "border-3 border-ink bg-paper py-2 font-mono text-xs font-bold shadow-brute-sm " +
              "hover:-translate-y-0.5 hover:bg-yellow active:translate-y-0 active:shadow-none " +
              (label.length > 2 ? "col-span-2" : "")
            }
          >
            {label}
          </button>
        ))}
      </div>

      <p className="mt-3 max-w-[420px] font-mono text-[11px] uppercase tracking-widest text-ink/60">
        {state === "ready" ? "Same C as the calculator - keys go straight into the add-in" : "Loading the add-in..."}
      </p>
    </div>
  );
}
