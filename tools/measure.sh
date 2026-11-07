#!/usr/bin/env bash
# The time of the whole program (reading the file + the search of the intersections) on random scenes of growing size
# usage: tools/measure.sh BUILD_DIR [N ...]        by default N = 10000 100000 1000000
BUILD=${1:?usage: measure.sh BUILD_DIR [N ...]}
shift
# store scene sizes in an array, if no sizes are provided, use default sizes
SIZES=("$@")
[[ ${#SIZES[@]} -eq 0 ]] && SIZES=(10000 100000 1000000)

# create a temporary directory to store the generated scenes, and ensure it is deleted on exit
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

printf "%-4s %9s %10s %12s %12s\n" dim N "file, MB" "time, ms" "intersecting"
for dim in 3 2; do
    for n in "${SIZES[@]}"; do
        "$BUILD/generate_scene" "$n" "$dim" 1 > "$TMP/scene.txt"
        start=$(date +%s%N)
        red=$("$BUILD/hw3d" --list "$TMP/scene.txt" | wc -l)
        stop=$(date +%s%N)
        size=$(stat -c %s "$TMP/scene.txt")
        printf "%-4s %9d %10.1f %12d %12d\n" "${dim}D" "$n" "$(awk -v s="$size" 'BEGIN { print s / 1048576 }')" "$(( (stop - start) / 1000000 ))" "$red"
    done
done
