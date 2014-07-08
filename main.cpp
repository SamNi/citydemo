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

static GLuint setupVerts() {
    GLuint verts_vbo, colors_vbo, tex_vbo, vao;
    static GLfloat points[] = {
        -1,-1,      // lower left
        +1,-1,      // lower right
        +1,+1,      // upper right
        -1,+1       // upper left
    };
    static GLfloat colors[] = {
        1,0,0,      // red
        0,1,0,      // green
        0,0,1,      // blue
        0,1,1       // cyan
    };
    static GLfloat texcoords[] = {
        0, 0,       // lower left corner of texmap
        1, 0,       // lower right corner
        1, 1,       // yadda...
        0, 1
    };


    glGenBuffers(1, &verts_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, verts_vbo);
    glBufferData(GL_ARRAY_BUFFER, 8*sizeof(float), points, GL_STATIC_DRAW);

    glGenBuffers(1, &colors_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, colors_vbo);
    glBufferData(GL_ARRAY_BUFFER, 12*sizeof(float), colors, GL_STATIC_DRAW);

    glGenBuffers(1, &tex_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, tex_vbo);
    glBufferData(GL_ARRAY_BUFFER, 8*sizeof(float), texcoords, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, verts_vbo);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,NULL);

    glBindBuffer(GL_ARRAY_BUFFER, colors_vbo);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,0,NULL);

    glBindBuffer(GL_ARRAY_BUFFER, tex_vbo);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,0,NULL);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    return vao;
}

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
    ShaderProgram prog(fragShader, vertShader);
    checkGL();
    GLuint vao = setupVerts();
    fprintf(stdout, "%s\n%s\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));


    glClearColor(.2,.2,.2,1);

    glm::mat4x4 transMat;
    //transMat = glm::translate(transMat, glm::vec3(-.35,0,0));
    //transMat = glm::rotate(transMat, (float)PI/6, glm::vec3(0,0,1));
    //transMat = glm::scale(transMat, glm::vec3(.5f,.5f,.5f));

    //glm::mat4x4::
   

    Texture t(16, 8);
    t.Use();

    // framebuffer
    /*
    GLuint fbID;
    glGenFramebuffers(1, &fbID);
    glBindFramebuffer(GL_FRAMEBUFFER, fbID);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texID, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    */

    prog.Use();


    GLuint location = glGetUniformLocation(prog.getProgID(), "transMat");
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(transMat));
    glUniform1i(glGetUniformLocation(prog.getProgID(), "texture"), t.getTexID());
    glUniform1i(glGetUniformLocation(prog.getProgID(), "Diffuse"), 0);
    float angle = 0;
    while(!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(vao);
        glDrawArrays(GL_QUADS, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();

        //glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(glm::rotate(glm::mat4x4(), angle, glm::vec3(0,0,1))));
    }
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}