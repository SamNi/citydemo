    // Copyright [year] <Copyright Owner>
// Goal: isolate the direct OpenGL calls, enums to the back end
// Singletons are ugly but I can't think of anything better for this
#include "./Backend.h"
#include "./GL.h"
#include "./GLM.h"
#include "Shader.h"
#pragma warning(disable : 4800)

namespace Backend {

static const char*      DEFAULT_NAME =              "citydemo";
static const int        DEFAULT_WIDTH =             1600;
static const int        DEFAULT_HEIGHT =            900;
static const int        DEFAULT_XPOS =              100;
static const int        DEFAULT_YPOS =              50;
static const int        DEFAULT_FOV =               60;
static const int        OFFSCREEN_WIDTH =           64;
static const int        OFFSCREEN_HEIGHT =          64;

static void DisableBlending(void);
static void DrawFullscreenQuad(void);
static void EnableAdditiveBlending(void);
static void EnableBlending(void);

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
    // Query and cache misc. device parameters
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nMaxVertexAttribs);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &nMaxDrawBuffers);

    // Allocate things, prepare VBOs,*/

    return true;
}

void Shutdown(void) {
}

static GLuint fbo_id, fbTexID;
static GLint old_texID;

void Backend::BeginFrame(void) {
    /*
    if (offscreenRender) {
        static bool firstTime = true;
        if (firstTime) {
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_texID);
            checkGL();
            glGenTextures(1, &fbTexID);
            glBindTexture(GL_TEXTURE_2D, fbTexID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            checkGL();

            glGenFramebuffers(1, &fbo_id);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fbTexID, 0);
            checkGL();

            firstTime = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);  // rendering to offscreen buffer 
        glBindTexture(GL_TEXTURE_2D, old_texID);    // with the image we read
        glViewport(0, 0, OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);
    }*/
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    

    // ... scene draws go in between the conclusion of BeginFrame()
    // and the prologue of EndFrame()
    //TheShaderManager.use("ortho2d");
    DrawFullscreenQuad();
}

void Backend::EndFrame(void) {
    /*
    if (offscreenRender) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);       // switch to the visible render buffer
        glClear(GL_DEPTH_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, fbTexID);
        glViewport(0,0,DEFAULT_WIDTH,DEFAULT_HEIGHT);
        DrawFullscreenQuad();
    }*/
}

// private

// debugging util. get something on screen for me to test
static void DrawFullscreenQuad(void) {
    static bool firstTime = true;
    // these are all in UpLeft, DownLeft, DownRight, UpRight order
    static const GLfloat points[] = {
        // ccw order starting from lower left
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
    };
    static const GLfloat colors[] = {
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
    };
    static const GLfloat texCoords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
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
    LOG(LOG_CRITICAL, descr);
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
    Resize(w, h);
}

void Resize(int w, int h) {
    glViewport(0, 0, w, h);
}


} // ~namespace