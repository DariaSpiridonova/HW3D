#!/usr/bin/env bash
# Shows everything: the checks, the examples, the measurement of the time
# usage: ./demo.sh [BUILD_DIR]       (BUILD_DIR is "build" by default)

BUILD=${1:-build}
ROOT="$(cd "$(dirname "$0")" && pwd)"
[[ -x $BUILD/hw3d ]] || { echo "build first: cmake -S . -B build && cmake --build build -j"; exit 1; }

step() { echo; echo "=================== $1"; }

step "1. Unit checks of the geometry (vectors, triangles, exact test, boxes, tree, scene)"
"$BUILD/check_project"

step "2. Unit checks of the viewer data (Vertex, the .vtx file, matrices, camera)"
"$BUILD/check_render"

step "3. Tests of the command line: .in file -> the expected .out (file and standard input)"
bash "$ROOT/tests/run_cli_tests.sh" "$BUILD/hw3d"

step "4. Examples: the numbers of the intersecting triangles"
echo "examples/small3d.txt  : $("$BUILD/hw3d" --list "$ROOT/examples/small3d.txt" | tr '\n' ' ')   (red in the window)"
echo "examples/small2d.txt  : $("$BUILD/hw3d" --list "$ROOT/examples/small2d.txt" | tr '\n' ' ')"

step "5. Saving the vertices to a file"
"$BUILD/hw3d" --save /tmp/demo.vtx --list "$ROOT/examples/small3d.txt" > /dev/null
echo "/tmp/demo.vtx: $(stat -c %s /tmp/demo.vtx) bytes = 16 (header) + 7 triangles * 3 vertices * 28"

step "6. Time of the whole program on random scenes (3D and 2D)"
bash "$ROOT/tools/measure.sh" "$BUILD" 10000 100000 1000000

step "7. How the time grows when N doubles (it must grow a bit more than twice: N log N)"
"$BUILD/bench_scene" 50000 100000 200000 400000

echo
echo "=================== The window (run it by hand, it needs a screen)"
echo "  $BUILD/hw3d examples/small3d.txt"
echo "  $BUILD/hw3d --view /tmp/demo.vtx          # the same from the saved file"
echo "  $BUILD/generate_scene 20000 3 > big.txt && $BUILD/hw3d big.txt"
