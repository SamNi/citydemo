#include "./citydemo.h"
#include "./LuaBindings.h"
#include "Renderer/Frontend/Frontend.h"
#include "Input.h"

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

// defined in essentials.cpp
void APIENTRY debugproc(GLenum source, GLenum type, GLuint id, GLenum severity,
               GLsizei length, const GLchar *incoming, void *userParam);

Application::Application(int argc, char *argv[], const char *title) {
    // parse command-line args, set globals accordingly
    appName = argv[0];
    argc; argv; title; // shut up
}

int Application::Run(void) {
    if (!startup())
        return EXIT_FAILURE;

    while (!Done()) {
        static double t0, t1, tdelta, remaining;
        t0 = glfwGetTime();

        Update(target_period);
        render();

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

    }

    shutdown();
    return EXIT_SUCCESS;
}

bool Application::startup(void) {
    srand(time(NULL));
    // various subsystems (be careful with order, dependencies)
    // GLFW boilerplate
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        LOG(LOG_CRITICAL, "glfwInit failed\n");
        return false;
    }

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
    window = glfwCreateWindow(screen_width, screen_height,
        appName, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return false;
    }
    glfwSetWindowPos(window, screen_pos_x, screen_pos_y);
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeCallback(window, size_callback);
    Input::startup(window);

    // GLEW boilerplate
    glewExperimental = GL_TRUE;
    if (GLEW_OK != glewInit()) {
        LOG(LOG_CRITICAL, "glewInit failed\n");
        return false;
    }
    glfwSwapInterval(vsync ? 1 : 0);

#ifdef _DEBUG
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    glDebugMessageCallback((GLDEBUGPROC)debugproc, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

    if (!Frontend::startup(screen_width, screen_height)) {
        LOG(LOG_CRITICAL, "Frontend::Startup\n");
        return false;
    }
    return true;
}
bool Application::Done(void) { return static_cast<bool>(glfwWindowShouldClose(window)); }

void Application::Update(uint32_t delta_ms) {
    // Gather input

    // Update world state
}

void Application::render(void) {
    static uint32_t numFrames = 0;
    static double t0 = 0.0, t1 = 0.0;

    t1 = glfwGetTime();
    if ((t1 - t0) >= 1.0) {
        // one second has passed, how many frames in this one second?
        LOG(LOG_TRACE, "%u frames per second at %d x %d", numFrames, screen_width, screen_height);
        numFrames = 0;
        t0 = t1;
    }
    numFrames++;
    Frontend::render();
}
void Application::shutdown(void) {
    ShaderManager::shutdown();
    Frontend::shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
}

// GLFW callbacks
static void error_callback(int err, const char *descr) {
    LOG(LOG_CRITICAL, "glfw says %s with code %d", descr, err);
}
static void size_callback(GLFWwindow *window, int w, int h) {
    Backend::resize(w, h);
    screen_width = w;
    screen_height = h;
    aspect_ratio = static_cast<float>(w)/h;
}