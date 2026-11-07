#include "viewer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <GL/glew.h>     // must be included before GLFW
#include <GLFW/glfw3.h>

#include "camera.h"

// The folder with the shader files (.vert and .frag), CMake sets it
#ifndef HW3D_SHADER_DIR
#define HW3D_SHADER_DIR "shaders"
#endif

namespace
{
    constexpr float kPi = 3.14159265358979f;

    // The whole text file in a string
    std::string read_text_file(const std::string& path)
    {
        std::ifstream in(path);
        if (!in) throw std::runtime_error("cannot open the shader file " + path);
        std::ostringstream text;
        text << in.rdbuf();
        return text.str();
    }

    // Compile one shader, if it does not compile: throw with the log
    GLuint compile_shader(GLenum type, const char* source)
    {
        const GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char log[1024];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            throw std::runtime_error(std::string("shader does not compile:\n") + log);
        }
        return shader;
    }

    // Two shaders from the files linked into one program
    GLuint build_program(const std::string& vertex_file, const std::string& fragment_file)
    {
        const std::string vertex_source = read_text_file(std::string(HW3D_SHADER_DIR) + "/" + vertex_file);
        const std::string fragment_source = read_text_file(std::string(HW3D_SHADER_DIR) + "/" + fragment_file);
        const GLuint vertex = compile_shader(GL_VERTEX_SHADER, vertex_source.c_str());
        const GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_source.c_str());

        const GLuint program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);

        GLint ok = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            char log[1024];
            glGetProgramInfoLog(program, sizeof(log), nullptr, log);
            throw std::runtime_error(std::string("program does not link:\n") + log);
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return program;
    }

    // A buffer in the video memory and the object that remembers how to read it
    struct GpuBuffer
    {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLsizei vertex_count = 0;
    };

    // Triangles: the vertices are sent as they are, the video card reads them by the layout of Vertex
    GpuBuffer upload_triangles(const std::vector<Vertex>& vertices)
    {
        GpuBuffer buffer;
        buffer.vertex_count = static_cast<GLsizei>(vertices.size());

        glGenVertexArrays(1, &buffer.vao);
        glGenBuffers(1, &buffer.vbo);
        glBindVertexArray(buffer.vao);
        glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

        // attribute 0: x, y, z     attribute 1: nx, ny, nz     attribute 2: isRed
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, x)));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, nx)));
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, isRed)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        return buffer;
    }

    // Lines: only the position (3 floats)
    GpuBuffer upload_lines(const std::vector<float>& data)
    {
        GpuBuffer buffer;
        buffer.vertex_count = static_cast<GLsizei>(data.size() / 3);

        glGenVertexArrays(1, &buffer.vao);
        glGenBuffers(1, &buffer.vbo);
        glBindVertexArray(buffer.vao);
        glBindBuffer(GL_ARRAY_BUFFER, buffer.vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        return buffer;
    }
}

int run_viewer(const ViewData& data)
{
    if (!glfwInit()) throw std::runtime_error("glfwInit failed: is there a display?");

    // we ask for OpenGL 3.3, core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1000, 700, "HW3D", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        throw std::runtime_error("cannot create a window with OpenGL 3.3");
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);   // one frame per screen refresh

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        glfwTerminate();
        throw std::runtime_error("glewInit failed");
    }
    glEnable(GL_DEPTH_TEST);   // the nearer triangle hides the farther one

    const GLuint triangle_program = build_program("triangle.vert", "triangle.frag");
    const GLuint line_program = build_program("line.vert", "line.frag");
    const GLint triangle_matrix = glGetUniformLocation(triangle_program, "u_view_projection");
    const GLint triangle_camera = glGetUniformLocation(triangle_program, "u_camera_position");
    const GLint line_matrix = glGetUniformLocation(line_program, "u_view_projection");
    const GLint line_color = glGetUniformLocation(line_program, "u_color");

    const GpuBuffer triangles = upload_triangles(data.triangle_vertices);
    const GpuBuffer box = upload_lines(data.box_lines);

    // The camera starts in front of the scene and looks at its center
    const Vector<float, 3> low(data.bounds.min[0], data.bounds.min[1], data.bounds.min[2]);
    const Vector<float, 3> high(data.bounds.max[0], data.bounds.max[1], data.bounds.max[2]);
    const Vector<float, 3> center = (low + high) * 0.5f;
    float radius = 0.5f * (high - low).length();
    if (radius < 1e-6f) radius = 1.0f;

    const Camera start_camera(center + Vector<float, 3>(0.0f, 0.0f, 2.5f * radius), -kPi / 2, 0.0f);
    Camera camera = start_camera;
    const float near_plane = 0.005f * radius;
    const float far_plane = 50.0f * radius;

    bool orbit = false;            // the camera flies around the scene by itself
    float orbit_angle = 0.0f;
    bool orbit_key_was_down = false;
    bool reset_key_was_down = false;
    bool dragging = false;         // the left button is held
    double last_x = 0.0, last_y = 0.0;

    double previous_time = glfwGetTime();
    double fps_time = previous_time;
    int frames = 0;

    auto pressed = [&](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };

    while (!glfwWindowShouldClose(window))
    {
        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - previous_time);   // seconds since the last frame
        previous_time = now;

        if (pressed(GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, GLFW_TRUE);

        // O switches the flight around the scene (one press = one switch, not while the key is held)
        const bool orbit_key_down = pressed(GLFW_KEY_O);
        if (orbit_key_down && !orbit_key_was_down)
        {
            orbit = !orbit;
            if (orbit) orbit_angle = std::atan2(camera.position()[0] - center[0], camera.position()[2] - center[2]);
        }
        orbit_key_was_down = orbit_key_down;

        const bool reset_key_down = pressed(GLFW_KEY_R);
        if (reset_key_down && !reset_key_was_down)
        {
            camera = start_camera;
            orbit = false;
        }
        reset_key_was_down = reset_key_down;

        // Mouse: look around while the left button is held
        double x = 0.0, y = 0.0;
        glfwGetCursorPos(window, &x, &y);
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            const float sensitivity = 0.0035f;   // radians per pixel
            if (dragging) camera.rotate(static_cast<float>(x - last_x) * sensitivity, static_cast<float>(last_y - y) * sensitivity);
            dragging = true;
            last_x = x;
            last_y = y;
        }
        else dragging = false;

        if (orbit)
        {
            orbit_angle += 0.4f * dt;
            const float orbit_radius = 2.2f * radius;
            camera.set_position(center + Vector<float, 3>(orbit_radius * std::sin(orbit_angle), 0.45f * radius, orbit_radius * std::cos(orbit_angle)));
            camera.look_at_point(center);
        }
        else
        {
            // the speed depends on the size of the scene, so that it is comfortable for any scene
            const float step = radius * 0.6f * (pressed(GLFW_KEY_LEFT_SHIFT) ? 4.0f : 1.0f) * dt;
            float forward = 0.0f, to_right = 0.0f, up = 0.0f;
            if (pressed(GLFW_KEY_W)) forward += step;
            if (pressed(GLFW_KEY_S)) forward -= step;
            if (pressed(GLFW_KEY_D)) to_right += step;
            if (pressed(GLFW_KEY_A)) to_right -= step;
            if (pressed(GLFW_KEY_E)) up += step;
            if (pressed(GLFW_KEY_Q)) up -= step;
            camera.move(forward, to_right, up);
        }

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (height == 0) height = 1;   // the window is minimized
        glViewport(0, 0, width, height);

        const Mat4 view_projection = multiply(perspective(kPi / 3, static_cast<float>(width) / static_cast<float>(height), near_plane, far_plane), camera.view());

        glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(triangle_program);
        glUniformMatrix4fv(triangle_matrix, 1, GL_FALSE, view_projection.m.data());
        glUniform3f(triangle_camera, camera.position()[0], camera.position()[1], camera.position()[2]);
        glBindVertexArray(triangles.vao);
        glDrawArrays(GL_TRIANGLES, 0, triangles.vertex_count);

        glUseProgram(line_program);
        glUniformMatrix4fv(line_matrix, 1, GL_FALSE, view_projection.m.data());
        glUniform3f(line_color, 0.75f, 0.75f, 0.80f);
        glBindVertexArray(box.vao);
        glDrawArrays(GL_LINES, 0, box.vertex_count);

        glfwSwapBuffers(window);   // show the finished frame
        glfwPollEvents();          // keys, mouse, closing the window

        // the counters in the title of the window, twice a second
        ++frames;
        if (now - fps_time >= 0.5)
        {
            const std::string title = "HW3D: " + std::to_string(data.triangles_num) + " triangles, " + std::to_string(data.red_num)
                                      + " intersecting, " + std::to_string(static_cast<int>(frames / (now - fps_time))) + " fps";
            glfwSetWindowTitle(window, title.c_str());
            fps_time = now;
            frames = 0;
        }
    }

    glDeleteVertexArrays(1, &triangles.vao);
    glDeleteBuffers(1, &triangles.vbo);
    glDeleteVertexArrays(1, &box.vao);
    glDeleteBuffers(1, &box.vbo);
    glDeleteProgram(triangle_program);
    glDeleteProgram(line_program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
