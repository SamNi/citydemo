#ifdef _TEST_BUILD
#include "../Renderer/Backend/Backend_local.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>
#include "gtest_utils.h"

static const int TEST_WIDTH = 160;
static const int TEST_HEIGHT = 100;

struct texture_fixture : public ::testing::Test {
    virtual void SetUp(void) {
        glfwInit();
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        m_window = glfwCreateWindow(TEST_WIDTH, TEST_HEIGHT, "gtest", nullptr, nullptr);
        glfwSetWindowPos(m_window, 300, 300);
        glfwMakeContextCurrent(m_window);
        ASSERT_EQ(GLEW_OK, glewInit());
        Backend::startup(TEST_WIDTH, TEST_HEIGHT);
        Backend::show_hud(false);
        Backend::disable_depth_testing();
        Backend::set_modelview(glm::mat4(1.0f));
        Backend::set_projection(glm::mat4(1.0f));
    }

    virtual void TearDown(void) {
        Backend::shutdown();
    }
    GLFWwindow *m_window;
};

TEST_F(texture_fixture, default_texture_quad) {
    static const char *actual = "default_texture_quad_actual_result.png";
    static const char *expected = "default_texture_quad_expected_result.png";
    Texture t(16, 16);
    glBindVertexArray(QuadVAO::get_vao());
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    Backend::write_screenshot(actual);
    ASSERT_TRUE(image_match(expected, actual, TEST_WIDTH, TEST_HEIGHT));
}
#endif // ~_TEST_BUILD