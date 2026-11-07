#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

#include "aabb.h"
#include "primitives/vec.h"

/*
    Bounding volume hierarchy (BVH): a binary tree of boxes
    Each node has a box around a group of boxes, a leaf holds a few boxes themselves
    The tree is built from boxes only and knows nothing about triangles,
    the triangle number is the index of its box
*/

template <std::floating_point T = double, std::size_t N = 3>
    requires(N == 2 || N == 3)
class BVH 
{
public:
    // Input:
    // 1) a vector of boxes of individual triangles 
    // 2) leaf_size - the max number of boxes of individual triangles in a leaf
    // Output: 
    // a bvh tree  
    // The boxes are moved inside; leaf_size is the max number of boxes in a leaf
    explicit BVH(std::vector<AABB<T, N>> boxes, std::size_t leaf_size = 4)
        : boxes_(std::move(boxes)), leaf_size_(std::max<std::size_t>(leaf_size, 1))
    {
        if (boxes_.empty()) return;

        // Centers of the boxes: we split the groups by them
        std::vector<Vector<T, N>> centers;
        centers.reserve(boxes_.size());
        for (const auto& b : boxes_) centers.push_back(b.center());

        // Order of the boxes: only this list of numbers is rearranged, not the boxes themselves
        order_.resize(boxes_.size());
        std::iota(order_.begin(), order_.end(), std::size_t{0});

        nodes_.reserve(2 * boxes_.size() / leaf_size_ + 2);
        build(0, order_.size(), centers);
    }

    // Calls visit(j) for every box j that overlaps the given box
    // visit returns true to stop the search, query returns true if it was stopped
    //   all pairs:           visit always returns false
    //   one partner enough:  visit returns true when it has found it
    template <typename Visitor>
    bool query(const AABB<T, N>& box, Visitor&& visit) const
    {
        if (nodes_.empty()) return false;
        return query_node(0, box, visit);
    }

    // Calls visit(i, j) for every pair of overlapping boxes, each pair exactly once (i != j)
    // Instead of one descent from the root for every box, we walk down with pairs of nodes
    // visit returns nothing; it can skip the expensive work itself (for example,
    // if both triangles are already marked)
    template <typename Visitor>
    void for_each_pair(Visitor&& visit) const
    {
        if (nodes_.empty()) return;
        pairs_inside(0, visit);
    }

    std::size_t size() const { return boxes_.size(); }
    std::size_t node_count() const { return nodes_.size(); }

private:
    static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

    struct Node
    {
        AABB<T, N> box;
        std::size_t begin, end;    // the range in order_ (for a leaf: the boxes themselves)
        std::size_t left, right;   // children, npos for a leaf
        bool is_leaf() const { return left == npos; }
    };

    std::vector<AABB<T, N>> boxes_;  // a vector of boxes of individual triangles
    std::size_t leaf_size_;          // the max number of boxes of individual triangles in a leaf
    std::vector<std::size_t> order_; // 
    std::vector<Node> nodes_;        // the plane bvh tree itself


    // Builds the node for the range [begin, end) of order_, returns its index
    std::size_t build(std::size_t begin, std::size_t end, const std::vector<Vector<T, N>>& centers)
    {
        // Box of the node: union of the boxes of the group
        AABB<T, N> box = boxes_[order_[begin]];
        for (std::size_t i = begin + 1; i < end; ++i) box.merge(boxes_[order_[i]]);

        const std::size_t index = nodes_.size();
        nodes_.push_back(Node{box, begin, end, npos, npos});

        // A small group is a leaf
        if (end - begin <= leaf_size_) return index;

        // Split the group in half by the number along the longest axis of the box
        // nth_element puts the left half to the left of mid, no full sort is needed
        const std::size_t axis = box.longest_axis();
        const std::size_t mid = begin + (end - begin) / 2;
        std::nth_element(order_.begin() + begin, order_.begin() + mid, order_.begin() + end,
                         [&](std::size_t a, std::size_t b) { return centers[a][axis] < centers[b][axis]; });

        const std::size_t l = build(begin, mid, centers);
        const std::size_t r = build(mid, end, centers);
        nodes_[index].left = l;   // do not take a reference before: nodes_ may be reallocated
        nodes_[index].right = r;
        return index;
    }

    template <typename Visitor>
    bool query_node(std::size_t index, const AABB<T, N>& box, Visitor& visit) const
    {
        const Node& node = nodes_[index];

        // The boxes do not overlap: skip the whole subtree
        if (!node.box.overlaps(box)) return false;

        if (node.is_leaf())
        {
            for (std::size_t i = node.begin; i < node.end; ++i)
            {
                const std::size_t j = order_[i];
                if (boxes_[j].overlaps(box) && visit(j)) return true;
            }
            return false;
        }
        return query_node(node.left, box, visit) || query_node(node.right, box, visit);
    }

    // Go through all pairs: one box from the first range of order_, the other one from the second range
    // start_order_pos and end_order_pos are positions in order_, not triangle numbers
    // Only the boxes are checked here, if they overlap we tell visit about the pair (it does the exact test)
    template <typename Visitor>
    void check_intersects_for_tr_boxes_range(std::size_t start_order_pos1, std::size_t end_order_pos1,
                                             std::size_t start_order_pos2, std::size_t end_order_pos2, Visitor& visit) const
    {
        for (std::size_t order_pos1 = start_order_pos1; order_pos1 < end_order_pos1; ++order_pos1)
            for (std::size_t order_pos2 = start_order_pos2; order_pos2 < end_order_pos2; ++order_pos2)
            {
                // get triangle numbers by their positions in order_
                const std::size_t triagb_i1 = order_[order_pos1], triagb_i2 = order_[order_pos2];
                if (boxes_[triagb_i1].overlaps(boxes_[triagb_i2])) visit(triagb_i1, triagb_i2);
            }
    }

    // All pairs inside one node
    template <typename Visitor>
    void pairs_inside(std::size_t index, Visitor& visit) const
    {
        const Node& node = nodes_[index];

        if (node.is_leaf())
        {
            // pairs inside one leaf: every position with all the positions after it
            // we can not take the whole rectangle [begin, end) x [begin + 1, end),
            // there would be pairs of a box with itself and the same pair twice
            for (std::size_t order_pos = node.begin; order_pos < node.end; ++order_pos)
                check_intersects_for_tr_boxes_range(order_pos, order_pos + 1, order_pos + 1, node.end, visit);
            return;
        }

        // Check for intersections of pairs inside the left child, inside the right child,
        // and the pairs where one box is from the left and the other from the right
        pairs_inside(node.left, visit);
        pairs_inside(node.right, visit);
        pairs_between(node.left, node.right, visit);
    }

    // All pairs (x from the node node_i1, y from the node node_i2)
    template <typename Visitor>
    void pairs_between(std::size_t node_i1, std::size_t node_i2, Visitor& visit) const
    {
        const Node& node1 = nodes_[node_i1];
        const Node& node2 = nodes_[node_i2];

        // The boxes of the groups do not overlap: skip all pairs at once
        if (!node1.box.overlaps(node2.box)) return;

        // Two leaves: go through all pairs of their boxes
        if (node1.is_leaf() && node2.is_leaf())
        {
            check_intersects_for_tr_boxes_range(node1.begin, node1.end, node2.begin, node2.end, visit);
            return;
        }

        // Go down into the bigger group (a leaf can not be split)
        const bool split_node1 = !node1.is_leaf() && (node2.is_leaf() || node1.end - node1.begin >= node2.end - node2.begin);
        if (split_node1)
        {
            // node2 stays, node1 is replaced by its two children
            pairs_between(node1.left, node_i2, visit);
            pairs_between(node1.right, node_i2, visit);
        }
        else
        {
            // node1 stays, node2 is replaced by its two children
            pairs_between(node_i1, node2.left, visit);
            pairs_between(node_i1, node2.right, visit);
        }
    }
};
