#!/bin/sh
# build.sh - compile the add-in and the PC stand-in to WebAssembly.
#
#   tools/wasm/build.sh [outdir]        (default: build-wasm)
#
# This is the browser build of exactly the sources `make sim` uses: the add-in
# (src/) plus tools/sim/sim.c, which stands in for gint.  The only difference is
# that keys arrive from JavaScript and the frame is pulled out as RGBA; see the
# __EMSCRIPTEN__ blocks in tools/sim/sim.c.
#
# Needs emscripten (emcc) on PATH.  Nothing in this repository requires it: CI
# has an optional job that produces the binary, and the site shows the gallery
# when the binary is not there.
#
# Asyncify is required because the add-in's event loop blocks inside getkey();
# Asyncify turns that sleep into a return to the browser, so the page stays
# responsive while the add-in waits for a key.
set -e

root=$(cd "$(dirname "$0")/../.." && pwd)
out=${1:-$root/build-wasm}

command -v emcc >/dev/null 2>&1 || {
    echo "build.sh: emcc not found - install emsdk, or skip this build" >&2
    exit 1
}

mkdir -p "$out"

# The demo content is preloaded into the in-memory filesystem, so the browser
# build opens on a calculator with something on it instead of empty folders.
mkdir -p "$out/fs/notes" "$out/fs/cards" "$out/fs/state" "$out/fs/study"
cp "$root"/demo/notes/*.md "$out/fs/notes/" 2>/dev/null || true
cp "$root"/demo/cards/*.txt "$out/fs/cards/" 2>/dev/null || true
cp "$root"/demo/study/log.txt "$out/fs/study/" 2>/dev/null || true

# src/main.c is the add-in's entry point; rename its main() so the stand-in's
# main() is the only one, exactly as the desktop build does.
emcc -std=gnu11 -O2 -DBENCH_TRACE -Dmain=bench_main \
     -Isrc -Isrc/ui -Isrc/core -Isrc/apps -Itools/sim/include \
     -c "$root/src/main.c" -o "$out/main.o"

# shellcheck disable=SC2086
emcc -std=gnu11 -O2 -DBENCH_TRACE \
     -Isrc -Isrc/ui -Isrc/core -Isrc/apps -Itools/sim/include \
     -sASYNCIFY=1 -sALLOW_MEMORY_GROWTH=1 \
     -sEXPORTED_FUNCTIONS='["_bench_wasm_run","_bench_wasm_key","_bench_wasm_frame","_bench_wasm_width","_bench_wasm_height","_malloc","_free"]' \
     -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
     -sMODULARIZE=1 -sEXPORT_NAME=BenchSim \
     --preload-file "$out/fs@/benchfs" \
     -o "$out/bench.js" \
     "$out/main.o" \
     $(find "$root/src/core" "$root/src/platform" "$root/src/ui" "$root/src/apps" \
            -name '*.c' ! -name main.c | sort) \
     "$root/tools/sim/sim.c"

rm -f "$out/main.o"
echo "built $out/bench.js and $out/bench.wasm"
echo "copy both into site/public/sim/ to serve them (site/scripts/copy-wasm.mjs does it)"
