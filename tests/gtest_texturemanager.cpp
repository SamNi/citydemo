#ifdef _TEST_BUILD
#include "../Renderer/Backend/Backend.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>
static const int TEST_WIDTH = 160;
static const int TEST_HEIGHT = 100;
struct texturemanager_fixture : public ::testing::Test {
    virtual void SetUp(void) {
        glfwInit();
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        m_window = glfwCreateWindow(TEST_WIDTH, TEST_HEIGHT, "gtest", nullptr, nullptr);
        glfwSetWindowPos(m_window, 300, 300);
        glfwMakeContextCurrent(m_window);
        ASSERT_EQ(GLEW_OK, glewInit());
        Backend::startup(TEST_WIDTH, TEST_HEIGHT);
        Backend::show_hud(false);
    }

    virtual void TearDown(void) {
        Backend::shutdown();
    }
    GLFWwindow *m_window;
};

TEST_F(texturemanager_fixture, foo) {

}
#endif // ~_TEST_BUILD