#ifdef _TEST_BUILD
#include "../Renderer/Backend/Shader.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <physfs/physfs.h>
#include <gtest/gtest.h>

static const int TEST_WIDTH = 160;
static const int TEST_HEIGHT = 100;
static const auto NUM_BAD_PIXEL_THRESHOLD = 50;
static const double INDIVIDUAL_PIXEL_ERR_THRESHOLD = 1e-3;
static const int NUM_PIXELS = TEST_WIDTH*TEST_HEIGHT;
static const double TOTAL_ERR_THRESHOLD = 0.0001*NUM_PIXELS;

struct shader_fixture : public ::testing::Test {
    virtual void SetUp(void) {
        glfwInit();
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        window = glfwCreateWindow(TEST_WIDTH, TEST_HEIGHT, "gtest", nullptr, nullptr);
        glfwSetWindowPos(window, 300, 300);
        glfwMakeContextCurrent(window);
        ASSERT_EQ(GLEW_OK, glewInit());
        PHYSFS_init("gtest");
        PHYSFS_addToSearchPath("data", true);
    }
    virtual void TearDown() {
        ASSERT_NE(0, PHYSFS_deinit());
        glfwTerminate();
        window = nullptr;
    }
    GLFWwindow *window;
};

TEST_F(shader_fixture, load) {
    auto ret = ShaderManager::load("ortho2d", "shaders/ortho2d.frag", "shaders/ortho2d.vert");
    ASSERT_NE(-1, ret);
}

TEST_F(shader_fixture, load_nonexistent) {
    auto ret = ShaderManager::load("doesnotexist", "anditneverwill", "stop reading");
    ASSERT_EQ(-1, ret);
}
#endif  // ~_TEST_BUILD
