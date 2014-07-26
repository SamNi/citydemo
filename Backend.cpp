// Copyright [year] <Copyright Owner>
// Goal: isolate the direct OpenGL calls, enums to the back end
// Singletons are ugly but I can't think of anything better for this
#include "./Backend.h"
#include "./GL.h"
#include "./GLM.h"
#pragma warning(disable : 4800)

namespace BackEnd {

static const char*      DEFAULT_NAME =              "citydemo";
static const int        DEFAULT_WIDTH =             1600;
static const int        DEFAULT_HEIGHT =            900;
static const int        DEFAULT_XPOS =              100;
static const int        DEFAULT_YPOS =              50;
static const int        DEFAULT_FOV =               60;

static void DisableBlending(void);
static void DrawRGBQuad(void);
static void EnableAdditiveBlending(void);
static void EnableBlending(void);
static void error_callback(int err, const char *descr);
static void key_callback(GLFWwindow *window, int key,
                         int scancode, int action, int mods);
static void size_callback(GLFWwindow *window, int width, int height);

int width, height;
float aspect;
float fov;

GLFWwindow *window;
    
// Device specifications to be queried once and then never again
// Stuff that does not change ever unless you buy a new video card
// Only backend.cpp should know about these
// Add more as the need to know arises
// TODO: Globals ugly, look into alternatives (thorny SWENG issue)
static int nMaxVertexAttribs;
static int nMaxDrawBuffers;


// public
// TODO: OpenGL state that changes often (cache these)
// consider: various kinds of bindings: textures, shaders, VBOs
// ...

bool Startup(void) {
    width = DEFAULT_WIDTH;
    height = DEFAULT_HEIGHT;
    aspect = static_cast<float>(DEFAULT_WIDTH)/DEFAULT_HEIGHT;
    fov = static_cast<float>(DEFAULT_FOV);

    // GLFW boilerplate
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        fprintf(stderr, "glfwInit failed\n");
        return false;
    }

    window = glfwCreateWindow(width, height,
        DEFAULT_NAME, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return false;
    }
    glfwSetWindowPos(window, DEFAULT_XPOS, DEFAULT_YPOS);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowSizeCallback(window, size_callback);

    // GLEW boilerplate
    glewExperimental = GL_TRUE;
    if (GLEW_OK != glewInit()) {
        fprintf(stderr, "glewInit failed\n");
        return false;
    }

    // Query and cache misc. device parameters
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nMaxVertexAttribs);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &nMaxDrawBuffers);

    DefaultGLState();

    // Allocate things, prepare VBOs,*/

    return true;
}

void Shutdown(void) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Done(void) {
    return static_cast<bool>(glfwWindowShouldClose(window));
}

void DefaultGLState(void) {
    int i;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POLYGON_SMOOTH);

    for (i = 0; i < nMaxVertexAttribs; ++i)
        glDisableVertexAttribArray(0);
}

void BackEnd::BeginFrame(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void BackEnd::EndFrame(void) {
    static bool firstTime = true;
    static GLuint fbo_id, texID;
    static GLint old_texID;
    checkGL();
    if (firstTime) {
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_texID);
        checkGL();
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        checkGL();

        glGenFramebuffers(1, &fbo_id);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
        glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, 8);
        glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, 8);
        //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texID, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texID, 0);
        checkGL();

        firstTime = false;
    }
    checkGL();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);  // rendering to offscreen buffer 
    glBindTexture(GL_TEXTURE_2D, old_texID);    // with the image we read
    glClear(GL_COLOR_BUFFER_BIT);               // Clear it first
    glDisable(GL_DEPTH_TEST);
    glViewport(0,0,8,8);
    DrawRGBQuad();                              // draw the quad
    glBindFramebuffer(GL_FRAMEBUFFER, 0);       // switch to the visible render buffer
    glBindTexture(GL_TEXTURE_2D, texID);
    glViewport(0,0,1600,900);
    DrawRGBQuad();
    glfwSwapBuffers(window);
    glfwPollEvents();
}

// private

// debugging util. get something on screen for me to test
static void DrawRGBQuad(void) {
    static bool firstTime = true;
    // these are all in UpLeft, DownLeft, DownRight, UpRight order
    static const GLfloat points[] = {
        // ccw order
        -1.0f,+1.0f, 0.0f,
        -1.0f,-1.0f, 0.0f,
        +1.0f,-1.0f, 0.0f,
        +1.0f,+1.0f, 0.0f,
    };
    static const GLfloat colors[] = {
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
    };
    static const GLfloat texCoords[] = {
        0.0f,0.0f, 
        0.0f,1.0f,  
        1.0f,1.0f,
        1.0f,0.0f, 
    };
    static const GLfloat normals[] = {
        -1.0f, +1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        +1.0f, -1.0f, -1.0f,
        +1.0f, +1.0f, -1.0f,
    };
    static GLuint vbo_position, vbo_colors, vbo_texCoords, vbo_normal, vao;
    if (firstTime) {
        glGenBuffers(1, &vbo_position);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
        glBufferData(GL_ARRAY_BUFFER, 3*4*sizeof(GLfloat), points, GL_STATIC_DRAW);
        checkGL();

        glGenBuffers(1, &vbo_colors);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
        glBufferData(GL_ARRAY_BUFFER, 4*4*sizeof(GLfloat), colors, GL_STATIC_DRAW);
        checkGL();

        glGenBuffers(1, &vbo_texCoords);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_texCoords);
        glBufferData(GL_ARRAY_BUFFER, 2*4*sizeof(GLfloat), texCoords, GL_STATIC_DRAW);
        checkGL();

        glGenBuffers(1, &vbo_normal);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_normal);
        glBufferData(GL_ARRAY_BUFFER, 3*4*sizeof(GLfloat), normals, GL_STATIC_DRAW);
        checkGL();

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_texCoords);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_normal);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
        checkGL();
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        checkGL();

        firstTime = false;
    } else
        glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

static void EnableBlending(void) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
static void DisableBlending(void) {
    glDisable(GL_BLEND);
}
static void EnableAdditiveBlending(void) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

// GLFW callbacks
static void error_callback(int err, const char *descr) {
    fprintf(stderr, descr);
}
static void key_callback(GLFWwindow *window, int key, int scancode,
                         int action, int mods) {
    if (action != GLFW_PRESS)
        return;

    switch (key) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GL_TRUE);
        break;
    }
}
static void size_callback(GLFWwindow *window, int w, int h) {
    glViewport(0, 0, w, h);
    width = w;
    height = h;
    aspect = static_cast<float>(w)/h;
}


} // ~namespace