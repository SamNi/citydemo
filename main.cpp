#include "citydemo.h"
#include "GL.H"
#include "Shader.h"
#include "Texture.h"
#include <GLFW/glfw3.h>

#pragma warning ( disable : 4100 )

const char *fragShader = "fragment.glsl";
const char *vertShader = "vertex.glsl";

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

static void checkGL(void) {
    GLuint err = glGetError();
    if (GL_NO_ERROR!=err) {
        fprintf(stderr, "%s\n", gluErrorString(err));
        exit(EXIT_FAILURE);
    }
}

struct Thing {
    glm::vec3 center;

    Thing() {
        load();
    }
    void draw() const {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, nTris);
    }

private:
    GLuint verts_vbo, vao;
    int nTris;
    void load() {
        nTris = 1024;
        GLfloat *vtxData = new GLfloat[nTris*3];
        for(int i = 0;i < nTris*3;++i)
            vtxData[i] = uniform(-1.0f,1.0f);

        glGenBuffers(1, &verts_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, verts_vbo);
        glBufferData(GL_ARRAY_BUFFER, 3*nTris, vtxData, GL_STATIC_DRAW);
        
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, verts_vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);

        checkGL();

        delete[] vtxData;
    }
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
        window = glfwCreateWindow(800, 800, argv[0], NULL, NULL);
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
    ShaderManager shMan;
    
    shMan.load(std::string("default"), fragShader, vertShader);

    checkGL();
    fprintf(stdout, "%s\n%s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));


    glClearColor(.2,.2,.2,1);

    glm::mat4x4 modelView = glm::lookAt(glm::vec3(0,3,-7), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4x4 projection = glm::perspective(glm::pi<float>()/3.0f, 1.0f, 0.01f, 500.0f);
   

    Texture t(32, 256);
    t.Use();
    shMan.use("default");

    GLuint location = shMan.getProgID("default");

    glUniformMatrix4fv(glGetUniformLocation(location, "modelView"), 1, GL_FALSE, glm::value_ptr(modelView));
    glUniformMatrix4fv(glGetUniformLocation(location, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(location, "texture"), t.getTexID());
    glUniform1i(glGetUniformLocation(location, "Diffuse"), 0);
    float angle = 0;
    Thing th;
    while(!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        th.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();

        glUniformMatrix4fv(glGetUniformLocation(location, "modelView"), 1, GL_FALSE, glm::value_ptr(glm::rotate(modelView, angle, glm::vec3(0,1,0))));
        angle += 0.009f;
    }
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}