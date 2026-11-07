#version 330 core
// Lines of the frame of the cube: one color

uniform vec3 u_color;

out vec4 color;

void main()
{
    color = vec4(u_color, 1.0);
}
