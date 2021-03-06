// Copyright [year] <Copyright Owner>
#ifndef _SHADER_H_
#define _SHADER_H_
#include "../../essentials.h"
#include "GL.H"

class ShaderManager {
public:
    static int16_t     load(std::string name, std::string frag, std::string vertex);
    static bool        bind(std::string name);
    static int16_t     get_program_id(std::string name);
    static void         shutdown(void);
private:
    class Impl;
    std::unique_ptr<Impl> mImpl;

    // non copyable, singular
    ShaderManager(const ShaderManager& rhs);
    ~ShaderManager(void);
    const ShaderManager& operator=(const ShaderManager& rhs);
};

#endif // ~_SHADER_H_   