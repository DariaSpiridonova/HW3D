#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <limits>
#include <utility>

#include "primitives/eps.h"
#include "intersect2D.h"
#include "primitives/plane.h"
#include "primitives/triangle.h"

/*
    Intersection of two triangles in space
    Touching is considered as intersection
    Based on «A Fast Triangle-Triangle Intersection Test» by Tomas Akenine-Möller
    The coplanar case is moved to 2D and solved by intersect from intersect2d.hpp
*/
 
/*
    d - distance
    t - triangle
    p - plane
*/
 
namespace details3D 
{
    // Distances of three vertices t to the plane pl
    // values ​​less than eps in absolute value are considered zero (the vertex lies on the plane)
    template <std::floating_point T>
    std::array<T, 3> distances(const Plane<T>& pl, const Triangle<T>& t) 
    {
        std::array<T, 3> d;
        for (std::size_t i = 0; i < 3; ++i) 
        {
            d[i] = pl.signed_distance(t[i]);
            if (std::abs(d[i]) < kEps<T>) d[i] = T(0);
        }
        return d;
    }
 
    // Are all three vertices strictly on one side of the plane? (zeros don`t count)
    template <std::floating_point T>
    bool all_strictly_one_side(const std::array<T, 3>& d) 
    {
        return (d[0] > 0 && d[1] > 0 && d[2] > 0) ||
            (d[0] < 0 && d[1] < 0 && d[2] < 0);
    }


    // Are all three vertices within eps of the plane (after zeroing in distances)?
    template <std::floating_point T>
    bool all_zero(const std::array<T, 3>& d) 
    {
        return d[0] == 0 && d[1] == 0 && d[2] == 0;
    }

    template <std::floating_point T>
    std::pair<T, T> interval_on_line(const Triangle<T>& t, const std::array<T, 3>& d,
                                    const Vector<T, 3>& dir) 
    {
        // Calculate projections of vertices onto dir
        std::array<T, 3> p;  
        for (std::size_t i = 0; i < 3; ++i) p[i] = dir.dot(t[i]);

        // Min and max coordinates on L
        T lo = std::numeric_limits<T>::max();
        T hi = std::numeric_limits<T>::lowest();

        // For checking the lower and upper boundaries
        auto check_lim = [&](T x) 
        {
            lo = std::min(lo, x);
            hi = std::max(hi, x);
        };

        // Looking for vertices lying on the plane
        for (std::size_t i = 0; i < 3; ++i)
            if (d[i] == 0) check_lim(p[i]);

        // Looking for vertices lying on different sides of the plane and look at the intersection point 
        for (std::size_t i = 0; i < 3; ++i) 
        {
            const std::size_t j = (i + 1) % 3;
            const bool opposite = (d[i] < 0 && d[j] > 0) || (d[i] > 0 && d[j] < 0);
            if (opposite)
            {
                const T s = d[i] / (d[i] - d[j]); // the fraction of the path at which the distance = 0
                check_lim(p[i] + (p[j] - p[i]) * s);
            }
        }

        assert(lo <= hi);
        return {lo, hi};
    }

    template <std::floating_point T>
    std::pair<Triangle<T, 2>, Triangle<T, 2>> project_to_2d(const Triangle<T, 3>& t1,
                                                          const Triangle<T, 3>& t2) 
    {
        const Vector<T, 3> n1 = t1.normal();
        const Vector<T, 3> n2 = t2.normal();
        // Get the longest normal for the greatest projection accuracy
        const Vector<T, 3>& n = (n1.dot(n1) >= n2.dot(n2)) ? n1 : n2;
    
        // Looking for the axis along which the projection of the normal is maximum and drop it
        std::size_t drop = 0;
        if (std::abs(n[1]) > std::abs(n[drop])) drop = 1;
        if (std::abs(n[2]) > std::abs(n[drop])) drop = 2;
        const std::size_t x_ind = (drop + 1) % 3;  // it will be the first  point coordinate in 2D
        const std::size_t y_ind = (drop + 2) % 3;  // it will be the second point coordinate in 2D
    
        // Lambda for obtaining the coordinates of the triangle vertices in 2D
        auto project = [&](const Triangle<T, 3>& t) {
            return Triangle<T, 2>(Vector<T, 2>(t[0][x_ind], t[0][y_ind]),
                                Vector<T, 2>(t[1][x_ind], t[1][y_ind]),
                                Vector<T, 2>(t[2][x_ind], t[2][y_ind]));
        };
        return {project(t1), project(t2)};
    }

}

// Do two triangles in space intersect (touching is intersection too)?
template <std::floating_point T>
bool intersect(const Triangle<T, 3>& t1, const Triangle<T, 3>& t2) 
{
    // Degenerate triangles (a segment or a point) are not considered: such a pair does not intersect
    if (t1.is_degenerate() || t2.is_degenerate()) return false;
 
    // Get triangles planes
    const Plane<T> p1(t1);
    const Plane<T> p2(t2);
 
    // Check the location of the vertices of
    // 1. t1 relative to the plane of t2
    const auto d1 = details3D::distances(p2, t1);
    if (details3D::all_strictly_one_side(d1)) return false;
 
    // 2. t2 relative to the plane of t1
    const auto d2 = details3D::distances(p1, t2);
    if (details3D::all_strictly_one_side(d2)) return false;
 
    // Direction of the line L of the planes intersection: dir = n1 x n2
    Vector<T, 3> dir = p1.normal().cross(p2.normal());
    const T len = dir.length();
 
    // The triangles are coplanar if
    //      a) all the vertices of one triangle lie within eps of the plane of the other
    //      b) the planes are almost parallel (len ~ 0): there is no line L,
    if (details3D::all_zero(d1) || details3D::all_zero(d2) || len < kEps<T>) 
    {
        const auto [a, b] = details3D::project_to_2d(t1, t2); // move to 2D
        return intersect(a, b);  // overload for Triangle<T, 2>
    }
    dir *= T(1) / len;  // normalize, so that eps below means a real length
 
    // Get segments on L cut out by triangles
    const auto [min1, max1] = details3D::interval_on_line(t1, d1, dir);
    const auto [min2, max2] = details3D::interval_on_line(t2, d2, dir);
 
    // Check for overlapping or touching intervals
    return std::max(min1, min2) <= std::min(max1, max2) + kEps<T>;
}
 
// Сonvert triangles with integer coordinates to doubles 
template <std::integral T>
bool intersect(const Triangle<T, 3>& t1, const Triangle<T, 3>& t2) 
{
    return intersect(Triangle<double, 3>(t1), Triangle<double, 3>(t2));
}
