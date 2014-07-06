#include "citydemo.h"
#include "GL.H"
#include <GLFW/glfw3.h>

#pragma warning ( disable : 4100 )

// forward decls
static char *readFile(char *fname);



// GLFW callbacks
static void error_callback(int err, const char *descr) {
    fprintf(stderr, descr);
}
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action!=GLFW_PRESS)
        return;

    switch(key) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GL_TRUE);
        break;
    }
}
static void size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0,0,width,height);
}
GLuint vbo = 0;
GLuint vao = 0;
GLuint p = 0;
static void setupShaders(const char *frag, const char *vert) {
    GLuint f, v;
    f = glCreateShader(GL_FRAGMENT_SHADER);
    v = glCreateShader(GL_VERTEX_SHADER);

    char *fragSrc = readFile("basic.frag");
    char *vertSrc = readFile("basic.vert");
    const char *ff = fragSrc;
    const char *vv = vertSrc;

    glShaderSource(f, 1, &ff, NULL);
    glShaderSource(v, 1, &vv, NULL);
    glCompileShader(f);
    glCompileShader(v);
    delete[] fragSrc;
    delete[] vertSrc;

    int params = -1;
    glGetShaderiv(f, GL_COMPILE_STATUS, &params);
    if(GL_TRUE!=params) {
        const int bufSize = 16384;
        static char tmp[bufSize];
        GLsizei len;

        glGetShaderInfoLog(f, bufSize-2, &len, tmp);
        fprintf(stderr,"fragment\n");
        fprintf(stderr, "%s\n", tmp);
        glGetShaderInfoLog(v, bufSize-2, &len, tmp);
        fprintf(stderr,"vertex\n");
        fprintf(stderr, "%s\n", tmp);
        exit(EXIT_FAILURE);
    }
        

    p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    assert(GL_NO_ERROR==glGetError());
    glUseProgram(p);
}

static void setupVerts() {
    static GLfloat points[] = {
        -0.4, -1,
        0.4, -1,
        0.4, 1,
        -0.4, 1
    };

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 8*sizeof(float), points, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,NULL);
}

static char *readFile(char *fname) {
    FILE *fin;
    size_t count;
    char *ret;

    fin = fopen(fname, "rb");
    fseek(fin, 0, SEEK_END);
    count = ftell(fin);
    rewind(fin);
    ret = new char[count+1];
    if(count != fread(ret, sizeof(char), count, fin)) {
        fprintf(stderr, "Error reading %s\n", fname);
        delete[] ret;
        return NULL;
    }
    ret[count]='\0';
    return ret;
}
int main(int argc, char *argv[]) {
    GLFWwindow *window;
    GLenum err;

    // GLFW boilerplate
    {
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
            exit(EXIT_FAILURE);
        window = glfwCreateWindow(1600, 900, argv[0], NULL, NULL);
        if(!window) {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        glfwMakeContextCurrent(window);
        glfwSetKeyCallback(window, key_callback);
        glfwSetWindowSizeCallback(window, size_callback);
    }
    
    // glew boilerplate
    glewExperimental = GL_TRUE;
    err = glewInit();
    if(GLEW_OK!=err) {
        fprintf(stderr, "%s\n", glewGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    // shader boilerplate
    setupShaders("basic.frag", "basic.vert");
    setupVerts();

    glClearColor(.2,.2,.2,1);

    while(!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glUseProgram(p);
        glBindVertexArray(vao);
        glDrawArrays(GL_QUADS, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}