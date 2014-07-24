#include "Shader.h"

#define SHADER_DIR                  "shaders"
const char *names[] = { 
    "standard",
    "diffuse",
    "white",
    "textured",
    "passthrough",
    "2dortho",
    NULL };

struct ShaderProgram {
    ShaderProgram(std::string frag, std::string vertex);
    ~ShaderProgram(void);
    void Bind(void) const;
    GLuint getProgID(void) const;

private:
    GLuint programID;
    GLuint vertexShaderID;
    GLuint fragmentShaderID;
    GLuint computeShaderID;
};



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

    GLint err;
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

void ShaderProgram::Bind(void) const {
    glUseProgram(programID);
}

GLuint ShaderProgram::getProgID(void) const { return programID; }

void ShaderManager::load(std::string name, std::string frag, std::string vertex) {
    assert(progs.find(name) == progs.end());

    ShaderProgram *p = new ShaderProgram(frag, vertex);
    progs[name] = p;
}

void ShaderManager::load(void) {
    const char **name;
    int count = 0;

    for (name = names;*name;++name,++count) {
        fprintf(stderr,"%s\n", *name);

        std::string path = std::string(SHADER_DIR) + std::string(PATH_SEP) + std::string(*name);
        std::string fragPath = path + std::string(".frag");
        std::string vertPath = path + std::string(".vert");
        load(std::string(*name), fragPath, vertPath);
    }
    fprintf(stderr,"successfully compiled, linked %d shader program(s)\n", count);
}

void ShaderManager::use(std::string name) {
    assert(progs.find(name) != progs.end());

    progs[name]->Bind();
}

GLuint ShaderManager::getProgID(const std::string& name) {
    assert(progs.find(name) != progs.end());
    return progs[name]->getProgID();
}

ShaderManager::~ShaderManager(void) {
    std::map<std::string, ShaderProgram*>::iterator it;
    for (it = progs.begin();it != progs.end();++it) {
        delete (it->second);
    }
}