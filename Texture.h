#ifndef _TEXTURE_H_
#define _TEXTURE_H_
#include "essentials.h"
#include "GL.H"

struct Texture {
    Texture(void);
    Texture(int w, int h);
    Texture(char *fname);
    ~Texture(void);

    void Bind(void) const;
    void Refresh(void);

    uint8_t *getPixels(void) const;

    GLuint getTexID(void) const;

    int width, height;
    GLuint texID, format;
    uint8_t *pixels;
    bool filter;

private:
    bool Alloc(int nbytes);
    bool Alloc(int w, int h);
    void DeAlloc(void);
    void UpGL(GLuint fmt);
    void MakeCheckerboard(void);
};

struct TextureManager {
};

#endif // ~_TEXTURE_H_