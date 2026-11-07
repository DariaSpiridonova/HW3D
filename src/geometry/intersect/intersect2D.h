#pragma once
 
#include <algorithm>
#include <concepts>
#include <cstddef>
 
#include "primitives/eps.h"
#include "primitives/triangle.h"
#include "primitives/vec.h"
 
/*
    Intersection of two triangles on a plane
    Touching is considered as intersection
*/
 
namespace details2D {
 
    // Position of the point p relative to the line ab: -1 / 0 / +1
    // +1 - on the left (counterclockwise), -1 - on the right, 0 - on the line
    template <std::floating_point T>
    int side(const Vector<T, 2>& a, const Vector<T, 2>& b, const Vector<T, 2>& p) 
    {
        const Vector<T, 2> ab = b - a;
        const T par_area = ab.det(p - a);
        const T tol = kEps<T> * ab.length();
        return (par_area > tol) - (par_area < -tol);
    }
 
    // The point p lies on the line ab, does it lie on the segment itself (by coordinates)?
    template <std::floating_point T>
    bool on_segment(const Vector<T, 2>& a, const Vector<T, 2>& b, const Vector<T, 2>& p) 
    {
        // Look at the projections on the coordinate axes
        for (std::size_t k = 0; k < 2; ++k)
            if (p[k] < std::min(a[k], b[k]) - kEps<T> || p[k] > std::max(a[k], b[k]) + kEps<T>)
                return false;
        return true;
    }
 
    // Do the segments ab and cd intersect (touching by an end is intersection too)?
    template <std::floating_point T>
    bool segments_intersect(const Vector<T, 2>& a, const Vector<T, 2>& b,
                            const Vector<T, 2>& c, const Vector<T, 2>& d) 
    {
        const int s1 = side(a, b, c);
        const int s2 = side(a, b, d);
        const int s3 = side(c, d, a);
        const int s4 = side(c, d, b);
    
        // Ordinary intersection: the ends of each segment are on different sides of the other one
        if (s1 * s2 < 0 && s3 * s4 < 0) return true;
    
        // An end lies on the line of the other segment, check if it lies on the segment itself
        if (s1 == 0 && on_segment(a, b, c)) return true;
        if (s2 == 0 && on_segment(a, b, d)) return true;
        if (s3 == 0 && on_segment(c, d, a)) return true;
        if (s4 == 0 && on_segment(c, d, b)) return true;
        return false;
    }
 
    // Is the point inside the triangle (the border is included)?
    template <std::floating_point T>
    bool point_in_triangle(const Vector<T, 2>& p, const Triangle<T, 2>& t)
    {
        const int s0 = side(t[0], t[1], p);
        const int s1 = side(t[1], t[2], p);
        const int s2 = side(t[2], t[0], p);
        const bool has_neg = s0 < 0 || s1 < 0 || s2 < 0;
        const bool has_pos = s0 > 0 || s1 > 0 || s2 > 0;
        return !(has_neg && has_pos);
    }
 
}

// Do two triangles on a plane intersect (touching is intersection too)?
template <std::floating_point T>
bool intersect(const Triangle<T, 2>& a, const Triangle<T, 2>& b) 
{
    // Degenerate triangles (a segment or a point) are not considered: such a pair does not intersect
    if (a.is_degenerate() || b.is_degenerate()) return false;

    // Check all nine pairs of edges
    for (std::size_t k = 0; k < 3; ++k)
        for (std::size_t m = 0; m < 3; ++m)
            if (details2D::segments_intersect(a[k], a[(k + 1) % 3], b[m], b[(m + 1) % 3]))
                return true;

    // The edges do not intersect, but one triangle can lie completely inside the other
    return details2D::point_in_triangle(a[0], b) || details2D::point_in_triangle(b[0], a);
}

// Сonvert triangles with integer coordinates to doubles 
template <std::integral T>
bool intersect(const Triangle<T, 2>& a, const Triangle<T, 2>& b) 
{
    return intersect(Triangle<double, 2>(a), Triangle<double, 2>(b));
}
