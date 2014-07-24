// Copyright 2014 SamNi PlaceholderLicenseText
#include "citydemo.h"

#pragma warning ( disable : 4100 4800 )

#define WIDTH               (1600)
#define HEIGHT              (900)
#define FOV                 (60)
#define NUM_TRIANGLES       (3500)

static void DrawQuad(void) {
    static bool firstTime = true;
    // these are all in UpLeft, DownLeft, DownRight, UpRight order
    static const GLfloat points[] = {
        // ccw order
        -1.0f,+1.0f, 0.0f,
        -1.0f,-1.0f, 0.0f,
        +1.0f,-1.0f, 0.0f,
        +1.0f,+1.0f, 0.0f,
    };
    static const GLfloat colors[] = {
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
    };
    static const GLfloat texCoords[] = {
        0.0f,0.0f, 
        0.0f,1.0f,  
        1.0f,1.0f,
        1.0f,0.0f, 
    };
    static const GLfloat normals[] = {
        -1.0f, +1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        +1.0f, -1.0f, -1.0f,
        +1.0f, +1.0f, -1.0f,
    };
    static GLuint vbo_position, vbo_colors, vbo_texCoords, vbo_normal, vao;
    if (firstTime) {
        glGenBuffers(1, &vbo_position);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
        glBufferData(GL_ARRAY_BUFFER, 3*4*sizeof(GLfloat), points, GL_STATIC_DRAW);
        checkGL();

        glGenBuffers(1, &vbo_colors);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
        glBufferData(GL_ARRAY_BUFFER, 4*4*sizeof(GLfloat), colors, GL_STATIC_DRAW);
        checkGL();

        glGenBuffers(1, &vbo_texCoords);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_texCoords);
        glBufferData(GL_ARRAY_BUFFER, 2*4*sizeof(GLfloat), texCoords, GL_STATIC_DRAW);
        checkGL();

        glGenBuffers(1, &vbo_normal);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_normal);
        glBufferData(GL_ARRAY_BUFFER, 3*4*sizeof(GLfloat), normals, GL_STATIC_DRAW);
        checkGL();

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_texCoords);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_normal);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
        checkGL();
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        checkGL();

        firstTime = false;
    }
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
int main(int argc, char *argv[]) {
    int i;

    BackEnd bk;

    if (!bk.Startup()) {
        fprintf(stderr, "BackEnd::Startup\n");
        exit(EXIT_FAILURE);
    }
    /*
    ShaderManager shMan;
    shMan.load();
    shMan.use("standard");
    checkGL();

    const int nTextures = 500;
    std::list<Texture*> textures;
    for (i = 0;i < nTextures;++i) {
        static char fname[64];
        sprintf(fname, "img\\img%05d.png", i+1);
        textures.push_back( new Texture(fname) );
    }

    checkGL();

    GLuint location = shMan.getProgID("standard");

    ParticleSystem ps;
    ps.Init(NUM_TRIANGLES*3);

    // compute shader boilerplate
    GLuint progHandle, cs;
    GLint result;
    const char *shadSrc;
    const char *fname = "example.compute";
    /*
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

    std::list<Texture *>::const_iterator it = textures.begin();
    checkGL();*/
    while(!bk.Done()) {
        //vg.Draw();
        //glUseProgram(shMan.getProgID("white"));
        //ps.Draw();
        //ps.Step();
        /*
        glUseProgram(shMan.getProgID("standard"));
        modelView = glm::lookAt(glm::vec3(3,3,3), glm::vec3(0,0,0), glm::vec3(0,1,0))*glm::rotate(rotAngle, glm::vec3(0,1,0));
        rotAngle += 0.025f;
        */
        bk.BeginFrame();
        /*
        glClear(GL_COLOR_BUFFER_BIT);
        (*it)->Bind();
        it++;
        if (it == textures.end())
            it = textures.begin();

        glUniform1f(glGetUniformLocation(location, "seed"), (seed+=1));
        glUniformMatrix4fv(glGetUniformLocation(location, "modelView"), 1, GL_FALSE, glm::value_ptr(modelView));
        glUniform3f(glGetUniformLocation(location, "lightPos"), 100.0f, 1.0f,-1.0f);
        DrawQuad();
        modelView *= glm::rotate(rotAngle, glm::vec3(1.0f,0.0f,0.0f));
        glUniformMatrix4fv(glGetUniformLocation(location, "modelView"), 1, GL_FALSE, glm::value_ptr(modelView));
        DrawQuad();
        */
        bk.EndFrame();
    }
    bk.Shutdown();

    return EXIT_SUCCESS;
}