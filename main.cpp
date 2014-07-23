#include "citydemo.h"
#include "GL.H"
#include "GLM.h"
#include "Shader.h"
#include "Texture.h"
#include "ParticleSystem.h"
#include "Contouring.h"



#include <GLFW/glfw3.h>

#include <physfs/physfs.h>

#pragma warning ( disable : 4100 4800 )

#define WIDTH               (1600)
#define HEIGHT              (900)
#define FOV                 (60)
#define NUM_TRIANGLES       (3500)

static void error_callback(int err, const char *descr);
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
static void size_callback(GLFWwindow *window, int width, int height);

struct GPU {
    void Init(void) {
        width = WIDTH;
        height = HEIGHT;
        aspect = (float)width/height;
        fov = FOV;
        fullscreen = false;

        GLenum err;

        // GLFW boilerplate
        glfwSetErrorCallback(error_callback);
        if (!glfwInit())
            exit(EXIT_FAILURE);
        // not sure how to enforce 4.x compliance
        //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        //window = glfwCreateWindow(width, height, "citydemo", glfwGetPrimaryMonitor(), nullptr);
        window = glfwCreateWindow(width, height, "citydemo", nullptr, nullptr);
        if(!window) {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        glfwSetWindowPos(window, 100, 50);
        glfwMakeContextCurrent(window);
        glfwSetKeyCallback(window, key_callback);
        glfwSetWindowSizeCallback(window, size_callback);
    
        // glew boilerplate
        glewExperimental = GL_TRUE;
        err = glewInit();
        if(GLEW_OK!=err) {
            fprintf(stderr, "%s\n", glewGetErrorString(err));
            exit(EXIT_FAILURE);
        }

        fprintf(stdout, "%s\n%s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        //glClearColor(1.0f,1.0f,1.0f,1.0f);
        //glEnable(GL_DEPTH_TEST);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_MULTISAMPLE);
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_POLYGON_SMOOTH);

        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_units);
        printf("%d texture image units\n", max_texture_units);
    }

    void Quit(void) {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void BeginFrame(void) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void EndFrame(void) {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    bool Done(void) {
        return (bool)glfwWindowShouldClose(window);
    }

    void Resize(int w, int h) {
        glViewport(0,0, w, h);
        width = w;
        height = h;
    }

    int width, height;
    float fov, aspect;
    bool fullscreen;

    GLFWwindow *window;

    std::string gl_renderer;
    std::string gl_version;
    int max_texture_units;
};

GPU theGPU;

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
    theGPU.Resize(width, height);
}

int main(int argc, char *argv[]) {
    int i;

    theGPU.Init();

    GLuint vbo_position, vbo_texCoords, vao;
    {
        static GLfloat points[] = {
            // ccw order
            -1.0f,+1.0f, 0.0f, // upper left
            -1.0f,-1.0f, 0.0f, // lower left
            +1.0f,-1.0f, 0.0f, // lower right
            +1.0f,+1.0f, 0.0f, // upper right
            -1.0f, 0.0f, 1.0f,
            +1.0f, 0.0f, 1.0f,
            +1.0f, 0.0f,-1.0f,
            -1.0f, 0.0f,-1.0f,
        };
        static GLfloat texCoords[] = {
            0,0,        // upper left
            0,1,        // lower left
            1,1,        // lower right
            1,0,        // upper right
            0,0,        // upper left
            0,1,        // lower left
            1,1,        // lower right
            1,0,        // upper right
        };
        glGenBuffers(1, &vbo_position);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
        glBufferData(GL_ARRAY_BUFFER, 3*8*sizeof(GLfloat), points, GL_STATIC_DRAW);

        glGenBuffers(1, &vbo_texCoords);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_texCoords);
        glBufferData(GL_ARRAY_BUFFER, 2*8*sizeof(GLfloat), texCoords, GL_STATIC_DRAW);

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

    /*
    const int nTextures = 280;
    std::list<Texture*> textures;
    for (i = 0;i < nTextures;++i) {
        static char fname[64];
        sprintf(fname, "img\\img%05d.png", i+1);
        textures.push_back( new Texture(fname) );
    }*/
    std::list<Texture*> textures;
    const int nTextures = 1;
    //textures.push_back(new Texture("img\\img00028.png"));
    textures.push_back(new Texture());

    checkGL();

    GLuint location = shMan.getProgID("textured");

    //ParticleSystem ps;
    //ps.Init(NUM_TRIANGLES*3);

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
    glm::mat4x4 projection = glm::perspective(glm::radians((float)FOV), (float)WIDTH/HEIGHT, 0.01f, 300.0f);
   
    glUniformMatrix4fv(glGetUniformLocation(location, "modelView"), 1, GL_FALSE, glm::value_ptr(modelView));
    glUniformMatrix4fv(glGetUniformLocation(location, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    static float seed, rotAngle = 0.0f;

    //VoxGrid vg(40,40,40);
    //vg.Clear();

    std::list<Texture*>::const_iterator it;
    for (it = textures.begin();it != textures.end();++it)
        printf("%s %d\n", (*it)->getName(), (*it)->getSizeInBytes());

    it = textures.begin();
    while(!theGPU.Done()) {
        theGPU.BeginFrame();
        //vg.Draw();
        //ps.Draw();
        //ps.Step();

        modelView = glm::lookAt(glm::vec3(3,3,3), glm::vec3(0,0,0), glm::vec3(0,1,0))*glm::rotate(rotAngle, glm::vec3(0,1,0));
        rotAngle += 0.025f;
        glUseProgram(location);
        glUniform1f(glGetUniformLocation(location, "seed"), (seed+=1));
        glUniformMatrix4fv(glGetUniformLocation(location, "modelView"), 1, GL_FALSE, glm::value_ptr(modelView));
        (*it)->Bind();
        it++;
        if (it == textures.end())
            it = textures.begin();
        glBindVertexArray(vao);
        glDrawArrays(GL_QUADS, 0, 8);

        theGPU.EndFrame();
    }
    for (it = textures.begin();it != textures.end();++it) {
        delete (*it);
    }
    theGPU.Quit();

    return EXIT_SUCCESS;
}