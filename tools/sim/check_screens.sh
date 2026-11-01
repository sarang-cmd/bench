#!/bin/sh
# check_screens.sh - run every key script through the overlap and contrast
# checks in tools/sim.
#
#   tools/sim/check_screens.sh
#
# Each script gets a scratch filesystem built from docs/samples, exactly like
# run_tour.sh, so the checks see the same content the screenshots do.  A script
# that trips a check exits non-zero and the whole run fails.

set -e

root=$(cd "$(dirname "$0")/../.." && pwd)
sim=$root/build/sim/bench
fs=/tmp/bench-checkfs

[ -x "$sim" ] || { echo "build the simulator first: make sim"; exit 1; }

rm -rf "$fs"
mkdir -p "$fs/notes" "$fs/cards" "$fs/state"
# State files the tour needs to show a real screen (the six expressions).
[ -f "$root/docs/samples/state/exprs.txt" ] &&
    cp "$root/docs/samples/state/exprs.txt" "$fs/state/"
# A Bench folder copied into @MainMem ends up in SAVE-F; one script shows that.
mkdir -p "$fs/SAVE-F/Bench/notes"
if [ -d "$root/docs/samples/savef/notes" ]; then
    cp "$root"/docs/samples/savef/notes/* "$fs/SAVE-F/Bench/notes/" 2>/dev/null || true
fi
for f in "$root"/docs/samples/*; do
    [ -f "$f" ] || continue
    case "$f" in
        *cards*.txt) cp "$f" "$fs/cards/" ;;
        *) cp "$f" "$fs/notes/" ;;
    esac
done

# 99_* steps run against an empty card, so what they show is the seed and
# nothing else (see run_tour.sh).
fs_fresh=/tmp/bench-checkfs-fresh
rm -rf "$fs_fresh"
mkdir -p "$fs_fresh"

fail=0
count=0
for step in "$root"/tools/sim/tour/*.txt; do
    name=$(basename "$step" .txt)
    count=$((count + 1))
    case "$name" in
        99_*) card=$fs_fresh; no_starterpack= ;;
        *)    card=$fs;        no_starterpack=1 ;;   # the empty states: no seed
    esac
    if ! out=$(BENCH_NO_STARTERPACK=$no_starterpack BENCH_FS=$card BENCH_KEYS=$step \
               BENCH_CHECK=1 BENCH_NAME=$name "$sim" 2>&1); then
        printf '%s\n' "$out"
        fail=$((fail + 1))
    fi
done

if [ "$fail" -ne 0 ]; then
    echo "SCREEN CHECKS FAILED ($fail of $count scripts)"
    exit 1
fi
echo "SCREEN CHECKS PASSED ($count scripts: no overlaps, no labels on borders, all text readable)"
