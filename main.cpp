#include "citydemo.h"
#include "GL.H"
#include "Shader.h"
#include "Texture.h"
#include "ParticleSystem.h"


#include <GLFW/glfw3.h>

#pragma warning ( disable : 4100 )

#define WIDTH               (1600)
#define HEIGHT              (900)
#define FOV                 (60)
#define NUM_TRIANGLES       (50)

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

struct MyType {
    int value;
    MyType(int x) : value(x) { }
    ~MyType(void) { printf("destroyed\n"); }
};

int main(int argc, char *argv[]) {
    GLFWwindow *window;
    GLenum err;

    // GLFW boilerplate
    {
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
            exit(EXIT_FAILURE);
        // not sure how to enforce 4.x compliance
        //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        window = glfwCreateWindow(WIDTH, HEIGHT, argv[0], NULL, NULL);
        if(!window) {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        glfwSetWindowPos(window, 100, 50);
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
    fprintf(stdout, "%s\n%s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

    glClearColor(.2f,.2f,.2f,1.0f);
    glEnable(GL_DEPTH_TEST);
    //glDisable(GL_DEPTH_TEST);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLint numUnits;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &numUnits);
    printf("%d texture image units\n", numUnits);

    GLuint vbo_position, vbo_texCoords, vao;
    {
        static GLfloat points[] = {
            // ccw order
            -1.0f,+1.0f,0.0f, // upper left
            -1.0f,-1.0f,0.0f, // lower left
            +1.0f,-1.0f,0.0f, // lower right
            +1.0f,+1.0f,0.0f, // upper right
        };
        static GLfloat texCoords[] = {
            0,0,        // upper left
            0,1,        // lower left
            1,1,        // lower right
            1,0,        // upper right
        };
        glGenBuffers(1, &vbo_position);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
        glBufferData(GL_ARRAY_BUFFER, 3*4*sizeof(GLfloat), points, GL_STATIC_DRAW);

        glGenBuffers(1, &vbo_texCoords);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_texCoords);
        glBufferData(GL_ARRAY_BUFFER, 2*4*sizeof(GLfloat), texCoords, GL_STATIC_DRAW);

        checkGL();
    }
    {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_position);          // refer to the above VBO definition
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_texCoords);          // refer to the above VBO definition
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        checkGL();
    }

    ShaderManager shMan;
    shMan.load();
    shMan.use("textured");
    checkGL();

    Texture textures[] = {
        Texture("kei.png"),
        Texture("gradient.png"),
        Texture("oreimo.png"),
        Texture("doesnotexist.png"),
        Texture("sarenna.png"),
    };
    const int nTextures = sizeof(textures)/sizeof(Texture);
    checkGL();

    GLuint location = shMan.getProgID("textured");

    ParticleSystem ps;
    ps.Init(NUM_TRIANGLES*3);

    // compute shader boilerplate
    GLuint progHandle, cs;
    GLint result;
    const char *shadSrc;
    const char *fname = "example.compute";

    progHandle = glCreateProgram();
    cs = glCreateShader(GL_COMPUTE_SHADER);
    shadSrc = readFile(fname);
    glShaderSource(cs, 1, &shadSrc, NULL);
    glCompileShader(cs);
    glGetShaderiv(cs, GL_COMPILE_STATUS, &result);
    checkGL();

    if (!result) {
        static char tmp[GL_INFO_LOG_LENGTH];
        GLsizei len;

        glGetShaderInfoLog(cs, GL_INFO_LOG_LENGTH-1, &len, tmp);
        tmp[len]='\0';
        fprintf(stderr, "%s\n%s\n", fname, tmp);
        exit(EXIT_FAILURE);
    }

    glAttachShader(progHandle, cs);
    glLinkProgram(progHandle);
    glGetProgramiv(progHandle, GL_LINK_STATUS, &result);

    if (!result) {
        static char tmp[GL_INFO_LOG_LENGTH];
        GLsizei len;

        glGetProgramInfoLog(progHandle, GL_INFO_LOG_LENGTH-1, &len, tmp);
        tmp[len]='\0';
        fprintf(stderr, "%s\n%s\n", fname, tmp);
        exit(EXIT_FAILURE);
    }
    delete[] shadSrc;
    checkGL();

    
    glUseProgram(progHandle);
    // updateTex
    glDispatchCompute(512/32, 512/32, 1);
    checkGL();

    glUseProgram(location);

    glm::mat4x4 modelView;
    //glm::mat4x4 projection = glm::ortho(-1.0f,1.0f,-1.0f,1.0f,0.01f,10.0f);
    glm::mat4x4 projection = glm::perspective(glm::radians((float)FOV), (float)WIDTH/HEIGHT, 0.01f, 100.0f);
   
    glUniformMatrix4fv(glGetUniformLocation(location, "modelView"), 1, GL_FALSE, glm::value_ptr(modelView));
    glUniformMatrix4fv(glGetUniformLocation(location, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    static float seed, rotAngle = 0.0f;
    while(!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        modelView = glm::lookAt(glm::vec3(0,1.5,2), glm::vec3(0,0,0), glm::vec3(0,1,0))*glm::rotate(rotAngle, glm::vec3(0,1,0));

        glUseProgram(location);
        glUniform1f(glGetUniformLocation(location, "seed"), (seed+=1));
        glUniformMatrix4fv(glGetUniformLocation(location, "modelView"), 1, GL_FALSE, glm::value_ptr(modelView));

        rotAngle += 0.01f;

        // shake shake shake
        //glfwSetWindowPos(window, 75+rand()%100, 10+rand()%100);

        textures[int(rotAngle)%nTextures].Bind();
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        //checkGL();
        ps.Draw();
        //ps.Step();

        glfwSwapBuffers(window);
        glfwPollEvents();

    }
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}