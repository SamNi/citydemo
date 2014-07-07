#ifndef _SHADER_H_
#define _SHADER_H_
#include "essentials.h"
#include "GL.H"

struct ShaderProgram {
    ShaderProgram(std::string frag, std::string vertex);
    ~ShaderProgram(void);
    void Use(void) const;
    GLuint getProgID(void) const;

private:
    GLuint loadShader(std::string path, GLuint shaderType);

    GLuint programID;
    GLuint vertexShaderID;
    GLuint fragmentShaderID;
    std::string name;
};

#endif // ~_SHADER_H_