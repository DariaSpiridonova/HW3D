#include <array>
#include <cstdio>
#include <random>
#include <set>
#include <sstream>
#include <vector>

#include "test_common.h"

/*
    Checks of the whole geometry part
    Build:  g++ -std=c++20 -O2 -Wall -Wextra -I<folder with the headers> check_project.cpp -o check_project
    The exact tests use integer coordinates and an independent oracle (separating axes), without any eps
*/

const size_t TRIANGLES_NUM = 2500;

// ---------- the oracle: exact test of two triangles by the separating axis theorem ----------
using P3 = std::array<long long, 3>;
using T3 = std::array<P3, 3>;
static P3 sub(const P3& a, const P3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
static P3 cross(const P3& a, const P3& b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
static long long dot(const P3& a, const P3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
static bool is_zero(const P3& a) { return a[0] == 0 && a[1] == 0 && a[2] == 0; }

// true if the closed triangles A and B have a common point (touching counts)
// checking intersections based on the Separating Axis Theorem
static bool oracle(const T3& A, const T3& B)
{
    const P3 nA = cross(sub(A[1], A[0]), sub(A[2], A[0]));
    const P3 nB = cross(sub(B[1], B[0]), sub(B[2], B[0]));
    std::vector<P3> axes = {nA, nB};

    for (int i = 0; i < 3; ++i)
    {
        // calculate the guide vectors of the edges 
        const P3 eA = sub(A[(i + 1) % 3], A[i]), eB = sub(B[(i + 1) % 3], B[i]);
        
        axes.push_back(cross(nA, eA));   // in-plane normals of the edges: they decide the coplanar case
        axes.push_back(cross(nB, eB));
        
        for (int j = 0; j < 3; ++j) axes.push_back(cross(eA, sub(B[(j + 1) % 3], B[j])));
    }
    // Iterate through all the assembled axes from the axes vector for the final check of the projections
    for (const P3& axis : axes)
    {
        if (is_zero(axis)) continue;
        // initialize variables to store projection boundaries
        long long minA = 1LL << 60, maxA = -(1LL << 60), minB = 1LL << 60, maxB = -(1LL << 60);
        
        for (const P3& p : A) { const long long v = dot(axis, p); minA = std::min(minA, v); maxA = std::max(maxA, v); }
        for (const P3& p : B) { const long long v = dot(axis, p); minB = std::min(minB, v); maxB = std::max(maxB, v); }
        
        if (maxA < minB || maxB < minA) return false;   // a separating axis exists
    }

    return true;
}

// Checking for degeneracy 
static bool flat(const T3& t) { return is_zero(cross(sub(t[1], t[0]), sub(t[2], t[0]))); }

// Translates the triangle from the oracle data format to the project structure
static Triangle<int, 3> to_tri(const T3& t)
{
    auto v = [](const P3& p) { return Vector<int, 3>(static_cast<int>(p[0]), static_cast<int>(p[1]), static_cast<int>(p[2])); };
    return Triangle<int, 3>(v(t[0]), v(t[1]), v(t[2]));
}
// Converts an integer triangle of the project to a real one
static Tri3 to_real(const T3& t) { return Tri3(Triangle<int, 3>(to_tri(t))); }

// Generates a random triangle with integer coordinates in the range from 0 to range
static T3 random_int_triangle(std::mt19937& rng, int range)
{
    std::uniform_int_distribution<int> c(0, range);
    T3 t;
    for (auto& p : t) p = {c(rng), c(rng), c(rng)};
    return t;
}

// Generate n triangles in 3D: side - scene size, size - the maximum size of the triangle
static std::vector<Tri3> random_triangles(int n, double side, double size, unsigned seed)
{
    std::mt19937 rng(seed);
    // position - the center's coordinates, offset - the displacement of vertices relative to the center
    std::uniform_real_distribution<double> position(0, side), offset(-size, size);
    std::vector<Tri3> triangles;
    // fill the triangles vector with n values
    while (static_cast<int>(triangles.size()) < n)
    {
        // generate the center
        const V3 c(position(rng), position(rng), position(rng));
        // generate the triangle
        Tri3 t(c + V3(offset(rng), offset(rng), offset(rng)), c + V3(offset(rng), offset(rng), offset(rng)), c + V3(offset(rng), offset(rng), offset(rng)));
        if (!t.is_degenerate()) triangles.push_back(t);
    }

    return triangles;
}

// Check the parser to detect an invalid argument error
template <typename F>
static bool throws(const char* text, F read)
{
    std::istringstream in(text);
    try { read(in); } catch (const std::runtime_error&) { return true; }
    return false;
}

int main()
{
    int before = 0;

    before = fails;   // ---------------------------------------------------------------- Vector, Triangle, Plane
    {
        // check operations with vectors
        V3 a(1, 2, 3), b(4, 5, 6);
        CHECK((a + b)[0] == 5 && (b - a)[2] == 3 && (a * 2)[1] == 4 && (2 * a)[1] == 4);
        CHECK(a.dot(b) == 32 && a.cross(b)[0] == -3 && a.cross(b)[1] == 6 && a.cross(b)[2] == -3);
        CHECK(std::abs(V3(3, 4, 0).length() - 5) < 1e-12);
        CHECK(V2(1, 0).det(V2(0, 1)) == 1 && V2(0, 1).det(V2(1, 0)) == -1);
        
        // check operations with triangles and degenerate triangles
        Tri3 t(V3(0, 0, 0), V3(1, 0, 0), V3(0, 1, 0)), line(V3(0, 0, 0), V3(1, 1, 1), V3(2, 2, 2));
        CHECK(!t.is_degenerate() && line.is_degenerate() && t.normal()[2] == 1);
        
        // check operations with plane
        Plane<> plane(t);
        CHECK(plane.signed_distance(V3(5, 5, 2)) == 2 && plane.signed_distance(V3(5, 5, -3)) == -3 && plane.signed_distance(V3(7, -4, 0)) == 0);
    }
    report("Vector, Triangle, Plane", before);

    before = fails;   // ---------------------------------------------------------------- AABB
    {
        Tri3 a(V3(1, 5, -2), V3(4, 2, 0), V3(3, 3, 7));
        AABB<double, 3> box(a);
        CHECK(box.min()[0] == 1 && box.min()[2] == -2 && box.max()[1] == 5 && box.max()[2] == 7 && box.longest_axis() == 2);
        CHECK(!box.overlaps(AABB<double, 3>(Tri3(V3(10, 0, 0), V3(12, 1, 0), V3(10, 3, 1)))));
        CHECK(box.overlaps(AABB<double, 3>(Tri3(V3(4, 0, 0), V3(6, 1, 0), V3(5, 3, 1)))));              // touching
        CHECK(box.overlaps(AABB<double, 3>(Tri3(V3(4 + 5e-10, 0, 0), V3(6, 1, 0), V3(5, 3, 1)))));     // gap less than eps
        CHECK(!box.overlaps(AABB<double, 3>(Tri3(V3(4.001, 0, 0), V3(6, 1, 0), V3(5, 3, 1)))));
        
        std::vector<Tri3> v{a, Tri3(V3(10, 0, 0), V3(12, 1, 0), V3(10, 3, 1))};
        auto all = AABB<double, 3>::of(v.begin(), v.end());
        CHECK(all.min()[0] == 1 && all.max()[0] == 12);
    }
    report("AABB", before);

    before = fails;   // ---------------------------------------------------------------- general 3D against the oracle
    for (int range : {4, 6, 10})
    {
        std::mt19937 rng(range);
        long long pairs = 0, yes = 0;
        for (int i = 0; i < 80000; ++i)
        {
            const T3 a = random_int_triangle(rng, range), b = random_int_triangle(rng, range);
            if (flat(a) || flat(b)) continue;
            ++pairs;
            const bool expect = oracle(a, b);
            yes += expect;
            CHECK(intersect(to_tri(a), to_tri(b)) == expect);
            if (fails > before + 5) break;
        }
        std::printf("    3D, coordinates 0..%d: %lld pairs, %lld intersect\n", range, pairs, yes);
    }
    report("3D (general case) against the oracle", before);

    before = fails;   // ---------------------------------------------------------------- coplanar pairs: three planes and 2D
    {
        std::mt19937 rng(21);
        std::uniform_int_distribution<int> c(0, 6);
        long long pairs = 0, yes = 0;
        for (int i = 0; i < 60000; ++i)
        {
            // an array for storing 2D coordinates for two triangles
            int xy[2][3][2];
            for (auto& t : xy) for (auto& p : t) { p[0] = c(rng); p[1] = c(rng); }
            for (int kind = 0; kind < 3; ++kind)     // z = 0;  x + y + z = 3;  x = 7
            {
                T3 t[2];
                for (int k = 0; k < 2; ++k)
                    for (int v = 0; v < 3; ++v)
                    {
                        const long long x = xy[k][v][0], y = xy[k][v][1];
                        t[k][v] = kind == 0 ? P3{x, y, 0} : kind == 1 ? P3{x, y, 3 - x - y} : P3{7, x, y};
                    }
                if (flat(t[0]) || flat(t[1])) continue;
                const bool expect = oracle(t[0], t[1]);
                ++pairs; yes += expect;
                CHECK(intersect(to_tri(t[0]), to_tri(t[1])) == expect);
                if (kind == 0)    // the same pairs as 2D triangles
                {
                    auto v2 = [&](int k) { return Triangle<int, 2>(Vector<int, 2>(xy[k][0][0], xy[k][0][1]), Vector<int, 2>(xy[k][1][0], xy[k][1][1]), Vector<int, 2>(xy[k][2][0], xy[k][2][1])); };
                    CHECK(intersect(v2(0), v2(1)) == expect);
                }
            }
            if (fails > before + 5) break;
        }
        std::printf("    coplanar: %lld pairs, %lld intersect\n", pairs, yes);
    }
    report("coplanar (3 planes) and 2D against the oracle", before);

    before = fails;   // ---------------------------------------------------------------- BVH: all pairs of boxes
    for (int leaf : {1, 3, 4, 16}) // set the maximum objects number inside the leaf
    {
        // generate triangles
        auto tri = random_triangles(1200, 25, 3, 40 + leaf);
        // create and fill in the vector of the boxes of this triangles
        std::vector<AABB<double, 3>> boxes;
        for (auto& t : tri) boxes.emplace_back(t);

        // create a tree
        BVH<double, 3> tree(boxes, leaf);
        std::set<std::pair<std::size_t, std::size_t>> got, want;
        std::size_t visits = 0, self = 0;
        tree.for_each_pair([&](std::size_t i, std::size_t j) { ++visits; self += i == j; got.insert({std::min(i, j), std::max(i, j)}); });
        for (std::size_t i = 0; i < boxes.size(); ++i)
            for (std::size_t j = i + 1; j < boxes.size(); ++j)
                if (boxes[i].overlaps(boxes[j])) want.insert({i, j});
        CHECK(got == want && visits == want.size() && self == 0);     // exactly these pairs, each once, no pair with itself
    }

    // edge cases
    {   
        BVH<double, 3> empty{std::vector<AABB<double, 3>>{}};
        int count = 0; empty.for_each_pair([&](std::size_t, std::size_t) { ++count; }); CHECK(count == 0);
        std::vector<AABB<double, 3>> same(50, AABB<double, 3>(Tri3(V3(0, 0, 0), V3(1, 0, 0), V3(0, 1, 0))));
        BVH<double, 3> tree(same); long long c2 = 0; tree.for_each_pair([&](std::size_t, std::size_t) { ++c2; }); CHECK(c2 == 50 * 49 / 2);
    }
    report("BVH: all pairs of overlapping boxes", before);

    before = fails;   // ---------------------------------------------------------------- Scene: reading
    {
        std::istringstream in("2 3\r\n0 0 0 1 0 0 0 1 0\r\n 1e0 +2.5 -3  4 5 6 7 8 9\n");
        Scene<3> s = Scene<3>::read(in);
        CHECK(s.size() == 2 && s.triangles()[1][0][1] == 2.5 && s.triangles()[1][0][2] == -3 && s.triangles()[1][2][2] == 9);
        
        std::istringstream in2("1 2\n0 0 1 0 0 1");
        CHECK(Scene<2>::read(in2).size() == 1);
        
        std::istringstream in3("1 3\n0 0 0 1 0 0 0 1 0"); auto v3 = read_scene(in3); CHECK(v3.index() == 1);
        std::istringstream in4("1 2\n0 0 1 0 0 1");       auto v2 = read_scene(in4); CHECK(v2.index() == 0);
        
        // lots of spaces between the numbers
        std::string gap(40, ' '), text = "3" + gap + "3\n\n\n"; int value = 1;
        for (int i = 0; i < 27; ++i) text += std::to_string(value++) + "\t" + gap + "\n\n";
        std::istringstream in5(text); Scene<3> s5 = Scene<3>::read(in5);
        CHECK(s5.size() == 3 && s5.triangles()[2][2][2] == 27);
        
        // validates incorrect input data
        auto read_any = [](std::istream& i) { read_scene(i); };
        auto read3 = [](std::istream& i) { Scene<3>::read(i); };
        CHECK(throws("1 4\n0 0 0 0 1 1 1 1 2 2 2 2", read_any));
        CHECK(throws("1 2\n0 0 1 0 0 1", read3));
        CHECK(throws("1 3\n0 0 0 1 0 0 0 1", read_any));
        CHECK(throws("2", read_any));
        CHECK(throws("abc 3", read_any));
        CHECK(throws("-1 3", read_any));
        CHECK(throws("1 3\n0 0 0 1 0 0 nan 1 0", read_any));
        CHECK(throws("", read_any));
        CHECK(throws("1000000000000 3\n0 0 0", read_any));
        CHECK(throws("\xEF\xBB\xBF" "1 3\n0 0 0 1 0 0 0 1 0", read_any));     // BOM: a clean error, not a crash
        
        std::istringstream in6("0 3"); Scene<3> empty = Scene<3>::read(in6); CHECK(empty.empty() && empty.find_intersecting().empty());
        
        Scene<3> b({Tri3(V3(1, 5, -2), V3(4, 2, 0), V3(3, 3, 7)), Tri3(V3(-1, 0, 0), V3(2, 9, 1), V3(0, 0, 3))});
        CHECK(b.bounds().min()[0] == -1 && b.bounds().min()[2] == -2 && b.bounds().max()[1] == 9 && b.bounds().max()[2] == 7);
    }
    report("Scene: reading, errors, bounds", before);

    before = fails;   // ---------------------------------------------------------------- Scene: flags
    {
        // 3 different scene dimensions
        for (double side : {15.0, 40.0, 80.0})     // real coordinates: against brute force with own intersect
        {
            auto tri = random_triangles(TRIANGLES_NUM, side, 3, 17); // get triangles vector
            Scene<3> s(tri); // create a scene
            std::vector<bool> want(tri.size(), false);

            // slow circle to find intersects
            for (std::size_t i = 0; i < tri.size(); ++i)
                for (std::size_t j = i + 1; j < tri.size(); ++j)
                    if (intersect(tri[i], tri[j])) want[i] = want[j] = true;
            // compare with the results of the quick algorithm
            CHECK(s.find_intersecting() == want);
        }

        for (int range : {12, 20, 40})             // integer coordinates: against the independent oracle
        {
            std::mt19937 rng(range);
            std::uniform_int_distribution<int> base(0, range), d(0, 3);
            std::vector<T3> t;
            while (t.size() < TRIANGLES_NUM)
            {
                const long long bx = base(rng), by = base(rng), bz = base(rng);
                T3 x; for (auto& p : x) p = {bx + d(rng), by + d(rng), bz + d(rng)};
                if (!flat(x)) t.push_back(x);
            }
            std::vector<Tri3> tri; for (auto& x : t) tri.push_back(to_real(x));
            std::vector<bool> want(t.size(), false);
            for (std::size_t i = 0; i < t.size(); ++i)
                for (std::size_t j = i + 1; j < t.size(); ++j)
                    if (oracle(t[i], t[j])) want[i] = want[j] = true;
            long long ones = 0; for (bool x : want) ones += x;
            CHECK(Scene<3>(tri).find_intersecting() == want);
            std::printf("    integer 3D scene, cube 0..%d: flagged %lld of %zu\n", range, ones, TRIANGLES_NUM);
        }

        // create a scene from 3 objects: two triangles and a point
        // test 3D scene
        Scene<3> deg({Tri3(V3(0, 0, 0), V3(4, 0, 0), V3(0, 4, 0)), Tri3(V3(1, 1, -1), V3(1, 1, 1), V3(2, 2, 0)), Tri3(V3(1, 1, 0), V3(1, 1, 0), V3(1, 1, 0))});
        auto f = deg.find_intersecting(); 
        CHECK(f[0] && f[1] && !f[2]);                     // a degenerate one is skipped for now
        // test 2D scene
        Scene<2> deg2({Tri2(V2(0, 0), V2(4, 0), V2(0, 4)), Tri2(V2(1, 1), V2(2, 1), V2(1, 2)), Tri2(V2(1, 1), V2(2, 2), V2(3, 3))});
        auto f2 = deg2.find_intersecting(); 
        CHECK(f2[0] && f2[1] && !f2[2]);
    }
    report("Scene: flags (BVH + exact test)", before);

    before = fails;   // ---------------------------------------------------------------- degenerate triangles
    {
        // a pair with a degenerate triangle (a point or a segment) is not considered: the answer is false in both orders
        
        // 3D
        const Tri3 big(V3(0, 0, 0), V3(4, 0, 0), V3(0, 4, 0));
        const Tri3 point(V3(1, 1, 0), V3(1, 1, 0), V3(1, 1, 0));                // lies inside big
        const Tri3 segment(V3(1, 1, -1), V3(1, 1, 0), V3(1, 1, 1));            // pierces big
        
        CHECK(!intersect(big, point) && !intersect(point, big));
        CHECK(!intersect(big, segment) && !intersect(segment, big));
        CHECK(!intersect(point, segment) && !intersect(point, point));
        
        // 2D
        const Tri2 big2(V2(0, 0), V2(4, 0), V2(0, 4));
        const Tri2 point2(V2(1, 1), V2(1, 1), V2(1, 1)), segment2(V2(1, 1), V2(2, 2), V2(3, 3)), far2(V2(9, 9), V2(9, 9), V2(9, 9));
        
        CHECK(!intersect(big2, point2) && !intersect(point2, big2));
        CHECK(!intersect(big2, segment2) && !intersect(segment2, big2));
        CHECK(!intersect(big2, far2) && !intersect(far2, big2));                // used to answer true
        
        // in a scene the degenerate triangle is ignored: it is not marked and does not mark the others
        Scene<3> scene({big, segment, point});
        const auto flags = scene.find_intersecting();
        CHECK(!flags[0] && !flags[1] && !flags[2]);
    }
    report("degenerate triangles are ignored", before);

    std::printf(fails ? "\nSOME CHECKS FAILED: %d\n" : "\nALL CHECKS PASSED\n", fails);
    return fails != 0;
}
