#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <utility>

#include "primitives/eps.h"
#include "primitives/triangle.h"
#include "primitives/vec.h"

/*
    Axis-aligned bounding box (AABB): a box with sides parallel to the coordinate axes
    It is set by two points: the minimum and the maximum coordinates
    Used for a fast rejection: if the boxes do not intersect, the triangles inside do not either
*/

template <std::floating_point T = double, std::size_t N = 3>
    requires(N == 2 || N == 3)
class AABB 
{
public:
    // Box of one triangle
    explicit AABB(const Triangle<T, N>& t) : min_(t[0]), max_(t[0]) {
        for (std::size_t v = 1; v < 3; ++v) {
            for (std::size_t k = 0; k < N; ++k) {
                min_[k] = std::min(min_[k], t[v][k]);
                max_[k] = std::max(max_[k], t[v][k]);
            }
        }
    }

    // Box of a set of triangles: the range [first, last) must not be empty
    // AABB<double, 3>::of(v.begin(), v.end())
    template <std::forward_iterator It>
        requires std::same_as<std::iter_value_t<It>, Triangle<T, N>>
    static AABB of(It first, It last) 
    {
        assert(first != last);
        AABB box(*first);
        for (++first; first != last; ++first) box.merge(AABB(*first));
        return box;
    }

    // Expands this box so that it also contains the box 
    AABB& merge(const AABB& box) 
    {
        for (std::size_t k = 0; k < N; ++k) 
        {
            min_[k] = std::min(min_[k], box.min_[k]);
            max_[k] = std::max(max_[k], box.max_[k]);
        }
        return *this;
    }

    // Do two boxes intersect? Touching is intersection too
    // eps is added, as in the exact test, otherwise a pair touching within eps
    // would be rejected by the boxes, but accepted by intersect
    bool overlaps(const AABB& box) const 
    {
        for (std::size_t k = 0; k < N; ++k)
            if (min_[k] > box.max_[k] + kEps<T> || box.min_[k] > max_[k] + kEps<T>)
                return false;
        return true;
    }

    Vector<T, N> center() const { return (min_ + max_) * T(0.5); }

    // Index of the axis along which the box is the longest (to split a group of triangles)
    std::size_t longest_axis() const 
    {
        std::size_t axis = 0;
        for (std::size_t k = 1; k < N; ++k)
            if (max_[k] - min_[k] > max_[axis] - min_[axis]) axis = k;
        return axis;
    }

    // get bounds of the box
    const Vector<T, N>& min() const { return min_; }
    const Vector<T, N>& max() const { return max_; }

private:
    Vector<T, N> min_; // min box coordinate 
    Vector<T, N> max_; // max box coordinate
};
