// CalcKeypad.jsx - the fx-CG 50 keypad, laid out where the real keys are.
//
// The old pad was a flat 5-column grid that followed no physical keyboard: DEL
// sat next to 9, ALPHA sat where x² is, and the arrow keys were scattered over
// two rows.  Anyone who owns the calculator had to hunt for every key.
//
// This one is the calculator: the six function keys, the SHIFT/OPTN/VARS/MENU
// block, the ALPHA block, the four-way pad on the right where the real one sits,
// and the numeric block as 7-8-9-DEL-AC / 4-5-6-×-÷ / 1-2-3-+-(−) /
// 0-.-×10^x-(-)-EXE.
//
// Only the keys the add-in actually reads are buttons.  The rest are drawn as
// muted hardware, which is honest twice over: they are real keys on the
// calculator, and pressing them here does nothing because they do nothing in
// Bench either.
const HARDWARE = null;

const FN_ROW = [
  ["F1", "f1", "Trace"],
  ["F2", "f2", "Zoom"],
  ["F3", "f3", "V-Window"],
  ["F4", "f4", "Sketch"],
  ["F5", "f5", "G-Solv"],
  ["F6", "f6", "G↔T"],
];

const APP_BLOCK = [
  ["SHIFT", HARDWARE],
  ["OPTN", HARDWARE],
  ["VARS", HARDWARE],
  ["MENU", "menu"],
  ["ALPHA", "alpha"],
  ["x²", HARDWARE],
  ["^", HARDWARE],
  ["EXIT", "exit"],
  ["x,θ,t", HARDWARE],
  ["log", HARDWARE],
  ["ln", HARDWARE],
  ["(", HARDWARE],
  [")", HARDWARE],
  [",", HARDWARE],
  ["→", HARDWARE],
  ["M+", HARDWARE],
];

const NUM_BLOCK = [
  ["7", "7"],
  ["8", "8"],
  ["9", "9"],
  ["DEL", "del"],
  ["AC/ON", "exit"],
  ["4", "4"],
  ["5", "5"],
  ["6", "6"],
  ["×", HARDWARE],
  ["÷", HARDWARE],
  ["1", "1"],
  ["2", "2"],
  ["3", "3"],
  ["+", HARDWARE],
  ["−", HARDWARE],
  ["0", "0"],
  [".", "dot"],
  ["×10ˣ", HARDWARE],
  ["(−)", "neg"],
  ["EXE", "exe"],
];

// The four-way pad, as the 3x3 block it physically is.
const DPAD = [
  [null, ["up", "\u2191"], null],
  [["left", "\u2190"], null, ["right", "\u2192"]],
  [null, ["down", "\u2193"], null],
];

const ARROWS = { up: "Up", down: "Down", left: "Left", right: "Right" };

function Key({ label, keyName, press }) {
  if (!keyName) {
    return (
      <span
        aria-hidden="true"
        title="On the calculator, but Bench does not read it"
        className="flex select-none items-center justify-center border-2 border-ink/45 bg-ink/10 px-1 py-1.5 font-mono text-[9px] font-bold leading-none text-ink/50 sm:text-[10px]"
      >
        {label}
      </span>
    );
  }

  return (
    <button
      type="button"
      onClick={() => press(keyName)}
      aria-label={ARROWS[keyName] ? undefined : label}
      className="flex select-none items-center justify-center border-2 border-ink bg-paper px-1 py-1.5 font-mono text-[10px] font-black leading-none text-ink shadow-[2px_2px_0_0_#2f2d2e] hover:bg-yellow focus:outline-none focus-visible:ring-2 focus-visible:ring-blue active:translate-x-[2px] active:translate-y-[2px] active:shadow-none sm:text-[11px]"
    >
      {label}
    </button>
  );
}

function Arrow({ name, glyph, press }) {
  return (
    <button
      type="button"
      onClick={() => press(name)}
      aria-label={ARROWS[name]}
      className="flex h-8 w-8 items-center justify-center border-2 border-ink bg-paper font-mono text-xs font-black shadow-[2px_2px_0_0_#2f2d2e] hover:bg-yellow focus:outline-none focus-visible:ring-2 focus-visible:ring-blue active:translate-x-[2px] active:translate-y-[2px] active:shadow-none sm:h-9 sm:w-9"
    >
      {glyph}
    </button>
  );
}

export default function CalcKeypad({ press }) {
  return (
    <div className="mt-4 w-full max-w-[420px] border-3 border-ink bg-shade p-2 shadow-brute sm:p-3">
      {/* The calculator's own name plate. */}
      <div className="mb-2 flex items-baseline justify-between font-mono text-[10px] font-bold uppercase tracking-[0.18em] text-ink/70">
        <span>Casio</span>
        <span>fx-CG50</span>
      </div>

      {/* Six function keys, with the labels the case prints under them. */}
      <div className="grid grid-cols-6 gap-1">
        {FN_ROW.map(([label, keyName, caption]) => (
          <div key={label} className="flex flex-col gap-0.5">
            <Key label={label} keyName={keyName} press={press} />
            <span className="text-center font-mono text-[7px] uppercase tracking-wider text-ink/55 sm:text-[8px]">
              {caption}
            </span>
          </div>
        ))}
      </div>

      {/* App block on the left, four-way pad on the right where the real one is. */}
      <div className="mt-2 grid grid-cols-[1fr_auto] gap-2">
        <div className="grid grid-cols-4 gap-1">
          {APP_BLOCK.map(([label, keyName]) => (
            <Key key={label} label={label} keyName={keyName} press={press} />
          ))}
        </div>

        <div className="grid grid-cols-3 grid-rows-3 gap-1">
          {DPAD.flat().map((cell, index) =>
            cell ? (
              <Arrow key={cell[0]} name={cell[0]} glyph={cell[1]} press={press} />
            ) : (
              <span key={`gap-${index}`} />
            ),
          )}
        </div>
      </div>

      {/* The numeric block, exactly as the case lays it out. */}
      <div className="mt-2 grid grid-cols-5 gap-1">
        {NUM_BLOCK.map(([label, keyName]) => (
          <Key key={label} label={label} keyName={keyName} press={press} />
        ))}
      </div>
    </div>
  );
}
