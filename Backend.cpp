// Copyright 2014 SamNi PlaceholderLicenseText
// Goal: isolate the direct OpenGL calls, enums to the back end
#include "Backend.h"
#include "GL.h"
#include "GLM.h"
#pragma warning ( disable : 4800 )

static const char *DEFAULT_NAME = "citydemo";
static const int DEFAULT_WIDTH = 1600;
static const int DEFAULT_HEIGHT = 900;
static const int DEFAULT_XPOS = 100;
static const int DEFAULT_YPOS = 50;

static void error_callback(int err, const char *descr);
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void size_callback(GLFWwindow *window, int width, int height);

struct BackEnd::BackEnd_Internal {
    int width, height;
    float mAspect;
    float mFov;

    GLFWwindow *window;
};

bool BackEnd::Startup(void) {
    mInternal = new BackEnd_Internal;

    mInternal->width = DEFAULT_WIDTH;
    mInternal->height = DEFAULT_HEIGHT;

    // GLFW boilerplate
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return false;

    mInternal->window = glfwCreateWindow(mInternal->width, mInternal->height,
        DEFAULT_NAME, nullptr, nullptr);
    if(!mInternal->window) {
        glfwTerminate();
        return false;
    }
    glfwSetWindowPos(mInternal->window, DEFAULT_XPOS, DEFAULT_YPOS);
    glfwMakeContextCurrent(mInternal->window);
    glfwSetKeyCallback(mInternal->window, key_callback);
    glfwSetWindowSizeCallback(mInternal->window, size_callback);

    // Allocate things, prepare VBOs,*/

    return true;
}

void BackEnd::Shutdown(void) {
    glfwDestroyWindow(mInternal->window);
    glfwTerminate();
    delete mInternal;
    mInternal = nullptr;
}

bool BackEnd::Done(void) const {
    return (bool)glfwWindowShouldClose(mInternal->window);
}

void BackEnd::DefaultGLState(void) {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDisable(GL_BLEND);
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POLYGON_SMOOTH);
}

void BackEnd::BeginFrame(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void BackEnd::EndFrame(void) {
    glfwSwapBuffers(mInternal->window);
    glfwPollEvents();
}

// GLFW callbacks
static void error_callback(int err, const char *descr) {
    fprintf(stderr, descr);
}
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action!=GLFW_PRESS)
        return;

    switch(key) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GL_TRUE);
        break;
    }
}
static void size_callback(GLFWwindow *window, int width, int height) {
    //theGPU.Resize(width, height);
}
