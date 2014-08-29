#ifdef _TEST_BUILD
#include "../Renderer/Backend/Backend_local.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>

static const int TEST_WIDTH = 160;
static const int TEST_HEIGHT = 100;

struct framebuffer_fixture : public ::testing::Test {
    virtual void SetUp(void) {
        glfwInit();
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        window = glfwCreateWindow(TEST_WIDTH, TEST_HEIGHT, "gtest", nullptr, nullptr);
        glfwSetWindowPos(window, 300, 300);
        glfwMakeContextCurrent(window);
        ASSERT_EQ(GLEW_OK, glewInit());
        Backend::startup(TEST_WIDTH, TEST_HEIGHT);

        Backend::show_hud(false);
    }
    virtual void TearDown(void) {
        Backend::shutdown();
        glfwTerminate();
        window = nullptr;
    }
    GLFWwindow *window;
};
#if 0
TEST_F(framebuffer_fixture, asdf) {
    Framebuffer fb(TEST_WIDTH, TEST_HEIGHT);
    fb.bind();
}
#endif
#endif  // ~_TEST_BUILD