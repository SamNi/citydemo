#include "citydemo.h"
#include <GLFW/glfw3.h>
#pragma warning ( disable : 4100 )

Renderer renderer;
Scene scene;

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
    renderer.Reshape(width, height);
}

int main(int argc, char *argv[]) {
    GLFWwindow *window;
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    window = glfwCreateWindow(1600, 900, argv[0], NULL, NULL);
    if(!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowSizeCallback(window, size_callback);

    renderer.Init(1600, 900);
    while(!glfwWindowShouldClose(window)) {
        scene.step(3.0);
        renderer.Render(scene);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}