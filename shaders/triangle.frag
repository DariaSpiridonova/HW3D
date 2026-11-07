#version 330 core
// Fragment shader: runs once for every pixel of a triangle and chooses its color
// Lighting: ambient + the main light fixed in the world + a weak light near the camera + a highlight
// Intersecting triangles are red

in vec3 v_position;
in vec3 v_normal;
in float v_isRed;

uniform vec3 u_camera_position;

out vec4 color;

void main()
{
    vec3 base = v_isRed > 0.5 ? vec3(0.92, 0.12, 0.12) : vec3(0.55, 0.70, 0.95);

    vec3 to_camera = normalize(u_camera_position - v_position);
    vec3 normal = normalize(v_normal);
    // a triangle is seen from both sides: turn the normal to the viewer
    if (dot(normal, to_camera) < 0.0) normal = -normal;

    vec3 to_light = normalize(vec3(0.5, 0.8, 0.6));
    float main_light = max(dot(normal, to_light), 0.0);
    float camera_light = dot(normal, to_camera);

    vec3 half_vector = normalize(to_light + to_camera);
    float highlight = pow(max(dot(normal, half_vector), 0.0), 48.0);

    vec3 result = base * (0.20 + 0.55 * main_light + 0.35 * camera_light) + vec3(0.25) * highlight;
    color = vec4(result, 1.0);
}
