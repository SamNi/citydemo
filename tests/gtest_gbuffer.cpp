#include "../Renderer/Backend/Backend_local.h"
#include <gtest/gtest.h>
#include "gtest_utils.h"

static const int TEST_WIDTH =       1280;
static const int TEST_HEIGHT =      720;

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

TEST_F(gbuffer_fixture, asdf) {
    //GBuffer x(TEST_WIDTH, TEST_HEIGHT);
    //x.bind();
    Backend::begin_frame();
    glBindVertexArray(QuadVAO::get_vao());
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    Backend::end_frame();
    Backend::write_screenshot("gbuffer_asdf.png");
}