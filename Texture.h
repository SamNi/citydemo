#ifndef _TEXTURE_H_
#define _TEXTURE_H_
#include "essentials.h"
#include "GL.H"

struct Texture {
    Texture(int w, int h);
    ~Texture(void);


    uint8_t *getPixels(void) const;
    void Use(void) const;

    GLuint getTexID(void) const;


private:
    int width, height;
    GLuint texID;
    uint8_t *pixels;
};

#endif // ~_TEXTURE_H_