#include "./citydemo.h"
#include "./LuaBindings.h"

int screen_width =              1600;
int screen_height =             900;
int screen_pos_x =              100;
int screen_pos_y =              50;
int offscreen_width =           256;
int offscreen_height =          256;
float aspect_ratio =            static_cast<float>(screen_width/screen_height);
float fov =                     60.0f;
const char *appName =           nullptr;  // set by command line
GLFWwindow *window =            nullptr;

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

    while (!Done()) {
        Update();
        Render();
        glfwSwapBuffers(window);
        glfwPollEvents();
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
    }

    if (!Backend::Startup()) {
        LOG(LOG_CRITICAL, "BackEnd::Startup\n");
        exit(EXIT_FAILURE);
    }

    if (!Lua::Startup()) {
        LOG(LOG_CRITICAL, "Lua::Startup\n");
        exit(EXIT_FAILURE);
    }

    if (!Manager::Startup()) {
        LOG(LOG_CRITICAL, "Manager::Startup\n");
        exit(EXIT_FAILURE);
    }
    return true;
}
bool Application::Done(void) { return static_cast<bool>(glfwWindowShouldClose(window)); }
void Application::Update(void) {
    // Gather input

    // Update world state
}
void Application::Render(void) {
    Backend::BeginFrame();

    /// Queue up things for the renderer to do
    // ...

    Backend::EndFrame(); // With this call, the renderer sets off to do its thing
}
void Application::Shutdown(void) {
    ShaderManager::Shutdown();
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
static void error_callback(int err, const char *descr) { fprintf(stderr, descr); }
static void key_callback(GLFWwindow *window, int key, int scancode,
                         int action, int mods) {
    if (action != GLFW_PRESS)
        return;

    switch (key) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GL_TRUE);
        break;
    }
}