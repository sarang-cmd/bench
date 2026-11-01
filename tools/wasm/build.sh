#!/bin/sh
# build.sh - compile the add-in and the PC stand-in to WebAssembly.
#
#   tools/wasm/build.sh [outdir]        (default: build-wasm)
#
# This is the browser build of exactly the sources `make sim` uses: the add-in
# (src/) plus tools/sim/sim.c, which stands in for gint.  The only difference is
# where keys come from and where the frame goes; see tools/sim/bench_wasm.h.
#
# Needs zig (0.13 or newer), which carries clang, wasm-ld and wasi-libc; the
# PyPI package `ziglang` is one `pip install` and needs no root.  Nothing in
# this repository requires it: the site shows the screenshot gallery when the
# binary is not there, and the optional CI job builds it.
#
# Why zig and not emscripten: emscripten's Asyncify exists to return from a
# blocking call back to the browser's main thread.  The page here runs the
# module in a Web Worker and talks to it through a SharedArrayBuffer, so a
# blocking getkey() is simply a blocked worker thread - no transform, no 40 MB
# SDK, and the same C.
#
# Output, all of it copied to site/public/sim/ by site/scripts/copy-wasm.mjs:
#   bench.wasm   the add-in plus the stand-in, WASI reactor
#   demo.json    the demo notes, decks and study log, base64, for the in-memory
#                filesystem the worker builds (the wasm has no real disk)
set -e

root=$(cd "$(dirname "$0")/../.." && pwd)
out=${1:-$root/build-wasm}
zig=${ZIG:-zig}

command -v "$zig" >/dev/null 2>&1 || {
    echo "build.sh: zig not found - pip install ziglang, or set ZIG=/path/to/zig" >&2
    exit 1
}

mkdir -p "$out"
obj="$out/obj"
rm -rf "$obj"
mkdir -p "$obj"

cflags="-target wasm32-wasi -O2 -DBENCH_WASM -DBENCH_TRACE -std=gnu11 -w
        -I$root/src -I$root/src/ui -I$root/src/core -I$root/src/apps
        -I$root/tools/sim/include -I$root/tools/sim"

compile() {
    # $1 source, $2 object name
    # shellcheck disable=SC2086
    "$zig" cc $cflags "$1" -c -o "$obj/$2.o"
}

# src/main.c is the add-in's entry point; rename its main() so the stand-in's
# own entry stays the only one, exactly as the desktop build does (where the
# stand-in's main() is the program).
# shellcheck disable=SC2086
"$zig" cc $cflags -Dmain=bench_main "$root/src/main.c" -c -o "$obj/main.o"

for f in "$root"/src/core/*.c "$root"/src/platform/*.c "$root"/src/ui/*.c "$root"/src/apps/*.c; do
    base=$(basename "$f" .c)
    [ "$base" = "main" ] && continue
    compile "$f" "$base"
done
compile "$root/tools/sim/sim.c" sim

# shellcheck disable=SC2086
"$zig" cc -target wasm32-wasi -O2 -mexec-model=reactor \
    -Wl,--export=bench_wasm_run \
    -Wl,--export=bench_wasm_frame \
    -Wl,--export=bench_wasm_width \
    -Wl,--export=bench_wasm_height \
    -Wl,--export=malloc -Wl,--export=free \
    -Wl,--export=__wasm_call_ctors -Wl,--export=__stack_pointer \
    -Wl,--export-memory \
    $( [ -n "$BENCH_WASM_DEBUG" ] || echo -Wl,--strip-all ) \
    $( [ -n "$BENCH_WASM_DEBUG" ] && echo -g ) \
    -o "$out/bench.wasm" "$obj"/*.o

# The demo content as a JSON map of path -> base64, so the worker can put it in
# the in-memory filesystem before the add-in starts.  Built here rather than in
# JS so the wasm and the files it opens always come from the same run.
python3 - "$root" "$out/demo.json" <<'PY'
import base64, json, os, sys

root, dest = sys.argv[1], sys.argv[2]
files = {}
for folder in ("notes", "cards", "study"):
    path = os.path.join(root, "demo", folder)
    if not os.path.isdir(path):
        continue
    for name in sorted(os.listdir(path)):
        full = os.path.join(path, name)
        if os.path.isfile(full):
            with open(full, "rb") as fh:
                files[f"{folder}/{name}"] = base64.b64encode(fh.read()).decode("ascii")
with open(dest, "w", encoding="utf-8") as fh:
    json.dump(files, fh, indent=0, sort_keys=True)
    fh.write("\n")
print(f"demo.json: {len(files)} files")
PY

rm -rf "$obj"
ls -l "$out/bench.wasm"
echo "built $out/bench.wasm - copy it, with demo.json and the page's worker, by"
echo "running: node site/scripts/copy-wasm.mjs"
