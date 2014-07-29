// Copyright [year] <Copyright Owner>
#ifndef _TEXTURE_H_
#define _TEXTURE_H_
#include "essentials.h"
#include "GL.H"

class TextureManager { 
public:
    static uint32_t                 Load(const char *path);
    static void                     Bind(uint32_t textureID);
    static void                     Delete(uint32_t textureID);
private:
    // non copyable, singular
    TextureManager(const TextureManager& rhs);
    ~TextureManager(void);
    const TextureManager& operator=(const TextureManager& rhs);
};


#endif // ~_TEXTURE_H_