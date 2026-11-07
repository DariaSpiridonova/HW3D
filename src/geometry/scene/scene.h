#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <istream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "accel/aabb.h"
#include "accel/bvh.h"
#include "intersect/intersect2D.h"
#include "intersect/intersect3D.h"
#include "primitives/triangle.h"
#include "primitives/vec.h"

/*
    Scene: a set of triangles in 2D or in 3D
    It reads them from a stream and finds which of them intersect other triangles
    It knows nothing about OpenGL: the renderer only takes triangles, flags and bounds from it

    Input format:
        triangles_num dimension
        then triangles_num triangles, each one is 3 * dimension numbers:
        the coordinates of the first point, of the second point, of the third point
    The numbers are separated by any whitespace
*/

namespace detailsScene
{
    // Read the whole stream into a string
    inline std::string read_all(std::istream& in)
    {
        std::string text; // result string
        char buffer[1 << 16]; // the size of the data read at a time
        while (in)
        {
            in.read(buffer, sizeof(buffer));
            text.append(buffer, static_cast<std::size_t>(in.gcount())); // add to result string
        }
        return text;
    }

    // Read the next number from [pos, end) and move pos right after it
    // from_chars is much faster than operator>>, which matters for millions of numbers
    template <typename Number>
    Number next_number(const char*& pos, const char* end)
    {
        // skip whitespace and the sign +, from_chars accepts neither of them
        while (pos != end && (std::isspace(static_cast<unsigned char>(*pos)) || *pos == '+')) ++pos;

        Number value{};
        const auto [next, error] = std::from_chars(pos, end, value);
        if (error != std::errc()) throw std::runtime_error("bad input: a number was expected");
        pos = next;
        return value;
    }

    // The first two numbers of the input
    struct Header
    {
        long long triangles_num;
        long long dimension;
    };

    inline Header read_header(const char*& pos, const char* end)
    {
        Header header;
        header.triangles_num = next_number<long long>(pos, end);
        header.dimension = next_number<long long>(pos, end);
        if (header.triangles_num < 0) throw std::runtime_error("bad input: negative number of triangles");
        return header;
    }
}

template <std::size_t N>
    requires(N == 2 || N == 3)
class Scene
{
public:
    explicit Scene(std::vector<Triangle<double, N>> triangles) : triangles_(std::move(triangles)) {}

    // Read the numbers of triangles_num triangles from [pos, end), the header is already read
    // Throws std::runtime_error if the input is broken
    static Scene parse(const char*& pos, const char* end, long long triangles_num)
    {
        // do not trust the number for the memory: a triangle takes at least 6 * N - 1 characters
        std::vector<Triangle<double, N>> triangles;
        triangles.reserve(std::min<std::size_t>(static_cast<std::size_t>(triangles_num),
                                                static_cast<std::size_t>(end - pos) / (6 * N - 1) + 1));

        for (long long i = 0; i < triangles_num; ++i)
        {
            // first we build every vertex (a vector) from its N numbers
            Vector<double, N> vertices[3];
            for (auto& vertex : vertices)
                for (std::size_t k = 0; k < N; ++k)
                {
                    vertex[k] = detailsScene::next_number<double>(pos, end);
                    if (!std::isfinite(vertex[k])) throw std::runtime_error("bad input: a coordinate is not finite");
                }

            // and then the triangle from the three vertices
            triangles.emplace_back(vertices[0], vertices[1], vertices[2]);
        }
        return Scene(std::move(triangles));
    }

    // Read a whole scene, the dimension in the input must be N
    static Scene read(std::istream& in)
    {
        const std::string text = detailsScene::read_all(in);
        const char* pos = text.data();
        const char* end = pos + text.size();

        const detailsScene::Header header = detailsScene::read_header(pos, end);
        if (header.dimension != static_cast<long long>(N))
            throw std::runtime_error("bad input: the dimension of the scene is not the expected one");
        // create triangles vector
        return parse(pos, end, header.triangles_num);
    }

    // get vector of input triangles
    const std::vector<Triangle<double, N>>& triangles() const { return triangles_; }
    // get triangles number
    std::size_t size() const { return triangles_.size(); }
    // check is a triangles vector empty
    bool empty() const { return triangles_.empty(); }

    // Get box around all triangles, it is needed for the camera and for the cube
    // The scene must not be empty
    AABB<double, N> bounds() const
    {
        assert(!triangles_.empty());
        return AABB<double, N>::of(triangles_.begin(), triangles_.end());
    }

    // intersecting[i] is true if the triangle i intersects at least one other triangle
    std::vector<bool> find_intersecting() const
    {
        std::vector<bool> intersecting(triangles_.size(), false);
        if (triangles_.size() < 2) return intersecting;

        // boxes of all triangles: the tree is built from them
        std::vector<AABB<double, N>> boxes;
        boxes.reserve(triangles_.size());
        for (const auto& triangle : triangles_) boxes.emplace_back(triangle);

        // degeneracy is checked once for every triangle, not for every pair
        std::vector<char> degenerate(triangles_.size());
        for (std::size_t i = 0; i < triangles_.size(); ++i) degenerate[i] = triangles_[i].is_degenerate();

        const BVH<double, N> tree(std::move(boxes));

        // the tree finds the pairs of triangles whose boxes overlap,
        // and for each of them we run the exact test
        tree.for_each_pair([&](std::size_t i, std::size_t j)
        {
            // both are already marked: nothing new to learn
            if (intersecting[i] && intersecting[j]) return;

            // degenerate triangles are not considered (intersect would return false for them too),
            // here we skip them without the call
            if (degenerate[i] || degenerate[j]) return;

            if (intersect(triangles_[i], triangles_[j])) intersecting[i] = intersecting[j] = true;
        });
        return intersecting;
    }

private:
    std::vector<Triangle<double, N>> triangles_;
};

// Read a scene when the dimension is known only from the input: it is 2D or 3D
// Use std::visit to work with the result:
//     std::visit([](const auto& scene) { auto flags = scene.find_intersecting(); ... }, read_scene(std::cin));
inline std::variant<Scene<2>, Scene<3>> read_scene(std::istream& in)
{
    const std::string text = detailsScene::read_all(in);
    const char* pos = text.data();
    const char* end = pos + text.size();

    const detailsScene::Header header = detailsScene::read_header(pos, end);
    if (header.dimension == 2) return Scene<2>::parse(pos, end, header.triangles_num);
    if (header.dimension == 3) return Scene<3>::parse(pos, end, header.triangles_num);
    throw std::runtime_error("bad input: the dimension must be 2 or 3");
}
