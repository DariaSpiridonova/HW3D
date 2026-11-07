#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <vector>

/*
    Plain data for the viewer: no OpenGL here and no geometry classes
    The viewer takes only this, so it does not depend on the scene
*/

// One vertex for the video card: 7 floats in a row 
struct Vertex
{
    float x, y, z;      // position
    float nx, ny, nz;   // normal (for the light)
    float isRed;        // 1.0f if the triangle intersects another one, 0.0f if not
};

// The vertices go to the file and to the video card as raw bytes, so there must be no gaps inside
static_assert(sizeof(Vertex) == 7 * sizeof(float), "Vertex must not have padding");
static_assert(std::is_standard_layout_v<Vertex> && std::is_trivially_copyable_v<Vertex>);

// Box around the scene as two corners in 3D (a 2D scene has z = 0)
struct Bounds3
{
    std::array<float, 3> min{};
    std::array<float, 3> max{};
};

struct ViewData
{
    std::vector<Vertex> triangle_vertices;   // 3 vertices per triangle
    std::vector<float> box_lines;            // 12 edges of the box: 24 points, 3 floats each
    Bounds3 bounds;
    std::size_t triangles_num = 0;           // how many triangles are drawn (degenerate ones are not)
    std::size_t red_num = 0;                 // how many of them are intersecting
};

// 12 edges of the box: two corners differ in exactly one coordinate
inline std::vector<float> make_box_lines(const Bounds3& box)
{
    std::vector<float> lines;
    lines.reserve(24 * 3);
    auto corner = [&](int i) {
        return std::array<float, 3>{(i & 1) ? box.max[0] : box.min[0], (i & 2) ? box.max[1] : box.min[1], (i & 4) ? box.max[2] : box.min[2]};
    };
    for (int i = 0; i < 8; ++i)
        for (int bit : {1, 2, 4})
            if (!(i & bit))
                for (int c : {i, i | bit})
                {
                    const auto point = corner(c);
                    lines.insert(lines.end(), point.begin(), point.end());
                }
    return lines;
}

// Everything the viewer needs, made from the vertices only
// So the data are the same if the vertices come from the scene or from a file
inline ViewData make_view_data(std::vector<Vertex> vertices)
{
    ViewData data;
    data.triangles_num = vertices.size() / 3;

    if (!vertices.empty())
    {
        constexpr float inf = std::numeric_limits<float>::max();
        data.bounds.min = {inf, inf, inf};
        data.bounds.max = {-inf, -inf, -inf};
        for (const Vertex& v : vertices)
        {
            const float position[3] = {v.x, v.y, v.z};
            for (int k = 0; k < 3; ++k)
            {
                data.bounds.min[k] = std::min(data.bounds.min[k], position[k]);
                data.bounds.max[k] = std::max(data.bounds.max[k], position[k]);
            }
        }
        // the flag is the same for the three vertices of a triangle: count the first one
        for (std::size_t t = 0; t < data.triangles_num; ++t)
            if (vertices[t * 3].isRed > 0.5f) ++data.red_num;
    }

    data.box_lines = make_box_lines(data.bounds);
    data.triangle_vertices = std::move(vertices);
    return data;
}
