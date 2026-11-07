# HW3D

Intersecting triangles in 2D/3D: the exact test, a bounding volume hierarchy, a scene reader, a viewer.

## Ubuntu

    sudo apt update
    sudo apt install build-essential cmake libglfw3-dev libglew-dev mesa-utils

    cmake -S . -B build
    cmake --build build -j

    ./build/check_project            # checks of the geometry
    ./build/check_render             # checks of the data for the viewer and of the camera
    ./build/hw3d examples/small3d.txt          # the window
    ./build/hw3d --list examples/small3d.txt   # only the numbers of the intersecting triangles
    ./build/hw3d --save scene.vtx examples/small3d.txt   # also save the vertices for the video card to a file
    ./build/hw3d --view scene.vtx              # show the vertices from the file, the scene is not read
    ./build/generate_scene 5000 3 > scene.txt  # a random scene, then: ./build/hw3d scene.txt

Input format: `triangles_num dimension`, then `triangles_num` triangles, 3 * dimension numbers each.
The compiler must support C++20 (g++ 11 or newer).

## Viewer keys

    W S A D     fly forward, back, left, right      Q E   down, up      Shift   faster
    left mouse button + move the mouse              look around
    O           the camera flies around the scene by itself
    R           back to the start                   Esc   exit

## Files

    src/geometry/primitives/   eps, vec (vector), triangle, plane
    src/geometry/intersect/    the exact test of two triangles: intersect2D, intersect3D
    src/geometry/accel/        aabb (boxes) and bvh (the tree of boxes)
    src/geometry/scene/        scene: reading, the flags "intersecting"
    src/render/     Vertex, the file .vtx, the camera math (no OpenGL) and viewer.cpp (the only file with OpenGL)
    shaders/        triangle.vert, triangle.frag, line.vert, line.frag: the programs for the video card
    tools/          hello_window.cpp (a check of the environment), generate_scene.cpp (random scenes)
    tests/          check_project.cpp, check_render.cpp

## Testing and measuring

    ctest --test-dir build --output-on-failure   # all the tests at once (geometry, render, command line)
    ./demo.sh build                              # shows everything: the checks, the examples, the time
    tests/run_cli_tests.sh build/hw3d            # only the tests of the command line: tests/data/NAME.in -> NAME.out
    tools/measure.sh build 10000 100000 1000000  # time of the whole program on random scenes
    ./build/bench_scene 50000 100000 200000      # time of the search only, and how it grows with N

    cmake -S . -B build-san -DHW3D_SANITIZE=ON   # the same with AddressSanitizer and UBSan
    cmake --build build-san -j && ctest --test-dir build-san --output-on-failure
