// The add-in itself, compiled to WebAssembly and driven from this page.
//
// Nothing here re-implements a screen: the C in src/ is the same C the
// calculator runs, tools/sim/sim.c is the same stand-in the screenshots come
// from, and tools/wasm/build.sh only changes where keys come from and where the
// frame goes.  See tools/wasm/worker.js for the other half.
//
// The module runs in a Web Worker (its getkey() blocks, which a worker may do),
// and the two directions are a SharedArrayBuffer: the page writes a key code and
// wakes the worker, the worker writes the framebuffer and bumps a counter the
// page's animation frame reads.  No Asyncify, no compiler on the visitor's
// machine.
//
// If the binary is not deployed, this component says so in one line and the
// simulator page carries on with the real screenshots.
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

// header: keySeq, key, frameSeq, width, height (Int32), then the pixels.
const HEADER_BYTES = 32;
const WIDTH = 396;
const HEIGHT = 224;

export default function WasmSim() {
  const [state, setState] = useState("loading");
  const canvasRef = useRef(null);
  const headerRef = useRef(null);
  const pixelsRef = useRef(null);
  const workerRef = useRef(null);
  const frameRef = useRef(0);
  const drawnRef = useRef(-1);
  // Presses the worker has not taken yet.  One key at a time: the add-in's
  // getkey() consumes one, and the frame it publishes is the signal that it is
  // ready for the next, so a fast double-press cannot land on top of itself.
  const queueRef = useRef([]);
  const busyRef = useRef(false);

  const send = useCallback((name) => {
    const header = headerRef.current;
    if (!header || KEYS[name] === undefined) return;
    Atomics.store(header, 1, KEYS[name]);
    busyRef.current = true;
    Atomics.add(header, 0, 1);
    Atomics.notify(header, 0);
  }, []);

  // One rAF loop: copy the shared framebuffer to the canvas whenever the worker
  // says it has written a new one.
  const pump = useCallback(() => {
    const header = headerRef.current;
    const pixels = pixelsRef.current;
    const canvas = canvasRef.current;

    if (header && pixels && canvas) {
      const seq = Atomics.load(header, 2);
      if (seq !== drawnRef.current) {
        drawnRef.current = seq;
        const context = canvas.getContext("2d");
        const image = context.createImageData(WIDTH, HEIGHT);
        image.data.set(pixels);
        context.putImageData(image, 0, 0);

        // The worker drew the frame for the last key, so it is waiting again.
        busyRef.current = false;
        const next = queueRef.current.shift();
        if (next !== undefined) {
          Atomics.store(header, 1, KEYS[next]);
          busyRef.current = true;
          Atomics.add(header, 0, 1);
          Atomics.notify(header, 0);
        }
      }
    }
    frameRef.current = requestAnimationFrame(pump);
  }, []);

  useEffect(() => {
    let cancelled = false;
    const base = new URL("sim/", document.baseURI).href;

    // A SharedArrayBuffer needs a cross-origin isolated page (see the headers in
    // site/vercel.json); without it the module cannot talk to the page.
    if (typeof SharedArrayBuffer === "undefined") {
      setState("no-shared-memory");
      return undefined;
    }

    fetch(new URL("bench.wasm", base).href, { method: "HEAD" })
      .then((response) => {
        if (!response.ok) throw new Error("no build");
        if (!response.headers.get("content-length")) return;
      })
      .then(() => {
        if (cancelled) return;
        const sab = new SharedArrayBuffer(HEADER_BYTES + WIDTH * HEIGHT * 4);
        headerRef.current = new Int32Array(sab, 0, 8);
        headerRef.current[3] = WIDTH;
        headerRef.current[4] = HEIGHT;
        pixelsRef.current = new Uint8ClampedArray(sab, HEADER_BYTES, WIDTH * HEIGHT * 4);

        const worker = new Worker(new URL("worker.js", base).href, { type: "module" });
        workerRef.current = worker;
        worker.onmessage = (event) => {
          if (cancelled) return;
          if (event.data?.type === "ready") {
            setState("ready");
            frameRef.current = requestAnimationFrame(pump);
          } else if (event.data?.type === "failed") {
            setState("absent");
          }
        };
        worker.onerror = () => {
          if (!cancelled) setState("absent");
        };
        worker.postMessage({
          type: "start",
          sab,
          wasm: new URL("bench.wasm", base).href,
          demo: new URL("demo.json", base).href,
          shim: new URL("browser_wasi_shim/index.js", base).href,
          width: WIDTH,
          height: HEIGHT,
        });
      })
      .catch(() => {
        if (!cancelled) setState("absent");
      });

    return () => {
      cancelled = true;
      cancelAnimationFrame(frameRef.current);
      workerRef.current?.terminate();
      workerRef.current = null;
    };
  }, [pump, send]);

  const press = (name) => {
    if (KEYS[name] === undefined) return;
    if (busyRef.current) {
      // Keep the last few, in order; anything beyond that is a held key.
      if (queueRef.current.length < 4) queueRef.current.push(name);
      return;
    }
    send(name);
  };

  if (state === "absent" || state === "no-shared-memory") {
    return (
      <div className="mt-8 border-3 border-ink bg-paper p-5">
        <p className="font-mono text-xs uppercase tracking-widest text-ink/60">
          Interactive build
        </p>
        <p className="mt-2 max-w-2xl text-sm leading-relaxed">
          {state === "no-shared-memory"
            ? "This page needs cross-origin isolation for the shared buffer the add-in runs on, and the deployment did not send those headers."
            : "Not deployed in this build."}{" "}
          Everything below is still real output from the same C sources.
        </p>
      </div>
    );
  }

  return (
    <div className="mt-8">
      <div className="inline-block border-3 border-ink bg-ink p-1 shadow-brute">
        <canvas
          ref={canvasRef}
          width={WIDTH}
          height={HEIGHT}
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
        {state === "ready"
          ? "Same C as the calculator - keys go straight into the add-in"
          : "Loading the add-in..."}
      </p>
    </div>
  );
}
