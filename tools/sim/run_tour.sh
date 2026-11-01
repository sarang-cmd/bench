#!/bin/sh
# run_tour.sh - render every screen of Bench to a PPM/PNG, on a PC.
#
#   tools/sim/run_tour.sh [outdir]
#
# A scratch filesystem is built in /tmp/bench-tour-fs from docs/samples, so the
# tour never reads or writes anything of yours.  Each step is one short key
# script from tools/sim/tour/, run from the menu, which keeps every screenshot
# independent of the previous one.
#
# Output: <outdir>/<step>.ppm and, when Pillow is installed, <outdir>/<step>.png
# (scaled 2x with nearest-neighbour so the pixels stay square).

set -e

root=$(cd "$(dirname "$0")/../.." && pwd)
out=${1:-$root/docs/images}
sim=$root/build/sim/bench
fs=/tmp/bench-tour-fs

[ -x "$sim" ] || { echo "build the simulator first: make sim"; exit 1; }

rm -rf "$fs"
mkdir -p "$fs/notes" "$fs/cards" "$fs/state" "$fs/formulas"
# State files the tour needs to show a real screen (the six expressions).
[ -f "$root/docs/samples/state/exprs.txt" ] &&
    cp "$root/docs/samples/state/exprs.txt" "$fs/state/"
# The formula booklets, so the Formula book screen has something to show.
[ -d "$root/docs/samples/formulas" ] &&
    cp "$root"/docs/samples/formulas/*.txt "$fs/formulas/"
# Every sample, so the worst-case screens have their files too.
for f in "$root"/docs/samples/*; do
    [ -f "$f" ] || continue
    case "$f" in
        *cards*.txt) cp "$f" "$fs/cards/" 2>/dev/null || true ;;
        *) cp "$f" "$fs/notes/" 2>/dev/null || true ;;
    esac
done
mkdir -p "$out"

for step in "$root"/tools/sim/tour/*.txt; do
    name=$(basename "$step" .txt)
    BENCH_FS=$fs BENCH_KEYS=$step BENCH_SHOT=$out/$name.ppm "$sim"
    echo "  $name"
done

python3 "$root/tools/sim/ppm2png.py" "$out" || {
    echo "  (Pillow missing, PPM only)"
}
