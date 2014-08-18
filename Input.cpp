#include <GLFW/glfw3.h>

#include "Input.h"


float movement_speed = 0.75f;
float mouse_sensitivity = 2.75f;

// cb: callback
static void cb_key(GLFWwindow* window, int key, int scancode, int action, int mods);
static void cb_unicode_char(GLFWwindow *window, unsigned int codepoint);
static void cb_cursor_movement(GLFWwindow* window, double delta_x, double delta_y);
static void cb_cursor_enter(GLFWwindow* window, int entered);
static void cb_mouse_button(GLFWwindow* window, int button, int action, int mods);
static void cb_scroll(GLFWwindow *window, double x_offset, double y_offset);

std::unique_ptr<Input::Impl> Input::mImpl = nullptr;

struct Input::Impl {
    explicit Impl(void) : window(nullptr) { }
    void startup(void *window_ptr) {
        window = (GLFWwindow*)window_ptr;

        glfwSetKeyCallback(window, cb_key);
        glfwSetCharCallback(window, cb_unicode_char);
        glfwSetCursorPosCallback(window, cb_cursor_movement);
        glfwSetCursorEnterCallback(window, cb_cursor_enter);
        glfwSetMouseButtonCallback(window, cb_mouse_button);
        glfwSetScrollCallback(window, cb_scroll);
        //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    void shutdown(void) {
        window = nullptr;
    }
    bool get_key_state(int key) {
        return glfwGetKey(window, key) == GLFW_PRESS;
    }
    void get_mouse_state(void) {
    }
    GLFWwindow* window;
};

// TODO(SamNi): need to cut the dependence on Frontend and Backend::
// The Application module should be calling these.
#include "Frontend.h"
#include "Backend.h"

// GLFW callbacks
static void cb_key(GLFWwindow *window, int key, int scancode,
                         int action, int mods) {
    if (action != GLFW_PRESS)
        return;

    switch (key) {
    case GLFW_KEY_F12:
        Backend::screenshot();
        break;
    case GLFW_KEY_W:
        Frontend::strafe(-movement_speed*glm::vec3(0.0f, 0.0f,-1.0f));
        break;
    case GLFW_KEY_A:
        Frontend::strafe(-movement_speed*glm::vec3(1.0f, 0.0f, 0.0f));
        break;
    case GLFW_KEY_S:
        Frontend::strafe(-movement_speed*glm::vec3(0.0f, 0.0f, 1.0f));
        break;
    case GLFW_KEY_D:
        Frontend::strafe(-movement_speed*glm::vec3(-1.0f, 0.0f, 0.0f));
        break;
    case GLFW_KEY_Q:
        // roll
        // ...
        break;
    case GLFW_KEY_E:
        // roll other way
        break;
    case GLFW_KEY_SPACE:
        Frontend::strafe(-movement_speed*glm::vec3(0.0f,-1.0f, 0.0f));
        break;
    case GLFW_KEY_C:
        Frontend::strafe(-movement_speed*glm::vec3(0.0f, 1.0f, 0.0f));
        break;
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GL_TRUE);
        break;
    }
}
static void cb_unicode_char(GLFWwindow *window, unsigned int codepoint) {
    LOG(LOG_TRACE, "window %x received unicode char %u", window, codepoint);
}

static void cb_mouse_button(GLFWwindow *window, int button, int action, int mods) {
    LOG(LOG_TRACE, "Mouse buttons %x %s (%x) window %x",
        button,
        GLFW_PRESS ? "pressed" : "released",
        mods,
        window);
}
static void cb_scroll(GLFWwindow *window, double x_offset, double y_offset) {
    Frontend::change_fov(y_offset);
}
static void cb_cursor_movement(GLFWwindow *window, double delta_x, double delta_y) {
    // transform screen coordinates to the usual [-1..1], [1..1] that we're used to
    // positive X and Y go to the upper right
    static double old_dx, old_dy;
    static double _dx = 0.0f, _dy = 0.0f;
    static int screen_width, screen_height; // TODO(SamNi): artifact of the Frontend/Backend dependence

    glfwGetFramebufferSize(window, &screen_width, &screen_height);

    old_dx = _dx;
    old_dy = _dy;
    _dx =  2.0*mouse_sensitivity*( (delta_x / screen_width) - 0.5 );
    _dy = -2.0*mouse_sensitivity*( (delta_y / screen_height) - 0.5 );

    Frontend::mouselook(_dx - old_dx, _dy - old_dy);
}

static void cb_cursor_enter(GLFWwindow* window, int entered) {
    LOG(LOG_TRACE, "cursor %s window %x", entered ? "entered" : "left", window);
}

void Input::startup(void *window_ptr) { 
    if (nullptr != mImpl) {
        LOG(LOG_WARNING, "Redundant Input::startup() call");
        return;
    }
    mImpl = std::unique_ptr<Impl>(new Impl());
    mImpl->startup(window_ptr); 
}
void Input::shutdown(void) { 
    if (nullptr == mImpl) {
        LOG(LOG_WARNING, "Redundant Input::shutdown() call");
        return;
    }
    mImpl->shutdown();
    mImpl.reset(nullptr);
}
bool Input::get_key_state(int key) { return mImpl->get_key_state(key); }
void Input::get_mouse_state(void) { mImpl->get_mouse_state(); }
