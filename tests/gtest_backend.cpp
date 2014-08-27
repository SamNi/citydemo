#include "../Renderer/Backend/Backend.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>
#include "gtest_utils.h"

static const int TEST_WIDTH = 160;
static const int TEST_HEIGHT = 100;
static const auto NUM_BAD_PIXEL_THRESHOLD = 50;
static const double INDIVIDUAL_PIXEL_ERR_THRESHOLD = 1e-3;
static const int NUM_PIXELS = TEST_WIDTH*TEST_HEIGHT;
static const double TOTAL_ERR_THRESHOLD = 0.0001*NUM_PIXELS;

inline double _my_max(double lhs, double rhs);
inline double _my_abs(double t);
inline double pixel_diff(const RGBPixel& lhs, const RGBPixel& rhs);
bool color_match(RGBPixel *img, float r, float g, float b);

struct backend_fixture : public ::testing::Test {
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
    virtual void TearDown() {
        Backend::shutdown();
        glfwTerminate();
        window = nullptr;
    }
    GLFWwindow *window;
};
TEST_F(backend_fixture, black_screen_color_match) {
    static const auto actual = "black_screen_color_match_actual.png";
    static const auto expected = "black_screen_color_match_expected.png";
    Backend::set_clear_color(COLOR[BLACK]);
    Backend::begin_frame();
    Backend::end_frame();
    Backend::write_screenshot(actual);
    ASSERT_TRUE( image_match(actual, expected, TEST_WIDTH, TEST_HEIGHT ) );
}

TEST_F(backend_fixture, white_screen_color_match) {
    static const auto actual = "white_screen_color_match_actual.png";
    static const auto expected = "white_screen_color_match_expected.png";
    Backend::set_clear_color(COLOR[WHITE]);
    Backend::begin_frame();
    Backend::end_frame();
    Backend::write_screenshot(actual);
    ASSERT_TRUE( image_match(actual, expected, TEST_WIDTH, TEST_HEIGHT ) );
}

TEST_F(backend_fixture, empty_scene_triangle_count) {
    Backend::begin_frame();
    Backend::end_frame();
    ASSERT_EQ(0, Backend::get_performance_count().n_triangles_drawn);
}

TEST_F(backend_fixture, single_front_facing_triangle) {
    static const char *actual = "single_front_facing_triangle_actual_result.png";
    static const char *expected = "single_front_facing_triangle_expected_result.png";

    Backend::set_clear_color(COLOR[BLACK]);
    auto st = std::shared_ptr<SurfaceTriangles>(new SurfaceTriangles(3, 3));
    st->vertices[0] = glm::vec3(-1.0f, -1.0f, 0.0f);
    st->vertices[1] = glm::vec3(+1.0f, -1.0f, 0.0f);
    st->vertices[2] = glm::vec3(+1.0f, +1.0f, 0.0f);
    st->indices[0] = 0;
    st->indices[1] = 1;
    st->indices[2] = 2;
    st->colors[0] = COLOR_ALPHA[RED];
    st->colors[1] = COLOR_ALPHA[GREEN];
    st->colors[2] = COLOR_ALPHA[BLUE];
    st->texture_coordinates[0] = TexCoord(0, 0);
    st->texture_coordinates[1] = TexCoord(65535, 0);
    st->texture_coordinates[2] = TexCoord(65535, 65535);
    auto handle = Backend::add_surface_triangles(st);
    Backend::begin_frame();
    Backend::draw_surface_triangles(handle);
    Backend::end_frame();
    Backend::write_screenshot(actual);
    ASSERT_TRUE(image_match(actual, expected, TEST_WIDTH, TEST_HEIGHT));
}