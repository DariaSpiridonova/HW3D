#pragma once

#include <cassert>
#include <concepts>
#include "triangle.h"

// Ax + By + Cz + D = 0

template <std::floating_point T = double>
class Plane 
{
public:
    explicit Plane(const Triangle<T>& t) : norm_(t.normal()), d_(0) {
        T len = norm_.length();
        assert(len > T(0));
        norm_ *= T(1) / len;
        d_ = -norm_.dot(t[0]);
    }

    // get the distance to the end of the vector from the plane
        // > 0: the point is on the side where the normal points, < 0: on the other side,
    // 0: the point lies on the plane
    T signed_distance(const Vector<T, 3>& p) const 
    { 
        return norm_.dot(p) + d_; 
    }

    // Get plane normal
    const Vector<T, 3>& normal() const { return norm_; }
    // Get distance to the origin
    T offset() const { return d_; }

private:
    Vector<T, 3> norm_; // plane normal
    T d_;               // distance to the origin
};
