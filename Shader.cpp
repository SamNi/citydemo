// Copyright [year] <Copyright Owner>
#include "Shader.h"
#include <physfs/physfs.h>

struct ShaderProgram {
    ShaderProgram(std::string frag, std::string vertex);
    ~ShaderProgram(void);
    void Bind(void) const;

    GLuint programID;
    GLuint vertexShaderID;
    GLuint fragmentShaderID;
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
    if (GL_TRUE!=params) {
        static char tmp[GL_INFO_LOG_LENGTH];
        GLsizei len;

        glGetShaderInfoLog(shaderHandle, GL_INFO_LOG_LENGTH-1, &len, tmp);
        tmp[len]='\0';
        LOG(LOG_CRITICAL, "%s\n%s\n", path.c_str(), tmp);
        exit(EXIT_FAILURE);
    }

    return shaderHandle;
}


ShaderProgram::ShaderProgram(std::string frag, std::string vert) {
    fragmentShaderID = loadShader(frag, GL_FRAGMENT_SHADER);
    vertexShaderID = loadShader(vert, GL_VERTEX_SHADER);

    programID = glCreateProgram();
    checkGL();
    glAttachShader(programID, fragmentShaderID);
    glAttachShader(programID, vertexShaderID);
    glLinkProgram(programID);
    checkGL();
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
// ----------------------------------
// ShaderManager
// ----------------------------------
#include <unordered_map>
class ShaderManager::Impl {
public:
    static std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> progs;
};
std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> ShaderManager::Impl::progs;

uint32_t ShaderManager::Load(std::string name, std::string frag, std::string vertex) {
    auto p = std::shared_ptr<ShaderProgram>(new ShaderProgram(frag, vertex));
    ShaderManager::Impl::progs.emplace(name, p);
    checkGL();
    return p->programID;
}

void ShaderManager::Bind(std::string name) {
    auto it = ShaderManager::Impl::progs.find(name);
    if (it == ShaderManager::Impl::progs.end()) {
        LOG(LOG_WARNING, "ShaderManager::Bind nonexistent id %s\n", name.c_str());
        return;
    }
    it->second->Bind();
}

uint32_t ShaderManager::GetProgID(std::string name) {
    auto it = ShaderManager::Impl::progs.find(name);
    if (it == ShaderManager::Impl::progs.end()) {
        LOG(LOG_WARNING, "ShaderManager::GetProgID no shader with name %s\n", name.c_str());
        return 0;
    }
    return it->second->programID;
}

void ShaderManager::Shutdown(void) {
    ShaderManager::Impl::progs.clear();
}

#include "./LuaBindings.h"
namespace Lua {

int L_LoadShader(lua_State *L) {
    if (lua_gettop(L) != 3)
        luaL_error(L, "Usage: LoadShader(name, frag, vert)");
    auto name = lua_tostring(L, 1);
    auto frag = lua_tostring(L, 2);
    auto vert = lua_tostring(L, 3);
    ShaderManager::Load(name, frag, vert);
    checkGL();
    return 0;
}

int L_BindShader(lua_State *L) {
    if (lua_gettop(L) != 1)
        luaL_error(L, "Usage: BindShader(textureID)");
    auto textureID = lua_tostring(L,1);
    ShaderManager::Bind(textureID);
    checkGL();
    return 0;
}

}  // Lua