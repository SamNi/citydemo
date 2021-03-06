#include "../Renderer/Backend/Backend_local.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>
#include "gtest_utils.h"

static const int TEST_WIDTH = 160;
static const int TEST_HEIGHT = 90;

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

static int x = 2;
TEST(asdf, asdf) {
    x = 3;
}
TEST(asdf, asdf2) {
    ASSERT_EQ(3, x);
}

TEST_F(framebuffer_fixture, crude_downscale) {
    static const auto actual = "crude_downscale_actual.png";
    static const auto expected = "crude_downscale_expected.png";

    auto tex_handle = Backend::load_texture("images/RGB1024.png");
    Backend::enable_downscale();
    Backend::set_clear_color(COLOR[GREEN]);
    Backend::begin_frame();
    TextureManager::bind(tex_handle);
    Backend::set_modelview( glm::rotate(glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f) ) );
    Backend::draw_fullscreen_quad();
    Backend::end_frame();
    Backend::write_screenshot(actual);
    ASSERT_TRUE(image_match(expected, actual, TEST_WIDTH, TEST_HEIGHT));
}
