#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <concepts>

#include "vec.h"

// Triangle in N-dimensional space (N = 2 or 3)
template <Scalar T = double, std::size_t N = 3>
requires(N == 2 || N == 3)
class Triangle {
public:
    Triangle(const Vector<T, N>& v0, const Vector<T, N>& v1, const Vector<T, N>& v2)
        : vertices_{v0, v1, v2} {}

    // Converts a triangle with another coordinate type, for example Triangle<double, 3>(t_int)
    template <Scalar U>
    explicit Triangle(const Triangle<U, N>& tri)
        : vertices_{Vector<T, N>(tri[0]), Vector<T, N>(tri[1]), Vector<T, N>(tri[2])} {}
 
    const Vector<T, N>& operator[](std::size_t i) const 
    {
        assert(i < 3);  
        return vertices_[i];
    }

    // for (const auto& v : t) { ... }
    auto begin() const { return vertices_.begin(); }
    auto end() const { return vertices_.end(); }

    // Unnormalized normal, its length is twice the area of the triangle
    Vector<T, N> normal() const requires(N == 3)
    {
        return (vertices_[1] - vertices_[0]).cross(vertices_[2] - vertices_[0]);
    }

    // Double the area of ​​a parallelogram constructed from two triangle vectors
    T twice_area() const requires(N == 2 || N == 3)
    {
        const Vector<T, N> e1 = vertices_[1] - vertices_[0];
        const Vector<T, N> e2 = vertices_[2] - vertices_[0];
        if constexpr (N == 3) 
        {
            const Vector<T, N> n = e1.cross(e2);
            return n.dot(n);
        } 
        else 
        {
            const T d = e1.det(e2);
            return d * d;
        }
    }

    // A triangle is degenerate if three points lie on one line (including coinciding points)
    bool is_degenerate(T eps = T(1e-12)) const { return twice_area() <= eps; }

private:
    std::array<Vector<T, N>, 3> vertices_;
};