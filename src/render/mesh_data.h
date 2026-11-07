#pragma once

#include <cassert>
#include <cstddef>
#include <vector>

#include "scene/scene.h"
#include "view_data.h"

/*
    Turns a scene and the flags "intersecting" into the vertices for the video card
    It knows nothing about OpenGL, so it is checked by ordinary tests
*/

namespace detailsMesh
{
    // A point of a triangle as 3 floats, a 2D point is placed to the plane z = 0
    template <std::size_t N>
    std::array<float, 3> position_3d(const Vector<double, N>& point)
    {
        if constexpr (N == 3) return {static_cast<float>(point[0]), static_cast<float>(point[1]), static_cast<float>(point[2])};
        else return {static_cast<float>(point[0]), static_cast<float>(point[1]), 0.0f};
    }

    // Normal of the triangle with length 1, for a 2D triangle it looks along z
    // The triangle must not be degenerate
    template <std::size_t N>
    std::array<float, 3> unit_normal(const Triangle<double, N>& triangle)
    {
        if constexpr (N == 3)
        {
            Vector<double, 3> normal = triangle.normal();
            normal *= 1.0 / normal.length();
            return {static_cast<float>(normal[0]), static_cast<float>(normal[1]), static_cast<float>(normal[2])};
        }
        else return {0.0f, 0.0f, 1.0f};
    }
}

// Fills the vertices of all triangles: 3 vertices for every triangle
// Degenerate triangles are not drawn: they have no area (and no normal)
template <std::size_t N>
std::vector<Vertex> make_vertices(const Scene<N>& scene, const std::vector<bool>& intersecting)
{
    assert(intersecting.size() == scene.size());

    std::vector<Vertex> vertices;
    vertices.reserve(scene.size() * 3);

    for (std::size_t i = 0; i < scene.size(); ++i)
    {
        const Triangle<double, N>& triangle = scene.triangles()[i];
        if (triangle.is_degenerate()) continue;

        const std::array<float, 3> normal = detailsMesh::unit_normal(triangle);
        const float isRed = intersecting[i] ? 1.0f : 0.0f;

        // all three vertices of the triangle have the same normal and the same flag
        for (std::size_t v = 0; v < 3; ++v)
        {
            // get 3D coordinates of the vertex, for a 2D triangle z = 0
            const std::array<float, 3> position = detailsMesh::position_3d(triangle[v]);
            vertices.push_back(Vertex{position[0], position[1], position[2], normal[0], normal[1], normal[2], isRed});
        }
    }
    return vertices;
}

// The same, but with everything the viewer needs
template <std::size_t N>
ViewData make_view_data(const Scene<N>& scene, const std::vector<bool>& intersecting)
{
    return make_view_data(make_vertices(scene, intersecting));
}
