#version 330 core
// Vertex shader: runs once for every vertex of every triangle

layout(location = 0) in vec3 a_position;   // x, y, z
layout(location = 1) in vec3 a_normal;     // nx, ny, nz
layout(location = 2) in float a_isRed;     // 1.0 if the triangle intersects another one

uniform mat4 u_view_projection;            // the matrix of the camera

out vec3 v_position;
out vec3 v_normal;
out float v_isRed;

void main()
{
    v_position = a_position;
    v_normal = a_normal;
    v_isRed = a_isRed;
    gl_Position = u_view_projection * vec4(a_position, 1.0);
}
