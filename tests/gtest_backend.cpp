#include "../Renderer/Backend/Backend.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>

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
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    Backend::begin_frame();
    Backend::end_frame();
    auto p_img = Backend::get_screenshot();
    Backend::write_screenshot();
    ASSERT_FALSE(color_match(p_img, 0.0f, 0.0f, 0.0f));
    delete[] p_img;
}

TEST_F(backend_fixture, white_screen_color_match) {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    Backend::begin_frame();
    Backend::end_frame();
    auto p_img = Backend::get_screenshot();
    Backend::write_screenshot();
    ASSERT_FALSE(color_match(p_img, 1.0f, 1.0f, 1.0f));
    delete[] p_img;
}

TEST_F(backend_fixture, white_screen_color_mismatch) {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    Backend::begin_frame();
    Backend::end_frame();
    auto p_img = Backend::get_screenshot();
    Backend::write_screenshot();
    ASSERT_TRUE(color_match(p_img, 1.0f, 1.0f, 0.9999f));
    delete[] p_img;
}
TEST_F(backend_fixture, empty_scene_triangle_count) {
    Backend::begin_frame();
    Backend::end_frame();
    ASSERT_EQ(0, Backend::get_performance_count().n_triangles_drawn);
}

TEST_F(backend_fixture, single_front_facing_triangle) {
    Backend::set_clear_color(COLOR_ALPHA[DARK_GRAY]);
    Backend::begin_frame();
    Backend::end_frame();
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
    Backend::disable_depth_testing();
    Backend::begin_frame();
    Backend::draw_surface_triangles(handle);
    Backend::end_frame();
    Backend::write_screenshot("single_front_facing_triangle_actual_result.png");
}

// misc helpers
// trying to ensure the highest precision possible
inline double _my_max(double lhs, double rhs) { return (lhs > rhs) ? lhs: rhs; }
inline double _my_abs(double t) { return (t > 0) ? t : -t ; }
// ad-hoc difference metric
inline double pixel_diff(const RGBPixel& lhs, const RGBPixel& rhs) {
    static const double k = 1.0/255.0;
    double tmp[] = {
        k*_my_abs(lhs.x - rhs.x),
        k*_my_abs(lhs.y - rhs.y),
        k*_my_abs(lhs.z - rhs.z),
    };
    return _my_max(tmp[0], _my_max(tmp[1], tmp[2]));
}
// returns true if match
bool color_match(RGBPixel *img, float r, float g, float b) {
    const int n = TEST_WIDTH*TEST_HEIGHT;
    auto num_bad_pixels = 0L;
    double total_err = 0.0;
    bool broken_threshold = false;
    auto ref = RGBPixel(r*255, g*255, b*255);

    for (auto i = 0;i < n;++i) {
        auto err = pixel_diff(ref, img[i]);
        total_err += err;
        if (err >= INDIVIDUAL_PIXEL_ERR_THRESHOLD)
            ++num_bad_pixels;

        if ((num_bad_pixels >= NUM_BAD_PIXEL_THRESHOLD) || (total_err >= TOTAL_ERR_THRESHOLD))
            broken_threshold = true;
    }
    if (broken_threshold) {
        LOG(LOG_WARNING, "Threshold broken: %d bad pixels with %lf%% error", num_bad_pixels, total_err/NUM_PIXELS);
        Backend::write_screenshot();
    }
    LOG(LOG_INFORMATION, "%d bad pixels and %lf percent error", num_bad_pixels, total_err/NUM_PIXELS);
    return broken_threshold;
}

// must have same dimensions!!!
bool image_match(RGBPixel* lhs, RGBPixel* rhs) {
    const int n = TEST_WIDTH*TEST_HEIGHT;
    auto num_bad_pixels = 0L;
    double total_err = 0.0;
    bool broken_threshold = false;

    for (auto i = 0;i < n;++i) {
        auto err = pixel_diff(lhs[i], rhs[i]);
        total_err += err;
        if (err >= INDIVIDUAL_PIXEL_ERR_THRESHOLD)
            ++num_bad_pixels;

        if ((num_bad_pixels >= NUM_BAD_PIXEL_THRESHOLD) || (total_err >= TOTAL_ERR_THRESHOLD))
            broken_threshold = true;
    }
    if (broken_threshold) {
        LOG(LOG_WARNING, "Threshold broken: %d bad pixels with %lf%% error", num_bad_pixels, total_err/NUM_PIXELS);
        Backend::write_screenshot();
    }
    LOG(LOG_INFORMATION, "%d bad pixels and %lf percent error", num_bad_pixels, total_err/NUM_PIXELS);
    return broken_threshold;
}
