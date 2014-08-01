// Copyright [year] <Copyright Owner>
#include "./citydemo.h"

int main(int argc, char *argv[]) {
    Application app(argc, argv, "citydemo");
    return app.Run();
    /*
    if (!Backend::Startup()) {
        fprintf(stderr, "BackEnd::Startup\n");
        exit(EXIT_FAILURE);
    }

    if (!Lua::Startup()) {
        fprintf(stderr, "Lua::Startup\n");
        exit(EXIT_FAILURE);
    }

    if (!Manager::Startup(argv[0])) {
        fprintf(stderr, "Manager::Startup\n");
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
/*
    while (!Backend::Done()) {
        // vg.Draw();
        // glUseProgram(shMan.getProgID("white"));
        // ps.Draw();
        // ps.Step();
        /*
        glUseProgram(shMan.getProgID("standard"));
        modelView = glm::lookAt(glm::vec3(3,3,3), glm::vec3(0,0,0), glm::vec3(0,1,0))*glm::rotate(rotAngle, glm::vec3(0,1,0));
        rotAngle += 0.025f;
        */
    /*
        Backend::BeginFrame();
        glBindTexture(GL_TEXTURE_2D, 1);
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
    /*
        Backend::EndFrame();
    }
    Manager::Shutdown();
    Lua::Shutdown();
    Backend::Shutdown();
    */
}