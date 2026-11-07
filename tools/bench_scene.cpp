#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
#include <vector>

#include "scene/scene.h"

/*
    Benchmark of Scene::find_intersecting
    Build: cmake --build build (the target bench_scene)
    Run:   ./build/bench_scene            (default sizes)
       ./build/bench_scene 50000 100000   (your own sizes)
*/

// Time of one call of f in milliseconds
template <typename F>
double time_ms(F&& f)
{
    const auto start = std::chrono::steady_clock::now();   // steady_clock never goes back, system_clock can
    f();
    const auto stop = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

// The best of several runs: other programs and the CPU frequency only make a run slower,
// so the minimum is the closest to the real speed of the code
template <typename F>
double best_of(int runs, F&& f)
{
    double best = std::numeric_limits<double>::max();
    for (int i = 0; i < runs; ++i) best = std::min(best, time_ms(f));
    return best;
}

// Generate a vector from n triangles 
std::vector<Triangle<double, 3>> random_triangles(int n, double side, double size, unsigned seed)
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> position(0, side), offset(-size, size);
    std::vector<Triangle<double, 3>> triangles;
    triangles.reserve(n);
    while (static_cast<int>(triangles.size()) < n)
    {
        const Vector<double, 3> center(position(rng), position(rng), position(rng));
        const Vector<double, 3> d1(offset(rng), offset(rng), offset(rng));
        const Vector<double, 3> d2(offset(rng), offset(rng), offset(rng));
        const Vector<double, 3> d3(offset(rng), offset(rng), offset(rng));
        Triangle<double, 3> triangle(center + d1, center + d2, center + d3);
        if (!triangle.is_degenerate()) triangles.push_back(triangle);
    }
    return triangles;
}

int main(int argc, char** argv)
{
    std::vector<int> sizes;
    if (argc > 1)
        for (int i = 1; i < argc; ++i) 
            sizes.push_back(std::atoi(argv[i]));
    else 
        // set default values
        sizes = {25000, 50000, 100000, 200000, 400000, 800000};


    std::printf("%10s %12s %16s %10s\n", "N", "time, ms", "us per triangle", "x slower");
    double previous = 0;
    for (int n : sizes)
    {
        // the volume grows with n, so the density of triangles is the same for every n
        const double side = std::cbrt(static_cast<double>(n)) * 3;

        // generate the scene
        const Scene<3> scene(random_triangles(n, side, 2, 1));

        // we use the result (count the true flags), otherwise the compiler may decide that the work is not needed
        std::size_t flagged = 0;
        // calculate the best time of the intersection search algorithm among 3 runs
        const double ms = best_of(3, [&]
        {
            const std::vector<bool> flags = scene.find_intersecting();
            flagged = std::count(flags.begin(), flags.end(), true);
        });

        std::printf("%10d %12.1f %16.3f", n, ms, ms * 1000 / n);
        if (previous > 0) std::printf(" %9.2fx", ms / previous);
        std::printf("   (flagged %zu)\n", flagged);
        previous = ms;
    }
}
