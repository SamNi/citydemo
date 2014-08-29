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
    }

    virtual void TearDown(void) {
        Backend::shutdown();
    }
    GLFWwindow *m_window;
};
TEST_F(texture_fixture, load_texture) {
    static const char *actual = "load_texture_actual_result.png";
    static const char *expected = "load_texture_expected_result.png";
    auto handle = Backend::load_texture("images/RGB1024.png");
    Backend::begin_frame();
    Backend::bind_texture(handle);
    Backend::draw_fullscreen_quad();
    Backend::end_frame();
    Backend::write_screenshot(actual);
    ASSERT_TRUE(image_match(expected, actual, TEST_WIDTH, TEST_HEIGHT));
}
TEST_F(texture_fixture, does_blend_work) {
    static const char *actual = "does_blend_work_actual_result.png";
    static const char *expected = "does_blend_work_expected_result.png";
    auto handle = Backend::load_texture("images/RGBA1024.png");
    Backend::set_clear_color(COLOR[YELLOW]);
    Backend::begin_frame();
    Backend::enable_blending();
    Backend::bind_texture(handle);
    Backend::draw_fullscreen_quad();
    Backend::end_frame();
    Backend::write_screenshot(actual);
    ASSERT_TRUE(image_match(expected, actual, TEST_WIDTH, TEST_HEIGHT));
}
#if 0
TEST_F(texture_fixture, default_texture_quad) {
    static const char *actual = "default_texture_quad_actual_result.png";
    static const char *expected = "default_texture_quad_expected_result.png";
    Texture t;
    Backend::begin_frame();
    t.bind();
    Backend::draw_fullscreen_quad();
    Backend::end_frame();
    Backend::write_screenshot(actual);
    ASSERT_TRUE(image_match(expected, actual, TEST_WIDTH, TEST_HEIGHT));
}

#endif