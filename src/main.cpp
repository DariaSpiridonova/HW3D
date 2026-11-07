#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>

#include "mesh_data.h"
#include "scene/scene.h"
#include "vertex_file.h"

#ifdef HW3D_GRAPHICS
#include "viewer.h"
#endif

/*
    hw3d [options] [scene file]
    The scene is read from the file, or from the standard input if there is no file

    --list         no window: print the numbers of the intersecting triangles, one per line
    --save F.vtx   save the vertices for the video card to the file F.vtx
    --view F.vtx   do not read a scene: show the vertices from the file F.vtx
*/

#ifdef HW3D_GRAPHICS
constexpr bool kGraphics = true;
#else
constexpr bool kGraphics = false;
#endif

struct Options
{
    bool list_only = false;
    std::string save_path;   // empty: do not save
    std::string view_path;   // empty: read a scene
    const char* scene_path = nullptr;
};

// Everything that happens after the scene is read
template <std::size_t N>
int process(const Scene<N>& scene, const Options& options)
{
    // geometry: intersecting[i] is true if the triangle i intersects another triangle
    const std::vector<bool> intersecting = scene.find_intersecting();

    // the vertices for the video card: 3 for every triangle
    const std::vector<Vertex> vertices = make_vertices(scene, intersecting);
    if (!options.save_path.empty()) save_vertices(options.save_path, vertices);

    if (kGraphics && !options.list_only && !scene.empty())
    {
#ifdef HW3D_GRAPHICS
        return run_viewer(make_view_data(vertices));
#endif
    }

    for (std::size_t i = 0; i < intersecting.size(); ++i)
        if (intersecting[i]) std::cout << i << '\n';
    return 0;
}

int main(int argc, char** argv)
{
    try
    {
        std::ios::sync_with_stdio(false);

        Options options;
        for (int i = 1; i < argc; ++i)
        {
            if (std::strcmp(argv[i], "--list") == 0) options.list_only = true;
            else if (std::strcmp(argv[i], "--save") == 0 && i + 1 < argc) options.save_path = argv[++i];
            else if (std::strcmp(argv[i], "--view") == 0 && i + 1 < argc) options.view_path = argv[++i];
            else options.scene_path = argv[i];
        }

        // show the vertices from a file: no geometry at all
        if (!options.view_path.empty())
        {
#ifdef HW3D_GRAPHICS
            return run_viewer(make_view_data(load_vertices(options.view_path)));
#else
            throw std::runtime_error("this build has no window (GLFW, GLEW or OpenGL were not found)");
#endif
        }

        // the input is a file from the command line, or the standard input if there is none
        std::ifstream file;
        std::istream* in = &std::cin;
        if (options.scene_path)
        {
            file.open(options.scene_path);
            if (!file) throw std::runtime_error(std::string("cannot open the file ") + options.scene_path);
            in = &file;
        }

        // the dimension (2 or 3) is known only from the input, so we get either Scene<2> or Scene<3>
        const std::variant<Scene<2>, Scene<3>> scene = read_scene(*in);

        // call process for whichever scene we got
        return std::visit([&](const auto& s) { return process(s, options); }, scene);
    }
    catch (const std::exception& error)
    {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
