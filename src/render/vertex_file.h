#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "view_data.h"

/*
    The file with the vertices: exactly what is sent to the video card, saved as it is. The extension is .vtx
    The format is binary, in the byte order of this computer (little-endian on x86 and ARM):
        8 bytes   the sign "HW3DVTX1"
        8 bytes   the number of vertices (unsigned 64 bit)
        then the vertices, 7 floats (28 bytes) each
*/

namespace detailsFile
{
    inline constexpr char kSign[8] = {'H', 'W', '3', 'D', 'V', 'T', 'X', '1'};
    inline constexpr std::uint64_t kHeaderSize = 16;
}

inline void save_vertices(const std::string& path, const std::vector<Vertex>& vertices)
{
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("cannot create the file " + path);

    const std::uint64_t count = vertices.size();
    out.write(detailsFile::kSign, sizeof(detailsFile::kSign));
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    out.write(reinterpret_cast<const char*>(vertices.data()), static_cast<std::streamsize>(vertices.size() * sizeof(Vertex)));
    if (!out) throw std::runtime_error("cannot write the file " + path);
}

// Get the vertices from the file, exactly as they were saved
// Throws std::runtime_error if the file does not exist or is not a vertex file or is damaged
inline std::vector<Vertex> load_vertices(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open the file " + path);

    char sign[8] = {};
    std::uint64_t count = 0;
    in.read(sign, sizeof(sign));
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!in || std::memcmp(sign, detailsFile::kSign, sizeof(sign)) != 0)
        throw std::runtime_error(path + " is not a vertex file");

    // the number comes from the file: compare it with the real size before allocating the memory
    in.seekg(0, std::ios::end);
    const std::uint64_t file_size = static_cast<std::uint64_t>(in.tellg());
    const std::uint64_t data_size = file_size - detailsFile::kHeaderSize;
    if (data_size % sizeof(Vertex) != 0 || data_size / sizeof(Vertex) != count)
        throw std::runtime_error("the vertex file " + path + " is damaged: the size does not match the number of vertices");
    if (count % 3 != 0)
        throw std::runtime_error("the vertex file " + path + " is damaged: the number of vertices is not a multiple of 3");

    std::vector<Vertex> vertices(static_cast<std::size_t>(count));
    in.seekg(static_cast<std::streamoff>(detailsFile::kHeaderSize));
    in.read(reinterpret_cast<char*>(vertices.data()), static_cast<std::streamsize>(data_size));
    if (!in) throw std::runtime_error("cannot read the file " + path);
    return vertices;
}
