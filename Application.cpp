#include "./citydemo.h"
#include "./LuaBindings.h"
#include "Frontend.h"


int screen_width =              1600;
int screen_height =             900;
int screen_pos_x =              100;
int screen_pos_y =              50;
float aspect_ratio =            static_cast<float>(screen_width/screen_height);
float fov =                     60.0f;
const char *appName =           nullptr;  // set by command line
GLFWwindow *window =            nullptr;

double target_fps =             60.0;
double target_period =          1.0/target_fps;
bool vsync =                    true;


static void size_callback(GLFWwindow *, int, int);
static void error_callback(int, const char *);
static void key_callback(GLFWwindow *, int, int, int, int);

Application::Application(int argc, char *argv[], const char *title) {
    // parse command-line args, set globals accordingly
    appName = argv[0];
}

int Application::Run(void) {
    if (!Startup())
        return EXIT_FAILURE;

    static double t0, t1, tdelta, remaining;
    static bool goSlow;
    while (!Done()) {
        t0 = glfwGetTime();

        Update(target_period);
        Render();

        t1 = glfwGetTime();
        tdelta = t1 - t0;
        remaining = target_period - tdelta;
        if (tdelta > target_period) {
            LOG(LOG_WARNING, "frame behind schedule by %.2lf (%.2lfms elapsed)", 1000.0*(-remaining), 1000.0*tdelta);
            glfwPollEvents();
            glfwSwapInterval(0); // screen tearing is a lesser evil than dropped frames
            glfwSwapBuffers(window);
            glfwSwapInterval(vsync ? 1 : 0);
        } else {
            glfwPollEvents();
            glfwSwapBuffers(window);
        }
#if 0
// actually, maybe glfwSwapbuffers() does this for me, but I don't know.
// investigation says it relies on SwapBuffers() on win32, which I don't know 
// if it sleeps or not. Run-time profiling strongly suggests that it does,
// but leaving this here for now
        else {
            // experimentally adjust this
            static double sleepScaleFactor =       0.8;
            // sleep off most of the rest of the time we have left and
            // give some precious, but unneeded cycles to the rest of
            // the program
            uint32_t sleepOffTime = (uint32_t)(1000.0*sleepScaleFactor*remaining);
            LOG(LOG_TRACE, "%u", sleepOffTime);
            //Sleep(sleepOffTime); 
        }
#endif
    }

    Shutdown();
    return EXIT_SUCCESS;
}

bool Application::Startup(void) {
    // various subsystems (be careful with order, dependencies)
    // GLFW boilerplate
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        LOG(LOG_CRITICAL, "glfwInit failed\n");
        return false;
    }

    window = glfwCreateWindow(screen_width, screen_height,
        appName, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return false;
    }
    glfwSetWindowPos(window, screen_pos_x, screen_pos_y);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowSizeCallback(window, size_callback);

    // GLEW boilerplate
    glewExperimental = GL_TRUE;
    if (GLEW_OK != glewInit()) {
        LOG(LOG_CRITICAL, "glewInit failed\n");
        return false;
    }
    glfwSwapInterval(vsync ? 1 : 0);

    if (!Frontend::Startup(screen_width, screen_height)) {
        LOG(LOG_CRITICAL, "Frontend::Startup\n");
        return false;
    }
    if (!Lua::Startup()) {
        LOG(LOG_CRITICAL, "Lua::Startup\n");
        return false;
    }

    if (!Manager::Startup()) {
        LOG(LOG_CRITICAL, "Manager::Startup\n");
        return false;
    }

    return true;
}
bool Application::Done(void) { return static_cast<bool>(glfwWindowShouldClose(window)); }
void Application::Update(uint32_t delta_ms) {
    // Gather input

    // Update world state
}
void Application::Render(void) {
    Frontend::Render();
}
void Application::Shutdown(void) {
    ShaderManager::Shutdown();
    Frontend::Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
}

// GLFW callbacks
static void size_callback(GLFWwindow *window, int w, int h) {
    Backend::Resize(w, h);
    screen_width = w;
    screen_height = h;
    aspect_ratio = static_cast<float>(w)/h;
}
static void error_callback(int err, const char *descr) {
    LOG(LOG_CRITICAL, "glfw says %s with code %d", descr, err);
}
static void key_callback(GLFWwindow *window, int key, int scancode,
                         int action, int mods) {
    if (action != GLFW_PRESS)
        return;

    switch (key) {
    case GLFW_KEY_F12:
        Backend::Screenshot();
        break;
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GL_TRUE);
        break;
    }
}