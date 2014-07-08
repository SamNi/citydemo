#include "Texture.h"

Texture::Texture(int w, int h) : width(w),height(h) {
    assert(w && h);

    pixels = new uint8_t[w*h*3];
    
    for (int i = 0;i < w*h;++i) {
        Real mid = 255*(i / ((Real)w*h));

        pixels[3*i+0] = mid;
        pixels[3*i+1] = 0;
        pixels[3*i+2] = 255 - mid;
    }

    glGenTextures(1, &texID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
}

Texture::~Texture(void) {
    if (pixels)
        delete[] pixels;
}

void Texture::Use(void) const {
    glBindTexture(GL_TEXTURE_2D, texID);
}

GLuint Texture::getTexID(void) const {
    return texID;
}

uint8_t *Texture::getPixels(void) const {
    return pixels;
}