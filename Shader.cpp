#include "Shader.h"

static GLuint loadShader(std::string path, GLuint shaderType) {
    GLuint shaderHandle;
    const char *source;

    source = readFile(path.c_str());
    shaderHandle = glCreateShader(shaderType);
    glShaderSource(shaderHandle, 1, &source, NULL);
    glCompileShader(shaderHandle);
    delete[] source;

    int params = -1;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &params);
    if(GL_TRUE!=params) {
        static char tmp[GL_INFO_LOG_LENGTH];
        GLsizei len;

        glGetShaderInfoLog(shaderHandle, GL_INFO_LOG_LENGTH-1, &len, tmp);
        tmp[len]='\0';
        fprintf(stderr, "%s\n%s\n", path.c_str(), tmp);
        exit(EXIT_FAILURE);
    }

    return shaderHandle;
}


ShaderProgram::ShaderProgram(std::string frag, std::string vert) {
    fragmentShaderID = loadShader(frag, GL_FRAGMENT_SHADER);
    vertexShaderID = loadShader(vert, GL_VERTEX_SHADER);
    programID = glCreateProgram();
    glAttachShader(programID, fragmentShaderID);
    glAttachShader(programID, vertexShaderID);
    glLinkProgram(programID);
}

ShaderProgram::~ShaderProgram(void) {
    if (fragmentShaderID) {
        glDeleteShader(fragmentShaderID);
        fragmentShaderID = NULL;
    }
    if (vertexShaderID) {
        glDeleteShader(vertexShaderID);
        vertexShaderID = NULL;
    }
    if (programID) {
        glDeleteShader(programID);
        programID = NULL;
    }
}

void ShaderProgram::Use(void) const {
    glUseProgram(programID);
}

GLuint ShaderProgram::getProgID(void) const { return programID; }