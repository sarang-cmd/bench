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
mkdir -p "$fs/fls0/Bench/notes" "$fs/fls0/Bench/cards" \
         "$fs/fls0/Bench/state" "$fs/fls0/Bench/formulas" "$fs/fls0/Bench/study"
# State files the tour needs to show a real screen (the six expressions).
[ -f "$root/docs/samples/state/exprs.txt" ] &&
    cp "$root/docs/samples/state/exprs.txt" "$fs/fls0/Bench/state/"
# The formula booklets, so the Formula book screen has something to show.
[ -d "$root/docs/samples/formulas" ] &&
    cp "$root"/docs/samples/formulas/*.txt "$fs/fls0/Bench/formulas/"
# Every sample, so the worst-case screens have their files too.
# A Bench folder copied into @MainMem ends up in SAVE-F; one script shows that.
mkdir -p "$fs/fls0/SAVE-F/Bench/notes"
if [ -d "$root/docs/samples/savef/notes" ]; then
    cp "$root"/docs/samples/savef/notes/* "$fs/fls0/SAVE-F/Bench/notes/" 2>/dev/null || true
fi
for f in "$root"/docs/samples/*; do
    [ -f "$f" ] || continue
    case "$f" in
        *cards*.txt) cp "$f" "$fs/fls0/Bench/cards/" 2>/dev/null || true ;;
        *) cp "$f" "$fs/fls0/Bench/notes/" 2>/dev/null || true ;;
    esac
done
mkdir -p "$out"

# What a first run shows: an empty card, and the add-in seed it.  A step named
# 99_* gets this filesystem; the rest share the one built above, with the
# starter pack switched off (BENCH_NO_STARTERPACK, a PC-build switch in
# src/platform/starter.c), because most steps are about the sample files and
# the "nothing yet" screens.
fs_fresh=/tmp/bench-tour-fresh
rm -rf "$fs_fresh"
mkdir -p "$fs_fresh/fls0"

for step in "$root"/tools/sim/tour/*.txt; do
    name=$(basename "$step" .txt)
    case "$name" in
        99_*) card=$fs_fresh; no_starterpack= ;;
        *)    card=$fs;        no_starterpack=1 ;;
    esac
    BENCH_NO_STARTERPACK=$no_starterpack BENCH_FS=$card BENCH_KEYS=$step \
        BENCH_SHOT=$out/$name.ppm "$sim"
    echo "  $name"
done

python3 "$root/tools/sim/ppm2png.py" "$out" || {
    echo "  (Pillow missing, PPM only)"
}
