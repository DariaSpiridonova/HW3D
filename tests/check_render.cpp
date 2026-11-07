#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>

#include "test_common.h"
#include "camera.h"
#include "mesh_data.h"
#include "vertex_file.h"

/*
    Checks of the part of the viewer that does not need OpenGL:
    the vertices for the video card, the file with them and the math of the camera
*/

// Checks if two floating-point numbers are nearly equal
static bool near(float a, float b, float eps = 1e-4f) { return std::abs(a - b) < eps; }
using F3 = Vector<float, 3>;

// // Transforms a point from world space to NDC (screenspace) with perspective division by w
static F3 transform(const Mat4& matrix, const F3& p, float* w_out = nullptr)
{
    float result[4];
    for (int row = 0; row < 4; ++row)
        result[row] = matrix.m[0 * 4 + row] * p[0] + 
                      matrix.m[1 * 4 + row] * p[1] + 
                      matrix.m[2 * 4 + row] * p[2] + 
                      matrix.m[3 * 4 + row];

    if (w_out) *w_out = result[3];
    return F3(result[0] / result[3], result[1] / result[3], result[2] / result[3]);
}

// // Returns a cross-platform absolute path for a file in the system's temporary directory
static std::string temp_path(const char* name) { return (std::filesystem::temp_directory_path() / name).string(); }

// the vertices of a small scene: 0 and 1 intersect, 2 is far away
static std::vector<Vertex> sample_vertices()
{
    Scene<3> scene({Tri3(V3(0, 0, 0), V3(4, 0, 0), V3(0, 4, 0)), Tri3(V3(1, 1, -1), V3(1, 1, 1), V3(2, 2, 0)), Tri3(V3(10, 10, 10), V3(11, 10, 10), V3(10, 11, 10))});
    return make_vertices(scene, scene.find_intersecting());
}

int main()
{
    int before = 0;

    before = fails;   // ---------------------------------------------------------------- the layout of Vertex
    {
        CHECK(sizeof(Vertex) == 28 && sizeof(Vertex) == 7 * sizeof(float));
        CHECK(offsetof(Vertex, x) == 0 && offsetof(Vertex, y) == 4 && offsetof(Vertex, z) == 8);
        CHECK(offsetof(Vertex, nx) == 12 && offsetof(Vertex, ny) == 16 && offsetof(Vertex, nz) == 20 && offsetof(Vertex, isRed) == 24);
    }
    report("the layout of Vertex (28 bytes, no gaps)", before);

    before = fails;   // ---------------------------------------------------------------- vertices of a 3D scene
    {
        // 0 and 1 intersect, 2 is far away, 3 is degenerate (a segment)
        Scene<3> scene({Tri3(V3(0, 0, 0), V3(4, 0, 0), V3(0, 4, 0)),
                        Tri3(V3(1, 1, -1), V3(1, 1, 1), V3(2, 2, 0)),
                        Tri3(V3(10, 10, 10), V3(11, 10, 10), V3(10, 11, 10)),
                        Tri3(V3(0, 0, 5), V3(1, 1, 5), V3(2, 2, 5))});
        const std::vector<bool> flags = scene.find_intersecting();
        const std::vector<Vertex> v = make_vertices(scene, flags);

        CHECK(v.size() == 3 * 3);                                                  // the segment is not drawn
        CHECK(v[1].x == 4 && v[1].y == 0 && v[1].z == 0);                          // the second vertex of the first triangle
        CHECK(near(v[0].nx, 0) && near(v[0].ny, 0) && near(v[0].nz, 1));           // its normal is along +z
        CHECK(v[0].isRed == 1 && v[1].isRed == 1 && v[2].isRed == 1);              // all three vertices are red
        CHECK(v[3].isRed == 1 && v[6].isRed == 0 && v[8].isRed == 0);              // the far triangle is not red
        for (const Vertex& vertex : v)                                              // normals have length 1
            CHECK(near(std::sqrt(vertex.nx * vertex.nx + vertex.ny * vertex.ny + vertex.nz * vertex.nz), 1));
        for (std::size_t t = 0; t < 3; ++t)                                         // the same normal and flag inside a triangle
            for (std::size_t k = 1; k < 3; ++k)
                CHECK(v[t * 3 + k].nx == v[t * 3].nx && v[t * 3 + k].ny == v[t * 3].ny && v[t * 3 + k].nz == v[t * 3].nz && v[t * 3 + k].isRed == v[t * 3].isRed);

        const ViewData data = make_view_data(scene, flags);
        CHECK(data.triangles_num == 3 && data.red_num == 2);
        CHECK(data.bounds.min[0] == 0 && data.bounds.min[2] == -1 && data.bounds.max[0] == 11 && data.bounds.max[2] == 10);
    }
    report("vertices of a 3D scene", before);

    before = fails;   // ---------------------------------------------------------------- a 2D scene is placed to z = 0
    {
        Scene<2> scene({Tri2(V2(0, 0), V2(4, 0), V2(0, 4)), Tri2(V2(1, 1), V2(2, 1), V2(1, 2)), Tri2(V2(9, 9), V2(10, 9), V2(9, 10))});
        const ViewData data = make_view_data(scene, scene.find_intersecting());
        CHECK(data.triangles_num == 3 && data.red_num == 2);
        
        // check the correct projection of 2D triangles into the 3D world
        bool flat = true;
        for (const Vertex& v : data.triangle_vertices) 
            flat = flat && v.z == 0 && v.nx == 0 && v.ny == 0 && v.nz == 1;
        CHECK(flat);                                                                // z = 0, normal (0, 0, 1)
        CHECK(data.bounds.min[2] == 0 && data.bounds.max[2] == 0 && data.bounds.max[0] == 10);
    }
    report("vertices of a 2D scene", before);

    before = fails;   // ---------------------------------------------------------------- view data from the vertices only, the frame
    {
        const ViewData empty = make_view_data(std::vector<Vertex>{});               // all triangles were degenerate
        CHECK(empty.triangles_num == 0 && empty.red_num == 0 && empty.box_lines.size() == 72);
        CHECK(empty.bounds.min[0] == 0 && empty.bounds.max[2] == 0);

        Bounds3 box; box.min = {-1, 2, 3}; box.max = {5, 6, 9};
        const std::vector<float> lines = make_box_lines(box);
        CHECK(lines.size() == 24 * 3);
        std::set<std::array<float, 3>> corners;
        int axis_aligned = 0;
        for (std::size_t edge = 0; edge < 12; ++edge)
        {
            const float* a = &lines[edge * 6];
            const float* b = a + 3;
            int different = 0;
            for (int k = 0; k < 3; ++k) different += a[k] != b[k];
            axis_aligned += different == 1;                                         // an edge changes exactly one coordinate
            corners.insert({a[0], a[1], a[2]}); corners.insert({b[0], b[1], b[2]});
            for (int k = 0; k < 3; ++k) CHECK(a[k] == box.min[k] || a[k] == box.max[k]);
        }
        CHECK(axis_aligned == 12 && corners.size() == 8);                           // 12 edges, 8 different corners
    }
    report("view data from the vertices, the frame of the cube", before);

    before = fails;   // ---------------------------------------------------------------- the file with the vertices
    {
        const std::string path = temp_path("hw3d_check.vtx");
        const std::vector<Vertex> original = sample_vertices(); // the vertices in RAM
        save_vertices(path, original);
        CHECK(std::filesystem::file_size(path) == 16 + original.size() * 28);       // header + the vertices as they are

        const std::vector<Vertex> loaded = load_vertices(path);
        CHECK(loaded.size() == original.size() && std::memcmp(loaded.data(), original.data(), original.size() * sizeof(Vertex)) == 0);

        // the viewer gets the same data from the file as from the memory
        const ViewData a = make_view_data(original), b = make_view_data(loaded);
        CHECK(a.triangles_num == b.triangles_num && a.red_num == b.red_num && a.bounds.min == b.bounds.min && a.bounds.max == b.bounds.max && a.box_lines == b.box_lines);

        save_vertices(path, {});                                                    // an empty file is correct
        CHECK(load_vertices(path).empty());

        // checking the response to corrupted files
        auto throws = [](const std::string& file) { try { load_vertices(file); } catch (const std::runtime_error&) { return true; } return false; };
        CHECK(throws(temp_path("hw3d_no_such_file.vtx")));
        { std::ofstream(path) << "this is just a text, not a vertex file at all"; }
        CHECK(throws(path));
        { std::ofstream(path, std::ios::binary) << "HW3D"; }                        // shorter than the header
        CHECK(throws(path));

        save_vertices(path, original);                                              // cut the last byte
        std::filesystem::resize_file(path, std::filesystem::file_size(path) - 1);
        CHECK(throws(path));

        // attempt to overflow the heap
        save_vertices(path, original);                                              // a lie in the header: 10^15 vertices
        { std::fstream f(path, std::ios::in | std::ios::out | std::ios::binary); f.seekp(8); const std::uint64_t lie = 1000000000000000ull; f.write(reinterpret_cast<const char*>(&lie), 8); }
        CHECK(throws(path));

        save_vertices(path, std::vector<Vertex>(4));                                // 4 vertices are not whole triangles
        CHECK(throws(path));
        std::filesystem::remove(path);
    }
    report("the file with the vertices (.vtx)", before);

    before = fails;   // ---------------------------------------------------------------- matrices
    {
        // the camera at (0, 0, 5) looks at the origin
        const Mat4 view = look_at(F3(0, 0, 5), F3(0, 0, 0), F3(0, 1, 0));
        // check projections
        CHECK(near(transform(view, F3(0, 0, 5))[2], 0) && near(transform(view, F3(0, 0, 5))[0], 0));    // the eye goes to the origin
        
        // check axes directions
        const F3 target = transform(view, F3(0, 0, 0));
        CHECK(near(target[0], 0) && near(target[1], 0) && near(target[2], -5));                       // the target is in front, along -z
        
        // check axis orientations
        const F3 right = transform(view, F3(1, 0, 0));
        CHECK(near(right[0], 1) && near(right[2], -5));                                                 // +x stays to the right
        const F3 up = transform(view, F3(0, 1, 0));
        CHECK(near(up[1], 1));                                                                          // +y stays up

        // perspective: the near plane goes to -1, the far plane to +1, the edge of the view at the near plane to x = 1
        const Mat4 projection = perspective(1.5707963f, 1.0f, 1.0f, 10.0f);                           // 90 degrees
        CHECK(near(transform(projection, F3(0, 0, -1))[2], -1));
        CHECK(near(transform(projection, F3(0, 0, -10))[2], 1));
        CHECK(near(transform(projection, F3(1, 0, -1))[0], 1) && near(transform(projection, F3(0, 1, -1))[1], 1));
        // check density = w
        float w = 0; transform(projection, F3(0, 0, -4), &w); CHECK(near(w, 4));                      // w is the distance

        // the product: first the view, then the projection
        const Mat4 both = multiply(projection, view);
        const F3 p = transform(both, F3(0, 0, 4));                                                      // 1 in front of the camera
        CHECK(near(p[2], -1));
        const Mat4 identity = [] { Mat4 i; i.m[0] = i.m[5] = i.m[10] = i.m[15] = 1; return i; }();
        const Mat4 same = multiply(identity, view);
        bool equal = true; for (int i = 0; i < 16; ++i) equal = equal && near(same.m[i], view.m[i]);
        CHECK(equal);
    }
    report("matrices: view, projection, product", before);

    before = fails;   // ---------------------------------------------------------------- the camera
    {
        const float pi = 3.14159265f;
        Camera camera(F3(0, 0, 5), -pi / 2, 0); // yaw = -pi / 2, pitch = 0
        const F3 front = camera.front(), right = camera.right();
        CHECK(near(front[0], 0) && near(front[1], 0) && near(front[2], -1));
        CHECK(near(right[0], 1) && near(right[1], 0) && near(right[2], 0));

        camera.move(2, 0, 0);                                                                           // forward
        CHECK(near(camera.position()[2], 3));
        camera.move(0, 1.5f, 0);                                                                        // to the right
        CHECK(near(camera.position()[0], 1.5f));
        camera.move(0, 0, -1);                                                                          // down
        CHECK(near(camera.position()[1], -1));

        camera.rotate(pi / 2, 0);                                                                       // turn right by 90 degrees: look along +x
        CHECK(near(camera.front()[0], 1) && near(camera.front()[2], 0));
        camera.rotate(0, 10);  // too big angle                                                                           // can not look over the vertical
        CHECK(camera.front()[1] < 1.0f && camera.front()[1] > 0.99f);
        camera.rotate(0, -20); 
        CHECK(camera.front()[1] > -1.0f && camera.front()[1] < -0.99f);

        camera.set_position(F3(3, 4, 5));
        camera.look_at_point(F3(3, 4, -5));                                                             // straight along -z
        CHECK(near(camera.front()[0], 0) && near(camera.front()[1], 0) && near(camera.front()[2], -1));
        camera.look_at_point(F3(10, 4, 5));                                                             // to +x
        CHECK(near(camera.front()[0], 1) && near(camera.front()[2], 0));

        // the target is on the screen axis after look_at_point
        camera.set_position(F3(2, 3, 9));
        camera.look_at_point(F3(0, 0, 0));
        const F3 on_axis = transform(camera.view(), F3(0, 0, 0));
        CHECK(near(on_axis[0], 0) && near(on_axis[1], 0) && on_axis[2] < 0);
    }
    report("camera: move, turn, look at a point", before);

    std::printf(fails ? "\nSOME CHECKS FAILED: %d\n" : "\nALL RENDER CHECKS PASSED\n", fails);
    return fails != 0;
}
