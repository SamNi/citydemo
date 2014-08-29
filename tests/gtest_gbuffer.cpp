#include "../Renderer/Backend/Backend_local.h"
#include <gtest/gtest.h>
#include "gtest_utils.h"

static const int TEST_WIDTH =       256;
static const int TEST_HEIGHT =      256;
struct gbuffer_fixture : public ::testing::Test {
    virtual void SetUp(void) {
        glfwInit();
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        window = glfwCreateWindow(TEST_WIDTH, TEST_HEIGHT, "gtest", nullptr, nullptr);
        glfwSetWindowPos(window, 300, 300);
        glfwMakeContextCurrent(window);
        ASSERT_EQ(GLEW_OK, glewInit());
        Backend::startup(TEST_WIDTH, TEST_HEIGHT);
        Backend::disable_depth_testing();
        Backend::show_hud(false);
    }
    virtual void TearDown() {
        Backend::shutdown();
        glfwTerminate();
        window = nullptr;
    }
    GLFWwindow *window;
};

TEST_F(gbuffer_fixture, gbuffer_basic) {
}