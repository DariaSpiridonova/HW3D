#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>

/*
    Generates a random scene in the input format of hw3d
    generate_scene N DIMENSION [seed] [triangle_size]  > scene.txt
    The density of the triangles is the same for any N: the cube grows with N
*/
int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::fprintf(stderr, "usage: generate_scene N DIMENSION(2 or 3) [seed] [triangle_size]\n");
        return 1;
    }
    const long long n = std::atoll(argv[1]);
    const int dimension = std::atoi(argv[2]);
    const unsigned seed = argc > 3 ? static_cast<unsigned>(std::atoi(argv[3])) : 1u;
    const double size = argc > 4 ? std::atof(argv[4]) : 2.0;
    if (n < 0 || (dimension != 2 && dimension != 3))
    {
        std::fprintf(stderr, "N must not be negative, DIMENSION must be 2 or 3\n");
        return 1;
    }

    const double side = (dimension == 3 ? std::cbrt(static_cast<double>(n)) : std::sqrt(static_cast<double>(n))) * 3;
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> position(0, side), offset(-size, size);

    std::printf("%lld %d\n", n, dimension);
    for (long long i = 0; i < n; ++i)
    {
        double center[3] = {position(rng), position(rng), position(rng)};
        for (int vertex = 0; vertex < 3; ++vertex)
        {
            for (int k = 0; k < dimension; ++k) std::printf("%.6g ", center[k] + offset(rng));
        }
        std::printf("\n");
    }
}
