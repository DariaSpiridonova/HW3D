#include <cstdio>
#include <cstdlib>

#include <GL/glew.h>     
#include <GLFW/glfw3.h>

/*
    The first check of the graphics environment: a window and one triangle
    If it works, GLFW, GLEW and the OpenGL driver are installed correctly
    Esc closes the window
*/

// Shaders are small programs for the video card, here they are just strings
static const char* vertex_source = R"(#version 330 core
layout(location = 0) in vec3 position;
void main()
{
    gl_Position = vec4(position, 1.0);
}
)";

static const char* fragment_source = R"(#version 330 core
out vec4 color;
void main()
{
    color = vec4(0.9, 0.2, 0.2, 1.0);
}
)";

// Compile one shader, if it does not compile: print the log and stop
static GLuint compile_shader(GLenum type, const char* source)
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
        std::fprintf(stderr, "shader does not compile:\n%s\n", log);
        std::exit(1);
    }
    return shader;
}

// Two shaders linked into one program
static GLuint build_program()
{
    const GLuint vertex = compile_shader(GL_VERTEX_SHADER, vertex_source);
    const GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_source);

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
        std::fprintf(stderr, "program does not link:\n%s\n", log);
        std::exit(1);
    }

    glDeleteShader(vertex);   // they are inside the program now
    glDeleteShader(fragment);
    return program;
}

int main()
{
    if (!glfwInit())
    {
        std::fprintf(stderr, "glfwInit failed: is there a display? (it does not work over ssh or in a console)\n");
        return 1;
    }

    // we ask for OpenGL 3.3, core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "HW3D: hello window", nullptr, nullptr);
    if (!window)
    {
        std::fprintf(stderr, "cannot create a window with OpenGL 3.3: the driver is too old?\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    // GLEW finds the addresses of the OpenGL functions
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::fprintf(stderr, "glewInit failed\n");
        glfwTerminate();
        return 1;
    }

    std::printf("OpenGL: %s\n", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    std::printf("GPU:    %s\n", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    const GLuint program = build_program();

    // three points of the triangle: x, y, z
    const float vertices[] = {
        -0.6f, -0.5f, 0.0f,
         0.6f, -0.5f, 0.0f,
         0.0f,  0.6f, 0.0f,
    };

    // vbo is the buffer in the video memory, vao remembers how to read it
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);   // attribute 0 = 3 floats per vertex
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);   // show the finished frame
        glfwPollEvents();          // keys, mouse, closing the window
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
