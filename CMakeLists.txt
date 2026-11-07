cmake_minimum_required(VERSION 3.16)
project(hw3d CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Release by default; we do not add -DNDEBUG, so the asserts stay on
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
add_compile_options(-Wall -Wextra)

# ctest runs everything that is registered with add_test below
enable_testing()

# Build with the checks of the memory and of the undefined behavior: cmake -S . -B build-san -DHW3D_SANITIZE=ON
option(HW3D_SANITIZE "Build with AddressSanitizer and UBSan" OFF)
if(HW3D_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# The geometry and the data for the viewer are header only: libraries without sources, they only give include folders
add_library(geometry INTERFACE)
target_include_directories(geometry INTERFACE ${CMAKE_SOURCE_DIR}/src/geometry)

add_library(render_data INTERFACE)
target_include_directories(render_data INTERFACE ${CMAKE_SOURCE_DIR}/src/render)
target_link_libraries(render_data INTERFACE geometry)

# Checks (no graphics needed)
add_executable(check_project tests/check_project.cpp)
target_link_libraries(check_project PRIVATE geometry)

add_executable(check_render tests/check_render.cpp)
target_link_libraries(check_render PRIVATE render_data)

# Random scenes for the program and for the benchmarks
add_executable(generate_scene tools/generate_scene.cpp)

# The benchmark of the search of the intersections on scenes of growing size
add_executable(bench_scene tools/bench_scene.cpp)
target_link_libraries(bench_scene PRIVATE geometry)

# The program itself
add_executable(hw3d src/main.cpp)
target_link_libraries(hw3d PRIVATE render_data)

# The window is built only if GLFW, GLEW and OpenGL are installed, without them hw3d only prints the numbers
find_package(OpenGL QUIET)
find_package(glfw3 3.3 QUIET)
find_package(GLEW QUIET)

if(TARGET OpenGL::GL AND TARGET glfw AND TARGET GLEW::GLEW)
    target_sources(hw3d PRIVATE src/render/viewer.cpp)
    # HW3D_SHADER_DIR: where the program looks for the shader files (.vert, .frag), it works from any folder
    target_compile_definitions(hw3d PRIVATE HW3D_GRAPHICS HW3D_SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders")
    target_link_libraries(hw3d PRIVATE glfw GLEW::GLEW OpenGL::GL)

    # The check of the environment: a window and one triangle
    add_executable(hello_window tools/hello_window.cpp)
    target_link_libraries(hello_window PRIVATE glfw GLEW::GLEW OpenGL::GL)
else()
    message(STATUS "GLFW, GLEW or OpenGL not found: hw3d has no window, hello_window is not built")
endif()

# The tests for ctest: two sets of unit checks and the tests of the command line (.in and .out files)
add_test(NAME geometry COMMAND check_project)
add_test(NAME render COMMAND check_render)
add_test(NAME command_line COMMAND bash ${CMAKE_SOURCE_DIR}/tests/run_cli_tests.sh $<TARGET_FILE:hw3d>)
