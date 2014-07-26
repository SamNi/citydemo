// Copyright [year] <Copyright Owner>
#ifndef _SHADER_H_
#define _SHADER_H_
#include "essentials.h"
#include "GL.H"

struct ShaderProgram;

struct ShaderManager {
    void load(std::string name, std::string frag, std::string vertex);
    void load(void);
    void use(std::string name);
    GLuint getProgID(const std::string& name);
    ~ShaderManager(void);

    std::map<std::string, ShaderProgram*> progs;
};

#endif // ~_SHADER_H_