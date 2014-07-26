// Copyright [year] <Copyright Owner>
#ifndef FBO_H_
#define FBO_H_
#include "./essentials.h"
#include "./GL.h"

struct FBO {
    FBO(void);
    ~FBO(void);

    void Bind(void);
    void Unbind(void);
    void Blit();

    GLuint fboId, renderTargetId;
    GLuint vbo, vbo_tex, vao;
};

#endif  // FBO_H_
